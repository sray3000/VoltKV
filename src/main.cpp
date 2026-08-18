#pragma once
#include<iostream>
#include<string>
#include "KVStore.h"

int main() {
    KVStore store;

    while(1) {
        std::string inp;

        std::cout << "> ";
        std::getline(std::cin, inp);

        std::string ops;
        int ind = 0, n = inp.length();
        while(ind < n && inp[ind] != ' ') {
            ops.push_back(inp[ind]);
            ind++;
        }

        if(ops == "SET") {
            std::string key, value;
            ind++;
            while(ind < n && inp[ind] != ' ') {
                key.push_back(inp[ind]);
                ind++;
            }
            ind++;
            while(ind < n && inp[ind] != ' ') {
                value.push_back(inp[ind]);
                ind++;
            }

            store.set(key, value);
            std::cout << "OK\n";
        } else if(ops == "GET") {
            std::string key;
            ind++;
            while(ind < n && inp[ind] != ' ') {
                key.push_back(inp[ind]);
                ind++;
            }

            auto value = store.get(key);

            if(value)
              std::cout << *value << "\n";
            else 
              std::cout << "NOT_FOUND\n";
        } else if(ops == "DELETE") {
            std::string key;
            ind++;
            while(ind < n && inp[ind] != ' ') {
                key.push_back(inp[ind]);
                ind++;
            }

            auto status = store.erase(key);
            std::cout << status << "\n";
        } else if(ops == "EXISTS") {
            std::string key;
            ind++;
            while(ind < n && inp[ind] != ' ') {
                key.push_back(inp[ind]);
                ind++;
            }

            auto status = store.exists(key);
            std::cout << status << "\n";
        } else if(ops == "exit") {
            break;
        } else {
            std::cout << "Invalid operation!!\n";
        }
    }
}