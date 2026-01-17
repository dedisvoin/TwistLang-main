#include "string"
#include <vector>
#pragma once

using namespace std;

/*
    Token types
*/
enum TokenType {
    NUMBER, STRING, OPERATOR, LITERAL, CHAR,
    DAC, DAD,
    L_BRACKET, R_BRACKET,
    L_RECT_BRACKET, R_RECT_BRACKET,
    L_CURVE_BRACKET, R_CURVE_BRACKET,
    KEYWORD,
    END_OF_FILE,
    DUMMY // Dummy token
};
 const vector<string> TTTypeArray = {
    "NUMBER", "STRING", "OPERATOR", "LITERAL", "CHAR",
    "DAC", "DAD",
    "L_BRACKET","R_BRACKET",
    "L_RECT_BRACKET", "R_RECT_BRACKET",
    "L_CURVE_BRACKET", "R_CURVE_BRACKET",
    "KEYWORD",
    "END_OF_FILE"
};

const string TokenTypeToString(TokenType T) noexcept{
    return TTTypeArray[T];
}



struct PosInFile {
    string file_name;
    int line;
    int index;
    int lenght;

    friend ostream& operator<< (ostream& os, const PosInFile& pif) {
        os << "'" + pif.file_name + "':" + to_string(pif.line) + ":" + to_string(pif.index);
        return os;
    }
};

struct Token {
    TokenType type;
    string value;
    PosInFile pif;

    Token() = default;
    Token(TokenType type, string value, PosInFile pif) : type(type), value(value), pif(pif) {}

    bool operator==(Token token) {
        return this->type == token.type;
    }

    bool operator!=(Token token) {
        return (!this->operator==(token));
    }
};