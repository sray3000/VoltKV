#pragma once
#include<unordered_map>
#include<string>
#include<optional>
#include<shared_mutex>
#include<chrono>

class KVStore {
private:
    struct Entry {
        std::string value;
        // No expiration by default.
        std::chrono::steady_clock::time_point expiry;
        bool has_expiry = false;
    };

    std::unordered_map<std::string, Entry> kvstore;
    mutable std::shared_mutex mutex;

public:
    void set(const std::string&, const std::string&);
    void set(const std::string&, const std::string&, std::chrono::seconds);
    std::optional<std::string> get(const std::string&);
    bool erase(const std::string&);
    bool exists(const std::string&);
};