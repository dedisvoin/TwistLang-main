#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"

struct NodeExit : public Node { NO_EVAL
    Node* expr;
    Token start_token;
    Token end_token;

    NodeExit(Node* expr, Token start_token, Token end_token)
        : expr(expr), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_EXIT;
    }

    void exec_from(Memory* _memory) override {
        if (expr) {
            auto value = expr->eval_from(_memory);
            if (value.type != STANDART_TYPE::INT) 
                throw ERROR_THROW::ExitInvalidCode(start_token, end_token, value.type);
            #ifndef SERVER
            exit(any_cast<int64_t>(value.data));
            #else
            ERROR_THROW::ExitWarning(start_token, end_token, any_cast<int64_t>(value.data)).Write();
            #endif
        } else {
            #ifndef SERVER
            exit(0);
            #else
            ERROR_THROW::ExitWarning(start_token, end_token, 0).Write();
            #endif
        }
    }
};