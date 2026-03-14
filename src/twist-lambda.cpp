#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "twist-tokens.cpp"
#include "twist-args.cpp"

#include "vector"


#pragma once

using namespace std;

struct Lambda {
    Memory* memory;
    void* expr;
    vector<Arg*> arguments;
    Node* return_type;
    string name;   // <-- новое поле

    Token start_args_token;
    Token end_args_token;
    Token start_type_token;
    Token end_type_token;

    Lambda(Memory* memory, void* expr, vector<Arg*> args,
           Node* return_type, string name,
           Token start_args_token, Token end_args_token,
           Token start_type_token, Token end_type_token)
        : memory(memory), expr(expr), arguments(args),
          return_type(return_type), name(name),
          start_args_token(start_args_token), end_args_token(end_args_token),
          start_type_token(start_type_token), end_type_token(end_type_token) {}
};

Value NewLambda(Memory* memory, void* expr, std::vector<Arg*> arguments,
                Node* return_type, string name,
                Token start_args_token, Token end_args_token,
                Token start_type_token, Token end_type_token) {
    Lambda* lambda = new Lambda(memory, expr, arguments, return_type, name,
                                start_args_token, end_args_token, start_type_token, end_type_token);
    return Value(STANDART_TYPE::LAMBDA, lambda);
}