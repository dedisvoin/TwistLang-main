#include "../twist-nodetemp.cpp"

struct NodeExpressionStatement : public Node { NO_EVAL
    Node* expr;
    NodeExpressionStatement(Node* expr) : expr(expr) {
        this->NODE_TYPE = NodeTypes::NODE_EXPRESSION_STATEMENT;
    }
    void exec_from(Memory& _memory) override {
        expr->eval_from(_memory);
    }
};