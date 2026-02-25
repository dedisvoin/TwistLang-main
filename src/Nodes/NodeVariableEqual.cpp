#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "NodeLiteral.cpp"
#include "NodeNamespaceResolution.cpp"
#include "NodeObjectResolution.cpp"

struct NodeVariableEqual : public Node { NO_EVAL
    unique_ptr<Node> expression;
    unique_ptr<Node> variable;

    Token start_left_value_token;
    Token end_left_value_token;

    Token start_value_token;
    Token end_value_token;

    NodeVariableEqual(unique_ptr<Node> variable, unique_ptr<Node> expression, Token start_left_value_token, Token end_left_value_token,
                        Token start_value_token, Token end_value_token)  :
        expression(std::move(expression)), variable(std::move(variable)),
        start_left_value_token(start_left_value_token), end_left_value_token(end_left_value_token),
        start_value_token(start_value_token), end_value_token(end_value_token) {
            this->NODE_TYPE = NodeTypes::NODE_VARIABLE_EQUAL;
        };

    void exec_from(Memory& _memory) override {

        auto right_value = expression->eval_from(_memory);

        // НЕ перемещаем variable, используем сырой указатель

        Node* variable_ptr = variable.get();

        // Получаем целевую память и имя переменной - используем ССЫЛКУ вместо копии!
        pair<Memory*, string> target = resolveAssignmentTargetMemory(variable_ptr, _memory);

        Memory* target_memory = target.first;
        string target_var_name = target.second;



        if (target_memory->is_private(target_var_name)) {
            ERROR::PrivateVariableAccess(start_left_value_token, end_value_token, target_var_name);
        }


        if (!target_memory->check_literal(target_var_name))
            ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, target_var_name);

        // Проверяем константность
        if (target_memory->is_const(target_var_name)) {
            ERROR::ConstRedefinition(start_left_value_token, end_value_token, target_var_name);
        }

        
        // Проверяем типизацию для статических переменных
        if (target_memory->is_static(target_var_name)) {
            
            auto wait_type = target_memory->get_wait_type(target_var_name);
            auto value_type = right_value.type;
            if (!IsTypeCompatible(wait_type, value_type)) {
                ERROR::StaticTypesMisMatch(start_left_value_token, end_value_token, wait_type, value_type);
            }
        }



        // Выполняем присваивание
        target_memory->set_object_value(target_var_name, right_value);
    }

    pair<Memory*, string> resolveAssignmentTargetMemory(Node* node, Memory& _memory) {
        if (node->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            if (!_memory.check_literal(((NodeLiteral*)node)->name)) {
                ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, ((NodeLiteral*)node)->name);
            }
            return {&_memory, ((NodeLiteral*)node)->name};
        }
        else if (node->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            NodeNamespaceResolution* resolution = (NodeNamespaceResolution*)node;

            // Получаем namespace
            Value ns_value = resolution->namespace_expr->eval_from(_memory);

            if (ns_value.type != STANDART_TYPE::NAMESPACE) {
                ERROR::InvalidAccessorType(resolution->start, resolution->end, ns_value.type.pool);
            }

            // ИСПРАВЛЕНО: используем ссылку вместо копии
            auto& ns = any_cast<Namespace&>(ns_value.data);

            // Получаем указатель на память из shared_ptr
            Memory* current_memory = ns.memory.get();

            // Строим полную цепочку имен
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            // Проходим по цепочке, если она есть
            if (full_chain.size() > 1) {
                // Ищем конечный namespace через цепочку
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& name = full_chain[i];

                    if (!current_memory->check_literal(name)) {
                        ERROR::UndefinedVariableInNamespace(name, "current namespace");
                    }

                    MemoryObject* obj = current_memory->get_variable(name);

                    // Проверяем приватность
                    if (obj->modifiers.is_private)
                        ERROR::PrivateVariableAccess(start_left_value_token, end_left_value_token, name);

                    if (obj->value.type != STANDART_TYPE::NAMESPACE) {
                        ERROR::InvalidNamespaceChainType(resolution->start, resolution->end, name, obj->value.type.pool);
                    }

                    // Переходим к следующему namespace
                    auto& next_ns = any_cast<Namespace&>(obj->value.data);
                    current_memory = next_ns.memory.get();
                }
            }

            // Возвращаем память и имя последнего элемента
            return {current_memory, full_chain.back()};
        }
        else if (node->NODE_TYPE == NodeTypes::NODE_OBJECT_RESOLUTION) {
            NodeObjectResolution* resolution = (NodeObjectResolution*)node;

            // Получаем значение объекта (структуры)
            Value obj_value = resolution->obj_expr->eval_from(_memory);

            // Проверяем, что это структура (не стандартный тип)
            if (STANDART_TYPE::UNTYPED.is_sub_type(obj_value.type)) {
                ERROR::InvalidAccessorType(resolution->start, resolution->end, obj_value.type.pool);
            }

            // Получаем ссылку на структуру
            auto& obj = any_cast<Struct&>(obj_value.data);

            // Получаем указатель на память
            Memory* current_memory = obj.memory.get();

            // Строим полную цепочку имён
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            // Проходим по цепочке, если она есть
            if (full_chain.size() > 1) {
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& name = full_chain[i];

                    if (!current_memory->check_literal(name)) {
                        ERROR::UndefinedFieldInObject(resolution->start, resolution->end, name, obj.type.pool);
                    }

                    MemoryObject* field_obj = current_memory->get_variable(name);

                    // Проверяем приватность
                    if (field_obj->modifiers.is_private)
                        ERROR::PrivatePropertyAccess(resolution->start, resolution->end, name);
                    

                    Value field_value = field_obj->value;

                    // Проверяем, что поле является структурой (для продолжения цепочки)
                    // Проверяем приватность
                    if (field_obj->modifiers.is_private)
                        ERROR::PrivateVariableAccess(start_left_value_token, end_left_value_token, name);

                    if (field_obj->value.type != STANDART_TYPE::NAMESPACE) {
                        ERROR::InvalidNamespaceChainType(resolution->start, resolution->end, name, field_obj->value.type.pool);
                    }

                    auto& next_obj = any_cast<Struct&>(field_value.data);
                    current_memory = next_obj.memory.get();
                }
            }

            // Возвращаем память и имя последнего поля
            return {current_memory, full_chain.back()};
        }

        // Ошибка для неизвестного типа ноды
        cout << "INTERNAL ERROR: Unknown assignment target node type" << endl;
        exit(0);
    }
};