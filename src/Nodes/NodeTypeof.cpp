#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"

struct NodeTypeof : public Node { NO_EXEC
    Node* expr;

    Token expr_token;

    NodeTypeof(Node* expr, Token expr_token) : expr(std::move(expr)), expr_token(expr_token) {
        this->NODE_TYPE = NodeTypes::NODE_TYPEOF;
    }

    Value eval_from(Memory* _memory) override {
        if (!expr)
            throw ERROR_THROW::UnexpectedToken(expr_token, "expression");

        auto value = expr->eval_from(_memory);
        return NewType(value.type);
    }
};