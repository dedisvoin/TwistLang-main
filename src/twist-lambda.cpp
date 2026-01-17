#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "vector"
#include "twist-args.cpp"
#include <memory>

using namespace std;

struct Lambda {
    Memory* memory;  // shared_ptr, но будет создаваться новый при каждом вызове
    void* expr;
    vector<Arg> arguments;
    void* return_type;
    
    Token start_args_token;
    Token end_args_token;
    
    Lambda(Memory* memory, void* expr, vector<Arg> args, 
           void* return_type, Token start_args_token, Token end_args_token)
        : memory(memory), expr(expr), arguments(std::move(args)),
          return_type(return_type), start_args_token(start_args_token), 
          end_args_token(end_args_token) {}

};


Value NewLambda(Memory* memory, void* expr, std::vector<Arg> arguments, void* return_type,
                Token start_args_token, Token end_args_token) {
    Lambda lambda = Lambda(memory, expr, arguments, return_type, start_args_token, end_args_token);
    return Value(make_unique<Type>(STANDART_TYPE::LAMBDA), lambda);
}