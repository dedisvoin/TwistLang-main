#include "../twist-nodetemp.cpp"
#include "../twist-args.cpp"
#include "../twist-errors.cpp"
#include "../twist-lambda.cpp"

struct NodeLambda : public Node { NO_EXEC
    vector<Arg*> args;
    unique_ptr<Node> return_type;
    unique_ptr<Node> body;

    Token start_args_token;
    Token end_args_token;
    Token start_type_token;
    Token end_type_token;

    string name = "";

    NodeLambda(unique_ptr<Node> body, vector<Arg*> args, unique_ptr<Node> return_type,
               Token start_args_token, Token end_args_token, Token start_type_token, Token end_type_token) :
        body(std::move(body)), args(std::move(args)), return_type(std::move(return_type)),
        start_args_token(start_args_token), end_args_token(end_args_token), start_type_token(start_type_token), end_type_token(end_type_token) {
            this->NODE_TYPE = NodeTypes::NODE_LAMBDA;
        }


    Value eval_from(Memory& _memory) override {
        auto new_lambda_memory = make_shared<Memory>();
        // Копируем глобальные переменные из родительской памяти
        _memory.link_objects(*new_lambda_memory);


        for (auto arg : args) {
            if (arg->type_expr) {
                auto super_type_value = arg->type_expr->eval_from(*new_lambda_memory);
                if (super_type_value.type != STANDART_TYPE::TYPE) {
                    ERROR::WaitedLambdaArgumentTypeSpecifier(start_args_token, end_args_token, arg->name);
                }
            }
        }

        if (return_type) {
            auto super_type_value = return_type->eval_from(*new_lambda_memory);
            if (super_type_value.type != STANDART_TYPE::TYPE) {
                ERROR::WaitedLambdaReturnTypeSpecifier(start_type_token, end_type_token);
            }
        }
        auto lambda = NewLambda(new_lambda_memory, body.get(), vector(args), std::move(return_type), start_args_token, end_args_token, start_type_token, end_type_token);
        if (name != "") {
            (any_cast<Lambda*>(lambda.data))->memory->add_object(name, lambda, lambda.type, true, true, true, true, false);
        }
        return lambda;
    }
};