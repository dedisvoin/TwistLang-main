#include "../twist-nodetemp.cpp"
#include "../twist-array.cpp"
#include "../twist-namespace.cpp"
#include "../twist-err.cpp"

struct NodeSizeof : public Node { NO_EXEC
    Node* expr;

    Token expr_token;

    NodeSizeof(Node* expr, Token expr_token) : expr(std::move(expr)), expr_token(expr_token){
        this->NODE_TYPE = NodeTypes::NODE_SIZEOF;
    }

    Value eval_from(Memory& _memory) override {
        if (!expr)
            throw ERROR_THROW::UnexpectedToken(expr_token, "expression");

        auto value = expr->eval_from(_memory);
        if (value.type == STANDART_TYPE::INT)
            return NewInt(sizeof(int64_t));
        if (value.type == STANDART_TYPE::DOUBLE)
            return NewInt(sizeof(long double));
        if (value.type == STANDART_TYPE::CHAR)
            return NewInt(sizeof(char));
        if (value.type == STANDART_TYPE::STRING)
            return NewInt(any_cast<string&>(value.data).size());
        if (value.type == STANDART_TYPE::BOOL)
            return NewInt(sizeof(bool));
        if (value.type == STANDART_TYPE::TYPE)
            return NewInt(sizeof(Type));
        if (value.type == STANDART_TYPE::NAMESPACE)
            return NewInt(sizeof(Namespace));
        if (value.type == STANDART_TYPE::NULL_T)
            return NewInt(sizeof(Null));
        if (value.type.is_array_type()) {
            return NewInt(any_cast<Array&>(value.data).values.size());
        }

        return NewInt(sizeof(std::any));
    }
};