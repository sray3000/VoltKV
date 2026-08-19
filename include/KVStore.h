#pragma once
#include<unordered_map>
#include<string>
#include<optional>

class KVStore {
private:
    std::unordered_map<std::string, std::string> kvstore;

public:
    void set(const std::string& key, const std::string& value);

    std::optional<std::string> get(const std::string& key);

    bool erase(const std::string& key);

    bool exists(const std::string& key);
};