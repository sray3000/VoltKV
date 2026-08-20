#pragma once

#include <string>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <memory>
#include "KVStore.h"
#include "ThreadPool.h"

#define LOCALHOST "127.0.0.1"
#define PORT 6379

struct ClientState {
    std::string recv_buffer;
    std::deque<std::string> commands;
    bool processing = false;
    bool active = true;
    std::mutex mutex;
};

class Server {
private:
    int port;
    int server_fd;
    int epoll_fd;
    KVStore& store;
    ThreadPool pool;
    std::unordered_map<int, std::shared_ptr<ClientState>> clients;

    bool sendResponse(int, const std::string&);
    void handleClient(int);
    void processCommand(int, const std::string&, std::shared_ptr<ClientState>);
    void processNextCommand(int, std::shared_ptr<ClientState>);
public:
    Server(int, KVStore&, size_t);
    void start();
};