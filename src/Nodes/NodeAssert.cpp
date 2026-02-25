#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

struct NodeAssert : public Node { NO_EVAL
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;

    NodeAssert(unique_ptr<Node> expr, Token start_token, Token end_token)
        : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
            this->NODE_TYPE = NodeTypes::NODE_ASSERT;
    }

    void exec_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        if (value.type != STANDART_TYPE::BOOL) {
            ERROR::AssertionIvalidArgument(start_token, end_token);
        }
        if (!any_cast<bool>(value.data)) {
            ERROR::AssertionFailed(start_token, end_token);
        }
    }
};