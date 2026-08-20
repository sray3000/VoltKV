#include<iostream>
#include<string>
#include "KVStore.h"
#include "Server.h"
#include "CmdParser.h"

int main() {
    KVStore store;

    Server server(PORT, store, NUM_THREADS);
    server.start();

    return 0;
}