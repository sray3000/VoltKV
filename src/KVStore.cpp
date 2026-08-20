#include<unordered_map>
#include<string>
#include<mutex>
#include "KVStore.h"

KVStore::KVStore(size_t capacity)
    : max_capacity(capacity) {
}

void KVStore::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    // Existing key: update value and move to MRU.
    if(it != kvstore.end()) {
        it->second.value = value;
        it->second.has_expiry = false;
        lru_list.splice(lru_list.begin(), lru_list, it->second.lru_iterator
        );
        return;
    }

    // New key: add it to the front (MRU).
    lru_list.push_front(key);

    Entry entry;
    entry.value = value;
    entry.has_expiry = false;
    entry.lru_iterator = lru_list.begin();

    kvstore[key] = std::move(entry);

    // Evict least recently used entry.
    if(kvstore.size() > max_capacity) {
        const std::string& lru_key = lru_list.back();
        kvstore.erase(lru_key);
        lru_list.pop_back();
    }
}

void KVStore::set(const std::string& key, const std::string& value, std::chrono::seconds ttl) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    // Existing key: update value, TTL, and move to MRU.
    if(it != kvstore.end()) {
        it->second.value = value;
        it->second.expiry = std::chrono::steady_clock::now() + ttl;
        it->second.has_expiry = true;

        lru_list.splice(lru_list.begin(), lru_list, it->second.lru_iterator);
        return;
    }

    // New key: add it to the front (MRU).
    lru_list.push_front(key);

    Entry entry;
    entry.value = value;
    entry.expiry = std::chrono::steady_clock::now() + ttl;
    entry.has_expiry = true;

    entry.lru_iterator = lru_list.begin();
    kvstore[key] = std::move(entry);

    // Evict least recently used entry.
    if(kvstore.size() > max_capacity) {
        const std::string& lru_key = lru_list.back();
        kvstore.erase(lru_key);
        lru_list.pop_back();
    }
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    if(it == kvstore.end())
      return std::nullopt;
    
    Entry& entry = it->second;

    // Lazy TTL expiration.
    if(entry.has_expiry && std::chrono::steady_clock::now() >= entry.expiry) {
        lru_list.erase(entry.lru_iterator);
        kvstore.erase(it);
        return std::nullopt;
    }

    // Key was accessed, so make it most recently used.
    lru_list.splice(lru_list.begin(), lru_list, entry.lru_iterator);

    return entry.value;
}

bool KVStore::erase(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    if(it == kvstore.end())
        return false;

    lru_list.erase(it->second.lru_iterator);
    kvstore.erase(it);
    return true;
}

bool KVStore::exists(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    auto it = kvstore.find(key);
    if(it == kvstore.end())
        return false;

    Entry& entry = it->second;
    if(entry.has_expiry && std::chrono::steady_clock::now() >= entry.expiry) {
        lru_list.erase(entry.lru_iterator);
        kvstore.erase(it);
        return false;
    }
    return true;
}