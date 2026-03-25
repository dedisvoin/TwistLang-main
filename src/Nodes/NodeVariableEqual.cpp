#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"
#include "NodeDereference.cpp"
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

    void exec_from(Memory* _memory) override {
        auto right_value = expression->eval_from(_memory);
        
        
        
        if (variable->NODE_TYPE == NODE_DEREFERENCE) {
            auto left_value = ((NodeDereference*)variable)->expr->eval_from(_memory);
            auto address = any_cast<int>(left_value.data);

            if (!STATIC_MEMORY.is_registered(address)){
                cout << "ERROR ADDR " << address << endl;
                exit(-1);
            }
    
            auto object = STATIC_MEMORY.get_by_address(address);
            auto modifiers = object->modifiers;
            
            // Проверяем константность
            if (modifiers.is_const) 
                throw ERROR_THROW::PointerToConstRedefinition(start_left_value_token, end_left_value_token);
            

            // Проверяем типизацию для статических переменных
            if (modifiers.is_static) {
                auto wait_type = object->wait_type;
                auto value_type = right_value.type;
                if (!IsTypeCompatible(wait_type, value_type)) {
                    throw ERROR_THROW::VariableStaticTypesMisMatch(start_left_value_token, end_value_token, wait_type, value_type);
                }
            }

            // Выполняем присваивание
            if (STATIC_MEMORY.is_registered(address)) {
                STATIC_MEMORY.set_object_value(address, right_value);
            }
            return;
        }

        pair<Memory*, string> target = resolveTargetMemory(variable, _memory);

        Memory* target_memory = target.first;
        string target_var_name = target.second;

        if (target_memory->is_private(target_var_name)) 
            throw ERROR_THROW::PrivateVariableAccess(start_left_value_token, end_value_token, target_var_name);
        

        if (!target_memory->check_literal(target_var_name))
            throw ERROR_THROW::VariableUndefined(start_left_value_token, end_left_value_token, target_var_name);

        if (target_memory->is_const(target_var_name)) 
            throw ERROR_THROW::VariableConstRedefinition(start_left_value_token, end_value_token, target_var_name);
        

        if (target_memory->is_static(target_var_name)) {
            auto wait_type = target_memory->get_wait_type(target_var_name);
            auto value_type = right_value.type;
            if (!IsTypeCompatible(wait_type, value_type)) 
                throw ERROR_THROW::VariableStaticTypesMisMatch(start_left_value_token, end_value_token, wait_type, value_type);
            
        }

        target_memory->set_object_value(target_var_name, right_value);
    }

};