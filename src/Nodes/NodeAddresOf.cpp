#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "NodeLiteral.cpp"
#include "NodeNamespaceResolution.cpp"
#include "NodeScopes.cpp"


struct NodeAddressOf : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeAddressOf(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_ADDRESS_OF;
    }

    pair<Memory*, string> resolveAssignmentTargetMemory(Node* node, Memory& _memory) {
        if (node->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            if (!_memory.check_literal(((NodeLiteral*)node)->name)) {
                cout << "ERROR: Undefined variable '" << ((NodeLiteral*)node)->name << "' for address-of operation" << endl;
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
        cout << "INTERNAL ERROR: Unknown node type in address-of operation" << endl;
        exit(0);
    }

    Value eval_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        if (expr->NODE_TYPE == NodeTypes::NODE_SCOPES) {
            while (expr->NODE_TYPE == NodeTypes::NODE_SCOPES)
                expr = std::move(((NodeScopes*)(expr.get()))->expression);
        }
        if (expr->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            auto var_name = ((NodeLiteral*)(expr.get()))->name;
            auto addr = _memory.get_variable(var_name)->address;
            return NewPointer(addr, value.type);
        }
        if (expr->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            auto [memory, var_name] = resolveAssignmentTargetMemory(expr.get(), _memory);
            auto addr = memory->get_variable(var_name)->address;
            return NewPointer(addr, value.type);
        }
    }
};