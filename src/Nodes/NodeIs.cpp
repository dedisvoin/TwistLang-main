#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"
#include "TargetResolver.cpp"

struct NodeIs : public Node { NO_EXEC
    Node* expr;
    Token start_token;
    Token end_token;
    Token start_expr;
    Token end_expr;
    string modifier;

    NodeIs(Node* expr, string modifier, Token start_token, Token end_token, Token start_expr, Token end_expr)
        : expr(expr), start_token(start_token), end_token(end_token), modifier(modifier), start_expr(start_expr), end_expr(end_expr) {
        this->NODE_TYPE = NodeTypes::NODE_IS;
    }

    Value eval_from(Memory* _memory) override {
        auto [mem, name] = resolveTargetMemory(expr, _memory, start_expr, end_expr);
        if (modifier == "const")
            return NewBool(mem->get_variable(name)->modifiers.is_const);
        else if (modifier == "static")
            return NewBool(mem->get_variable(name)->modifiers.is_static);
        else if (modifier == "final")
            return NewBool(mem->get_variable(name)->modifiers.is_final);
        else if (modifier == "global")
            return NewBool(mem->get_variable(name)->modifiers.is_global);
        else if (modifier == "private")
            return NewBool(mem->get_variable(name)->modifiers.is_private);
        else if (modifier == "shadow")
            return NewBool(mem->get_variable(name)->modifiers.is_shadow);
        else
            throw ERROR_THROW::InvalidModifierTypeToIs(start_token, end_token, modifier);
    }
};
