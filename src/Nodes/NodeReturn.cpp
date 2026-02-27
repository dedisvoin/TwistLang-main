#include "../twist-nodetemp.cpp"

#pragma once

struct Return {
    Value value;
    Return(Value value) : value(value) {}
};

struct NodeReturn : public Node { NO_EVAL
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;
    NodeReturn(unique_ptr<Node> expr, Token start_token, Token end_token) : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_RETURN;
    }

    void exec_from(Memory& _memory) override {
        if (!expr) 
            throw Return(NewNull()); 

        auto value = expr->eval_from(_memory);
        throw Return(value);
    }
};