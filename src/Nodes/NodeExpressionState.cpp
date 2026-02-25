#include "../twist-nodetemp.cpp"

struct NodeExpressionStatement : public Node { NO_EVAL
    unique_ptr<Node> expr;
    NodeExpressionStatement(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_EXPRESSION_STATEMENT;
    }
    void exec_from(Memory& _memory) override {
        expr->eval_from(_memory);
    }
};