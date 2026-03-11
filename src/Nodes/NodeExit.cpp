#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"

struct NodeExit : public Node { NO_EVAL
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;

    NodeExit(unique_ptr<Node> expr, Token start_token, Token end_token)
        : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_EXIT;
    }

    void exec_from(Memory& _memory) override {
        if (expr) {
            auto value = expr->eval_from(_memory);
            if (value.type != STANDART_TYPE::INT) 
                throw ERROR_THROW::ExitInvalidCode(start_token, end_token, value.type);
            exit(any_cast<int64_t>(value.data));
        } else {
            exit(0);
        }
    }
};