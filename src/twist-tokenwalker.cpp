#include "twist-tokens.cpp"
#pragma once

struct TokenWalker {
    vector<Token>* tokens;
    size_t token_index = 0;

    TokenWalker(vector<Token>* tokens) {
        this->tokens = tokens;
    }

    inline void next() { token_index++; }
    inline void before() { token_index--; }
    inline Token* get(const int offset = 0) { return &(*tokens)[token_index + offset]; }
    inline bool isEnd() { return token_index + 1 >= tokens->size(); }
    inline bool CheckType(TokenType type, const int offset = 0) {
        return get(offset)->type == type;
    }
    inline bool CheckValue(const string& value, const int offset = 0) {
        return get(offset)->value == value;
    }
};