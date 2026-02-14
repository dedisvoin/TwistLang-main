#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "vector"
#include "twist-args.cpp"
#include <memory>


using namespace std;

struct Function {
    shared_ptr<Memory> memory; 
    void* body;
    vector<Arg*> arguments;
    unique_ptr<Node> return_type;

    Token start_args_token;
    Token end_args_token;
    Token start_return_type_token;
    Token end_return_type_token;



    string name;
    
    Function(string name,shared_ptr<Memory> memory, void* body, vector<Arg*> args, 
           unique_ptr<Node> return_type, Token start_args_token, Token end_args_token, Token start_return_type_token, Token end_return_type_token)
        : name(name),memory(memory), body(body), arguments(std::move(args)),
          return_type(std::move(return_type)), start_args_token(start_args_token), end_args_token(end_args_token), start_return_type_token(start_return_type_token), end_return_type_token(end_return_type_token) {}
};



Value NewFunction(string name, shared_ptr<Memory> memory, void* body, std::vector<Arg*> args, unique_ptr<Node> return_type, Type func_type,
    Token start_args_token, Token end_args_token, Token start_return_type_token, Token end_return_type_token) {
    Function* func = new Function(name, memory, body, std::move(args), std::move(return_type), start_args_token, end_args_token, start_return_type_token, end_return_type_token);
    return Value(func_type, func);
}


