#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "Server.h"

constexpr int OPERATIONS_PER_CLIENT = 10000;
constexpr int GET_PERCENT = 90;

void runClient(int client_id) {

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd == -1) {
        perror("socket");
        return;
    }

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if(inet_pton(
            AF_INET,
            LOCALHOST,
            &server_address.sin_addr
        ) <= 0) {

        perror("inet_pton");
        close(fd);
        return;
    }

    if(connect(
            fd,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)
        ) == -1) {

        perror("connect");
        close(fd);
        return;
    }

    char buffer[1024];

    for(int i = 0; i < OPERATIONS_PER_CLIENT; i++) {

        std::string command;

        // 90% GET, 10% SET
        if(i % 100 < GET_PERCENT) {

            command =
                "SET key_" +
                std::to_string(client_id) +
                " value\n";

        } else {

            command =
                "GET key_" +
                std::to_string(client_id) +
                "\n";
        }

        ssize_t total_sent = 0;

        while(total_sent <
              static_cast<ssize_t>(command.size())) {

            ssize_t sent = send(
                fd,
                command.data() + total_sent,
                command.size() - total_sent,
                0
            );

            if(sent <= 0) {
                perror("send");
                close(fd);
                return;
            }

            total_sent += sent;
        }

        // Read until we receive the newline-terminated response.
        std::string response;

        while(response.find('\n') == std::string::npos) {

            ssize_t received = recv(
                fd,
                buffer,
                sizeof(buffer),
                0
            );

            if(received <= 0) {
                perror("recv");
                close(fd);
                return;
            }

            response.append(buffer, received);
        }
    }

    close(fd);
}


void runBenchmark(int num_clients) {

    std::cout
        << "\nClients: "
        << num_clients
        << "\n";

    std::cout
        << "Operations/client: "
        << OPERATIONS_PER_CLIENT
        << "\n";

    const long long total_operations =
        static_cast<long long>(num_clients) *
        OPERATIONS_PER_CLIENT;

    std::vector<std::thread> clients;

    clients.reserve(num_clients);

    auto start =
        std::chrono::steady_clock::now();

    for(int i = 0; i < num_clients; i++) {

        clients.emplace_back(
            runClient,
            i
        );
    }

    for(auto& client : clients) {
        client.join();
    }

    auto end =
        std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed =
        end - start;

    double throughput =
        total_operations /
        elapsed.count();

    std::cout
        << "Total operations: "
        << total_operations
        << "\n";

    std::cout
        << "Time: "
        << elapsed.count()
        << " s\n";

    std::cout
        << "Throughput: "
        << throughput
        << " ops/sec\n";
}


int main() {

    std::cout
        << "========================================\n"
        << " VoltKV End-to-End Benchmark\n"
        << "========================================\n";

    std::cout
        << "Workload: 90% GET / 10% SET\n"
        << "Operations/client: "
        << OPERATIONS_PER_CLIENT
        << "\n";

    const std::vector<int> client_counts = {
        1, 2, 4, 8, 16
    };

    for(int clients : client_counts) {
        runBenchmark(clients);
    }

    std::cout
        << "\n========================================\n"
        << " Benchmark complete\n"
        << "========================================\n";

    return 0;
}