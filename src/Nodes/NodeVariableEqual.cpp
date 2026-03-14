#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "TargetResolver.cpp"


struct NodeVariableEqual : public Node { NO_EVAL
    Node* expression;
    Node* variable;

    Token start_left_value_token;
    Token end_left_value_token;

    Token start_value_token;
    Token end_value_token;

    NodeVariableEqual(Node* variable, Node* expression,
                      Token start_left_value_token, Token end_left_value_token,
                      Token start_value_token, Token end_value_token)
        : expression(expression), variable(variable),
          start_left_value_token(start_left_value_token), end_left_value_token(end_left_value_token),
          start_value_token(start_value_token), end_value_token(end_value_token) {
        this->NODE_TYPE = NodeTypes::NODE_VARIABLE_EQUAL;
    }

    void exec_from(Memory& _memory) override {
        auto right_value = expression->eval_from(_memory);

        pair<Memory*, string> target = resolveTargetMemory(variable, _memory);

        Memory* target_memory = target.first;
        string target_var_name = target.second;

        if (target_memory->is_private(target_var_name)) {
            ERROR::PrivateVariableAccess(start_left_value_token, end_value_token, target_var_name);
        }

        if (!target_memory->check_literal(target_var_name))
            ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, target_var_name);

        if (target_memory->is_const(target_var_name)) {
            ERROR::ConstRedefinition(start_left_value_token, end_value_token, target_var_name);
        }

        if (target_memory->is_static(target_var_name)) {
            auto wait_type = target_memory->get_wait_type(target_var_name);
            auto value_type = right_value.type;
            if (!IsTypeCompatible(wait_type, value_type)) {
                ERROR::StaticTypesMisMatch(start_left_value_token, end_value_token, wait_type, value_type);
            }
        }

        target_memory->set_object_value(target_var_name, right_value);
    }

};