#include <iostream>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "Server.h"
#include "CmdParser.h"

Server::Server(int port, KVStore& store)
    : port(port), server_fd(-1), store(store) {
}

void Server::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd == -1) {
        perror("socket");
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

    int client_fd = accept(server_fd, nullptr, nullptr);
    if(client_fd == -1) {
        perror("accept");
        close(server_fd);
        return;
    }

    std::cout << "Connected to client...\n";

    std::string recv_buffer;

    while(1) {
        char buffer[1024];
        ssize_t bytes_recd = recv(client_fd, buffer, sizeof(buffer), 0);

        if (bytes_recd == 0) {
            std::cout << "Client disconnected.\n";
            close(client_fd);
            close(server_fd);
            return;
        }

        if(bytes_recd < 0) {
            perror("recv");
            close(client_fd);
            close(server_fd);
            return;
        }

        recv_buffer.append(buffer, bytes_recd);

        size_t pos;

        while((pos = recv_buffer.find('\n')) != std::string::npos) {
            std::string command = recv_buffer.substr(0, pos);
            recv_buffer.erase(0, pos + 1);

            Parser parser(command);
            std::vector<std::string> tokens;

            tokens = parser.parse();

            bool valid = true;

            if(tokens.size() < 1 || tokens.size() > 3) {
                valid = false;
            } else if(tokens[0] == "SET") {
                valid = (tokens.size() == 3);
            } else if(tokens[0] == "GET" || tokens[0] == "DELETE" || tokens[0] == "EXISTS") {
                valid = (tokens.size() == 2);
            } else {
                valid = false;
            }

            if(!valid) {
                if(!sendResponse(client_fd, "ERROR invalid command\n")) {
                    close(client_fd);
                    close(server_fd);
                    return;
                }
                continue;
            }

            std::string cmd = tokens[0];

            if(cmd == "SET") {
                std::string key = tokens[1];
                std::string value = tokens[2];

                store.set(key, value);

                if(!sendResponse(client_fd, "OK\n")) {
                    close(client_fd);
                    close(server_fd);
                    return;
                }
            } else if(cmd == "GET") {
                std::string key = tokens[1];

                auto value = store.get(key);

                std::string message = value ? (*value + "\n") : "NOT_FOUND\n";
                if(!sendResponse(client_fd, message)) {
                    close(client_fd);
                    close(server_fd);
                    return;
                }
            } else if(cmd == "DELETE") {
                std::string key = tokens[1];

                auto status = store.erase(key);

                std::string message = status ? "1\n" : "0\n";
                if(!sendResponse(client_fd, message)) {
                    close(client_fd);
                    close(server_fd);
                    return;
                }
            } else if(cmd == "EXISTS") {
                std::string key = tokens[1];

                auto status = store.exists(key);

                std::string message = status ? "1\n" : "0\n";
                if(!sendResponse(client_fd, message)) {
                    close(client_fd);
                    close(server_fd);
                    return;
                }
            }
        }
    }
    close(client_fd);
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