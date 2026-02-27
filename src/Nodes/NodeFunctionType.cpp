#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

struct NodeNewFuncType : public Node { NO_EXEC
    vector<unique_ptr<Node>> args_types_expr;
    unique_ptr<Node> return_type_expr;

    Token start_token_args;
    Token end_token_args;

    Token start_token_return;
    Token end_token_return;

    NodeNewFuncType(vector<unique_ptr<Node>> args_types, unique_ptr<Node> return_type_expr,
                    Token start_token_args, Token end_token_args, Token start_token_return, Token end_token_return) :
                    args_types_expr(std::move(args_types)), return_type_expr(std::move(return_type_expr)),
                    start_token_args(start_token_args), end_token_args(end_token_args),
                    start_token_return(start_token_return), end_token_return(end_token_return) {
        this->NODE_TYPE = NodeTypes::NODE_FUNCTION_TYPE;
    }


    Value eval_from(Memory& _memory) override {
        vector<Type> args_types;

        for (int i = 0; i < args_types_expr.size(); i++) {
            auto value = args_types_expr[i]->eval_from(_memory);
            if (value.type != STANDART_TYPE::TYPE)
                ERROR::WaitedFuncTypeArgumentTypeSpecifier(start_token_args, end_token_args, i);
            args_types.push_back(any_cast<Type>(value.data));
        }


        if (return_type_expr) {
            auto value = return_type_expr->eval_from(_memory);
            if (value.type != STANDART_TYPE::TYPE)
                ERROR::WaitedFuncTypeReturnTypeSpecifier(start_token_return, end_token_return);

            auto result = create_function_type(any_cast<Type>(value.data), args_types);
            return NewType(result);
        } else {
            auto result = create_function_type(args_types);
            return NewType(result);
        }

    }
}; 