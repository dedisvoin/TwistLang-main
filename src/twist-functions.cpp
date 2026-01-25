#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "vector"
#include "twist-args.cpp"
#include <memory>

using namespace std;

struct Function {
    shared_ptr<Memory> memory;  // Изменяем с Memory* на shared_ptr<Memory>
    unique_ptr<Node> body;
    vector<Arg*> arguments;
    unique_ptr<Node> return_type;
    
    Function(shared_ptr<Memory> memory, unique_ptr<Node> body, vector<Arg*> args, 
           unique_ptr<Node> return_type)
        : memory(memory), body(std::move(body)), arguments(std::move(args)),
          return_type(std::move(return_type)) {}
};



Value NewFunction(shared_ptr<Memory> memory, unique_ptr<Node> body, std::vector<Arg*> args, unique_ptr<Node> return_type, Type func_type) {
    Function* lambda = new Function(memory, std::move(body), std::move(args), std::move(return_type));
    return Value(func_type, lambda);
}