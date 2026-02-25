#include "../twist-nodetemp.cpp"
#include "../twist-array.cpp"
#include "../twist-namespace.cpp"

struct NodeSizeof : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeSizeof(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_SIZEOF;
    }

    Value eval_from(Memory& _memory) override {
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