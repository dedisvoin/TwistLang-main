#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

struct NodeNewArrayType : public Node { NO_EXEC
    Node* type_expr;
    Node* size_expr;
    Token start_token;
    Token end_token;

    NodeNewArrayType(Node* type_expr, Node* size_expr, Token start_token, Token end_token)
        : type_expr(type_expr), size_expr(size_expr), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_ARRAY_TYPE;
    }

    Value eval_from(Memory& _memory) override {
        if (type_expr) {
            auto type_value = type_expr->eval_from(_memory);

            if (type_value.type != STANDART_TYPE::TYPE) {
                ERROR::InvalidArrayTypeExpression(start_token, end_token);
            }

            if (size_expr) {
                auto size_value = size_expr->eval_from(_memory);
                if (size_value.type != STANDART_TYPE::INT) {
                    ERROR::InvalidArraySize(start_token);
                }

                auto ret_type = "[" + any_cast<Type>(type_value.data).pool + ", " + to_string(any_cast<int64_t>(size_value.data)) + "]";
                return Value(STANDART_TYPE::TYPE, Type(ret_type));
            }

            auto ret_type = "[" + any_cast<Type>(type_value.data).pool + ", ~]";

            return Value(STANDART_TYPE::TYPE, Type(ret_type));
        } else {
            auto ret_type = "[, ~]";

            return Value(STANDART_TYPE::TYPE, Type(ret_type));
        }
    }
};
