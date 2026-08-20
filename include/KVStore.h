#pragma once
#include<unordered_map>
#include<string>
#include<optional>
#include<shared_mutex>
#include<chrono>
#include<cstddef>
#include <list>

#define CAPACITY 10000

class KVStore {
private:
    struct Entry {
        std::string value;
        // No expiration by default.
        std::chrono::steady_clock::time_point expiry;
        bool has_expiry = false;
        // Position of this key in the LRU list.
        std::list<std::string>::iterator lru_iterator;
    };

    std::unordered_map<std::string, Entry> kvstore;
    size_t max_capacity;
    std::list<std::string> lru_list;
    mutable std::shared_mutex mutex;

public:
    explicit KVStore(size_t capacity = CAPACITY);
    void set(const std::string&, const std::string&);
    void set(const std::string&, const std::string&, std::chrono::seconds);
    std::optional<std::string> get(const std::string&);
    bool erase(const std::string&);
    bool exists(const std::string&);
};