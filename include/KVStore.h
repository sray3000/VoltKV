#pragma once
#include<unordered_map>
#include<string>
#include<optional>
#include<shared_mutex>

class KVStore {
private:
    std::unordered_map<std::string, std::string> kvstore;
    std::shared_mutex mutex;

public:
    void set(const std::string&, const std::string&);

    std::optional<std::string> get(const std::string&);

    bool erase(const std::string&);

    bool exists(const std::string&);
};