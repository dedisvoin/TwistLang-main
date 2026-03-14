#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "twist-tokens.cpp"
#include "twist-args.cpp"
#include "vector"


#pragma once


using namespace std;

struct Function {
    shared_ptr<Memory> memory; 
    Node* body;
    vector<Arg*> arguments;
    Node* return_type;

    Token start_args_token;
    Token end_args_token;
    Token start_return_type_token;
    Token end_return_type_token;

    Type type;
    string name;
    
    Function(string name, shared_ptr<Memory> memory, Node* body, vector<Arg*> args, 
           Node* return_type, 
           Type type,
           Token start_args_token, Token end_args_token, Token start_return_type_token, Token end_return_type_token)
        : name(name), memory(memory), body(body), arguments(std::move(args)),
          return_type(return_type), type(type), start_args_token(start_args_token), end_args_token(end_args_token), start_return_type_token(start_return_type_token), end_return_type_token(end_return_type_token) {}

};

struct Method {
    Function* func;
    std::shared_ptr<Memory> instance_memory;
};



Value NewFunction(string name, shared_ptr<Memory> memory, Node* body, std::vector<Arg*> args, Node* return_type, Type func_type,
    Token start_args_token, Token end_args_token, Token start_return_type_token, Token end_return_type_token) {
    Function* func = new Function(name, memory, body, std::move(args), return_type, func_type, start_args_token, end_args_token, start_return_type_token, end_return_type_token);
    return Value(func_type, func);
}


