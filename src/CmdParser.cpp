#include <iostream>
#include "CmdParser.h"

Parser::Parser(const std::string& inp)
    : inp(inp) {
}

std::vector<std::string> Parser::parse() {
    std::vector<std::string> tokens;
    std::string token;

    for(char c: inp) {
        if(c == ' ') {
            if(!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token.push_back(c);
        }
    }

    if(!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}