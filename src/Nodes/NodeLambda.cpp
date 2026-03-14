#include "../twist-nodetemp.cpp"
#include "../twist-args.cpp"
#include "../twist-lambda.cpp"
#include "../twist-err.cpp"

struct NodeLambda : public Node { NO_EXEC
    vector<Arg*> args;
    Node* return_type;
    Node* body;

    Token start_args_token;
    Token end_args_token;
    Token start_type_token;
    Token end_type_token;

    string name = "";

    NodeLambda(Node* body, vector<Arg*> args, Node* return_type,
               Token start_args_token, Token end_args_token, Token start_type_token, Token end_type_token) :
        body(body), args(args), return_type(return_type),
        start_args_token(start_args_token), end_args_token(end_args_token), start_type_token(start_type_token), end_type_token(end_type_token) {
            this->NODE_TYPE = NodeTypes::NODE_LAMBDA;
        }


    Value eval_from(Memory& _memory) override {
        
        auto new_lambda_memory = new Memory();
        
        _memory.link_objects(*new_lambda_memory);
        
        
        for (auto arg : args) {
            if (arg->type_expr) {
                auto super_type_value = arg->type_expr->eval_from(_memory);
                if (super_type_value.type != STANDART_TYPE::TYPE)
                    throw ERROR_THROW::WaitedLambdaArgumentTypeSpecifier(start_args_token, end_args_token, arg->name);
            }
        }
        
       
        auto super_type_value = return_type->eval_from(_memory);
        
        if (super_type_value.type != STANDART_TYPE::TYPE)
            throw ERROR_THROW::WaitedLambdaReturnTypeSpecifier(start_type_token, end_type_token, super_type_value.type);
        
        
        auto lambda = NewLambda(new_lambda_memory, body, args, return_type, name,
                                start_args_token, end_args_token, start_type_token, end_type_token);

        if (name != "") {
            // Добавляем лямбду в её собственную память под заданным именем
            (any_cast<Lambda*>(lambda.data))->memory->add_object(name, lambda, lambda.type,
                                                                true, true, true, true, false);
        }
        return lambda;
    }
};