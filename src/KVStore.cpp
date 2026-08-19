#include<unordered_map>
#include<string>
#include<mutex>
#include "KVStore.h"

void KVStore::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    kvstore[key] = value;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
        
    if(it == kvstore.end())
      return std::nullopt;
    return it->second;
}

bool KVStore::erase(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    return kvstore.erase(key) > 0;
}

bool KVStore::exists(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex);

    return (kvstore.find(key) != kvstore.end());
}