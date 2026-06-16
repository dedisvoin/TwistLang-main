#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"

#include "TargetResolver.cpp"

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

struct NodeTypeis : public Node { NO_EXEC
    Node* expr;
    Token start;
    Token end;

    NodeTypeis(Node* expr, Token start_token, Token end_token) : expr(expr), start(start_token), end(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_TYPEIS;
    }

    Value eval_from(Memory* _memory) override {
        auto [mem, name] = resolveTargetMemory(expr, _memory, start, end);

        auto obj = mem->get_variable(name);
        return NewType(obj->wait_type);
    }
};
