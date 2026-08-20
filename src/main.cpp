#include<iostream>
#include<string>
#include "KVStore.h"
#include "Server.h"
#include "CmdParser.h"

int main() {
    KVStore store(3);

    Server server(PORT, store, NUM_THREADS);
    server.start();

    return 0;
}