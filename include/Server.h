#pragma once
#include <string>
#include "KVStore.h"

#define LOCALHOST "127.0.0.1"
#define PORT 6379

class Server {
private:
    int port;
    int server_fd;
    KVStore& store;

    bool sendResponse(int, const std::string&);
    void handleClient(int);
public:
    Server(int, KVStore&);
    void start();
};