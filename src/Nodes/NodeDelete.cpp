#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "NodeDereference.cpp"
#include "TargetResolver.cpp"

struct NodeDelete : public Node { NO_EVAL
    Node* target;
    Token start_token;
    Token end_token;

    NodeDelete(Node* target, Token start_token, Token end_token)
        : target(target), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_DELETE;
    }

    void exec_from(Memory& _memory) override {
        if (target->NODE_TYPE != NodeTypes::NODE_LITERAL && 
            target->NODE_TYPE != NodeTypes::NODE_NAME_RESOLUTION && 
            target->NODE_TYPE != NodeTypes::NODE_DEREFERENCE) {
            ERROR::InvalidDeleteInstruction(start_token, end_token);
        }

        if (target->NODE_TYPE == NodeTypes::NODE_DEREFERENCE) {
            auto deref_node = static_cast<NodeDereference*>(target);
            auto value = deref_node->expr->eval_from(_memory);
            
            if (!value.type.is_pointer()) {
                ERROR::CanNotDeleteUndereferencedValue(start_token, end_token);
            }

            int address = any_cast<int>(value.data);
            MemoryObject* obj = STATIC_MEMORY.get_by_address(address);
            if (!obj) {
                ERROR::CanNotDeleteUndereferencedValue(start_token, end_token);
            }

            // Если объект принадлежит какой-то памяти и имеет имя, удаляем через владельца
            if (obj->owner && !obj->var_name.empty()) {
                obj->owner->delete_variable(obj->var_name);
            } else {
                // Безымянный объект (создан через new) – просто удаляем из глобальной памяти
                STATIC_MEMORY.unregister_object(address);
                delete obj;
            }
            return;
        } else {
            // Удаление по имени (переменная или пространство имён) – остаётся без изменений
            pair<Memory*, string> target_info = resolveTargetMemory(target, _memory);
            Memory* target_memory = target_info.first;
            string target_name = target_info.second;

            if (!target_memory->check_literal(target_name)) {
                ERROR::UndefinedVariable(start_token);
            }

            if (target_memory->is_private(target_name)) {
                ERROR::PrivateVariableAccess(start_token, end_token, target_name);
            }

            target_memory->delete_variable(target_name);
        }
    }
};