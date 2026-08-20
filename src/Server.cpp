#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <cerrno>
#include <chrono>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "Server.h"
#include "CmdParser.h"

Server::Server(int port, KVStore& store, size_t num_threads)
    : port(port), server_fd(-1), epoll_fd(-1), store(store), pool(num_threads) {
}

void Server::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd == -1) {
        perror("socket");
        return;
    }

    int flags = fcntl(server_fd, F_GETFL, 0);

    if(flags == -1) {
        perror("fcntl");
        close(server_fd);
        return;
    }

    if(fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(server_fd);
        return;
    }

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(LOCALHOST);
    address.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return;
    }

    if(listen(server_fd, 3) == -1) {
        perror("listen");
        close(server_fd);
        return;
    }

    std::cout << "Listening on port " << port << "...\n";

    epoll_fd = epoll_create1(0);

    if(epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return;
    }

    epoll_event event{};

    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if(epoll_ctl(
            epoll_fd,
            EPOLL_CTL_ADD,
            server_fd,
            &event
        ) == -1) {

        perror("epoll_ctl");
        close(epoll_fd);
        close(server_fd);
        return;
    }

    constexpr int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while(true) {

        int ready = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );

        if(ready == -1) {
            perror("epoll_wait");
            break;
        }

        for(int i = 0; i < ready; i++) {
            int fd = events[i].data.fd;

            if(fd != server_fd &&
            (events[i].events & (EPOLLERR | EPOLLHUP))) {

                std::cout << "Client error/disconnected: "
                        << fd << "\n";

                epoll_ctl(
                    epoll_fd,
                    EPOLL_CTL_DEL,
                    fd,
                    nullptr
                );

                clients.erase(fd);
                close(fd);

                continue;
            }

            if(fd == server_fd) {
                int client_fd = accept(server_fd, nullptr, nullptr);
                if(client_fd == -1) {
                    perror("accept");
                    continue;
                }

                std::cout << "Client connected: " << client_fd << "\n";

                int flags = fcntl(client_fd, F_GETFL, 0);

                if(flags == -1) {
                    perror("fcntl");
                    close(client_fd);
                    continue;
                }

                if(fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    perror("fcntl");
                    close(client_fd);
                    continue;
                }

                epoll_event client_event{};

                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;

                if(epoll_ctl(
                        epoll_fd,
                        EPOLL_CTL_ADD,
                        client_fd,
                        &client_event
                    ) == -1) {

                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }
                clients.emplace(client_fd, std::make_shared<ClientState>());
            } else {
                handleClient(fd);
            }
        }
    }

    close(epoll_fd);
    close(server_fd);
}

bool Server::sendResponse(int client_fd, const std::string& response) {
    size_t total_sent = 0;

    while(total_sent < response.size()) {
        ssize_t bytes_sent = send(client_fd, response.data() + total_sent, response.size() - total_sent, 0);

        if(bytes_sent <= 0) {
            perror("send");
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

void Server::handleClient(int client_fd) {
    auto it = clients.find(client_fd);

    if(it == clients.end())
        return;

    std::shared_ptr<ClientState> state = it->second;

    char buffer[1024];

    ssize_t bytes_recd = recv(client_fd, buffer, sizeof(buffer), 0);

    if(bytes_recd == 0) {
        std::cout << "Client disconnected: " << client_fd << "\n";
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->active = false;
            state->commands.clear();
        }

        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);

        clients.erase(client_fd);
        close(client_fd);
        return;
    }

    if(bytes_recd < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
          return;
        perror("recv");
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->active = false;
            state->commands.clear();
        }
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        clients.erase(client_fd);
        close(client_fd);
        return;
    }

    state->recv_buffer.append(buffer, bytes_recd);

    std::string& recv_buffer = state->recv_buffer;

    size_t pos;

    {
        std::lock_guard<std::mutex> lock(state->mutex);

        while((pos = recv_buffer.find('\n')) != std::string::npos) {
            std::string command = recv_buffer.substr(0, pos);

            recv_buffer.erase(0, pos + 1);

            if(!command.empty()) {
                state->commands.push_back(command);
            }
        }
    }
    processNextCommand(client_fd, state);
}

void Server::processCommand(int client_fd, const std::string& command, std::shared_ptr<ClientState> state) {
    Parser parser(command);

    std::vector<std::string> tokens = parser.parse();

    bool valid = true;

    if(tokens.size() < 1 || tokens.size() > 4) {
        valid = false;
    } else if(tokens[0] == "SET") {
        valid = (tokens.size() == 3 || tokens.size() == 4);
    } else if(tokens[0] == "GET" || tokens[0] == "DELETE" || tokens[0] == "EXISTS") {
        valid = (tokens.size() == 2);
    } else {
        valid = false;
    }

    if(!valid) {
        std::lock_guard<std::mutex> lock(state->mutex);

        if(!state->active)
            return;
        sendResponse(client_fd, "ERROR invalid command\n");
        return;
    }

    std::string cmd = tokens[0];

    if(cmd == "SET") {
        std::string key = tokens[1];
        std::string value = tokens[2];

        if(tokens.size() == 3)
          store.set(key, value);
        else {
            try {
                long long ttl = std::stoll(tokens[3]);

                if(ttl <= 0) {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    if(!state->active)
                        return;
                    sendResponse(client_fd, "ERROR invalid TTL\n");
                    return;
                }

                store.set(key, value, std::chrono::seconds(ttl));
            } catch(...) {
                std::lock_guard<std::mutex> lock(state->mutex);
                if(!state->active)
                    return;

                sendResponse(client_fd, "ERROR invalid TTL\n");
                return;
            }
        }

        std::lock_guard<std::mutex> lock(state->mutex);
        if(!state->active)
            return;

        sendResponse(client_fd, "OK\n");
    } else if(cmd == "GET") {
        auto value = store.get(tokens[1]);

        std::string message = value ? (*value + "\n") : "NOT_FOUND\n";

        std::lock_guard<std::mutex> lock(state->mutex);

        if(!state->active)
            return;
        sendResponse(client_fd, message);
    } else if(cmd == "DELETE") {
        bool status = store.erase(tokens[1]);

        std::string message = status ? "1\n" : "0\n";

        std::lock_guard<std::mutex> lock(state->mutex);

        if(!state->active)
            return;
        sendResponse(client_fd, message);
    } else if(cmd == "EXISTS") {
        bool status = store.exists(tokens[1]);

        std::string message = status ? "1\n" : "0\n";

        std::lock_guard<std::mutex> lock(state->mutex);

        if(!state->active)
            return;
        sendResponse(client_fd, message);
    }
}

void Server::processNextCommand(int client_fd, std::shared_ptr<ClientState> state) {
    std::string command;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        if(state->processing || state->commands.empty() || !state->active)
            return;

        command = std::move(state->commands.front());
        state->commands.pop_front();
        state->processing = true;
        pool.enqueue(
            [this, client_fd, state, command]() {
                processCommand(client_fd, command, state);

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->processing = false;
                }
                processNextCommand(client_fd, state);
            }
        );
    }
}