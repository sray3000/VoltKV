#pragma once
#include<unordered_map>
#include<string>
#include "KVStore.h"

void KVStore::set(const std::string& key, const std::string& value) {
    kvstore[key] = value;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    auto it = kvstore.find(key);
        
    if(it == kvstore.end())
      return std::nullopt;
    return it->second;
}

bool KVStore::erase(const std::string& key) {
    return kvstore.erase(key) > 0;
}

bool KVStore::exists(const std::string& key) {
    return (kvstore.find(key) != kvstore.end());
}