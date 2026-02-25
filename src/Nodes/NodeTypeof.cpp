#include "../twist-nodetemp.cpp"

struct NodeTypeof : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeTypeof(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_TYPEOF;
    }

    Value eval_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        return NewType(value.type);
    }
};