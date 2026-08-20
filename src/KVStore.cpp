#include<unordered_map>
#include<string>
#include<mutex>
#include "KVStore.h"

void KVStore::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    Entry entry;
    entry.value = value;
    entry.has_expiry = false;

    kvstore[key] = std::move(entry);
}

void KVStore::set(const std::string& key, const std::string& value, std::chrono::seconds ttl) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    Entry entry;
    entry.value = value;
    entry.expiry = std::chrono::steady_clock::now() + ttl;
    entry.has_expiry = true;

    kvstore[key] = std::move(entry);
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    if(it == kvstore.end())
      return std::nullopt;
    
    Entry& entry = it->second;

    if(entry.has_expiry && std::chrono::steady_clock::now() >= entry.expiry) {
        kvstore.erase(it);
        return std::nullopt;
    }
    return entry.value;
}

bool KVStore::erase(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    return kvstore.erase(key) > 0;
}

bool KVStore::exists(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    if(it == kvstore.end())
        return false;

    Entry& entry = it->second;
    if(entry.has_expiry && std::chrono::steady_clock::now() >= entry.expiry) {
        kvstore.erase(it);
        return false;
    }
    return true;
}