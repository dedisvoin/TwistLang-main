#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "NodeLiteral.cpp"
#include "NodeNamespaceResolution.cpp"
#include "NodeDereference.cpp"

struct NodeDelete : public Node { NO_EVAL

    unique_ptr<Node> target;
    Token start_token;
    Token end_token;

    NodeDelete(unique_ptr<Node> target, Token start_token, Token end_token)
        : target(std::move(target)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_DELETE;
    }

    void exec_from(Memory& _memory) override {
        if (target->NODE_TYPE != NodeTypes::NODE_LITERAL && target->NODE_TYPE != NodeTypes::NODE_NAME_RESOLUTION && target->NODE_TYPE != NodeTypes::NODE_DEREFERENCE) {
            ERROR::InvalidDeleteInstruction(start_token, end_token);
        }

        if (target->NODE_TYPE == NodeTypes::NODE_DEREFERENCE) {
            auto value = ((NodeDereference*)(target.get()))->expr->eval_from(_memory);
            if (value.type.is_pointer()) {
                auto address = any_cast<int>(value.data);
                if (STATIC_MEMORY.is_registered(address)) {
                    STATIC_MEMORY.unregister_object(address);
                    return;
                }
            }
            ERROR::CanNotDeleteUndereferencedValue(start_token, end_token);
        } else {
            pair<Memory*, string> target_info = resolveAssignmentTargetMemory(target.get(), _memory);
            Memory* target_memory = target_info.first;
            string target_name = target_info.second;

            if (!target_memory->check_literal(target_name)) {
                ERROR::UndefinedVariable(start_token);
            }

            // Выполняем удаление
            auto object = target_memory->get_variable(target_name);
            target_memory->delete_variable(target_name);
            STATIC_MEMORY.unregister_object(object->address);
        }
    }

    pair<Memory*, string> resolveAssignmentTargetMemory(Node* node, Memory& _memory) {
        if (node->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            if (!_memory.check_literal(((NodeLiteral*)node)->name)) {
                cout << "ERROR: Undefined variable '" << ((NodeLiteral*)node)->name << "' for delete operation" << endl;
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
                        ERROR::PrivatePropertyAccess(resolution->start, resolution->end, name);

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

        // Ошибка для неизвестного типа ноды
        ERROR::InvalidDeleteTarget(start_token, end_token);
        exit(0);
    }
};
