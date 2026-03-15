#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"
#include <any>


struct NodeAssert : public Node { NO_EVAL
    Node* expr;
    Node* message_expr;

    Token start_token;
    Token end_token;

    Token message_start;
    Token message_end;

    NodeAssert(Node* expr, Node* message_expr, 
        Token start_token, Token end_token, Token message_start, Token message_end)
        : expr(expr), message_expr(message_expr), 
        start_token(start_token), end_token(end_token), message_start(message_start), message_end(message_end) {
            this->NODE_TYPE = NodeTypes::NODE_ASSERT;
    }

    void exec_from(Memory* _memory) override {
        auto value = expr->eval_from(_memory);
        if (value.type != STANDART_TYPE::BOOL) 
            throw ERROR_THROW::AssertionInvalidArgument(start_token, end_token);
        
        if (!any_cast<bool>(value.data)) {
            if (message_expr) {
                auto message_value = message_expr->eval_from(_memory);
                if (message_value.type != STANDART_TYPE::STRING) 
                    throw ERROR_THROW::AssertionInvalidMessage(message_start, message_end);
                throw ERROR_THROW::AssertionFailed(start_token, end_token, any_cast<string&>(message_value.data));
            }
            throw ERROR_THROW::AssertionFailed(start_token, end_token);
        }
    }
};