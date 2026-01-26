#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "vector"
#include "twist-args.cpp"
#include <memory>

using namespace std;

struct Lambda {
    shared_ptr<Memory> memory;  // Изменяем с Memory* на shared_ptr<Memory>
    void* expr;
    vector<Arg*> arguments;
    unique_ptr<Node> return_type;
    
    Token start_args_token;
    Token end_args_token;
    Token start_type_token;
    Token end_type_token;
    
    Lambda(shared_ptr<Memory> memory, void* expr, vector<Arg*> args, 
           unique_ptr<Node> return_type, 
           Token start_args_token, Token end_args_token, Token start_type_token, Token end_type_token)
        : memory(memory), expr(expr), arguments(args),
          return_type(std::move(return_type)), start_args_token(start_args_token), 
          end_args_token(end_args_token), start_type_token(start_type_token), end_type_token(end_type_token) {}
};

Value NewLambda(shared_ptr<Memory> memory, void* expr, std::vector<Arg*> arguments, unique_ptr<Node> return_type,
                Token start_args_token, Token end_args_token, Token start_type_token, Token end_type_token) {
    Lambda* lambda = new Lambda(memory, expr, arguments, std::move(return_type), start_args_token, end_args_token, start_type_token, end_type_token);
    return Value(STANDART_TYPE::LAMBDA, lambda);
}