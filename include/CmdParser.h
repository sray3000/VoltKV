#pragma once
#include<string>
#include<vector>

class Parser {
private:
    std::string inp;

public:
    Parser(const std::string&);
    std::vector<std::string> parse();
};