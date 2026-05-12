#include "../twist-nodetemp.cpp"
#include "../twist-structs.cpp"
#include "../twist-err.cpp"

#pragma once

struct NodeStructDeclaration : public Node { NO_EVAL
    Node* body;
    string struct_name;

    Token decl_token;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;
    bool is_private = false;
    bool is_shadow = false;

    NodeStructDeclaration(Node* statement, string name, Token decl_token) :
         body(statement), struct_name(name), decl_token(decl_token) {
            this->NODE_TYPE = NodeTypes::NODE_STRUCT_DECLARATION;
        }

    void exec_from(Memory* _memory) override {
        if (_memory->check_literal(struct_name)) {
            if (_memory->is_final(struct_name)) {
                throw ERROR_THROW::VariableAlreadyDefined(decl_token, struct_name);
            }
            if (_memory->is_global(struct_name) && !_memory->is_shadow(struct_name)) {
                throw ERROR_THROW::VariableShadowsGlobal(decl_token, struct_name);
            }
            // auto addr = _memory->get_variable(struct_name)->address;
            // STATIC_MEMORY.unregister_object(addr);
            // _memory->delete_variable(struct_name);
        }

        auto new_struct_memory = new Memory();
        auto new_struct = NewStruct(struct_name);
        
        // 1. Сначала устанавливаем память у самой структуры
        any_cast<Struct*>(new_struct.data)->memory = new_struct_memory;
        any_cast<Struct*>(new_struct.data)->body = body;
        
        // 2. Теперь добавляем объект в память структуры (копия будет иметь тот же shared_ptr)
        new_struct_memory->add_object_in_struct(struct_name, new_struct, false, false, false, true);

        // 3. Линкуем глобальные объекты
        _memory->link_objects(new_struct_memory);
        
        // 4. Выполняем тело структуры (поля добавляются в new_struct_memory)
        if (body) 
            body->exec_from(new_struct_memory);
        

        
        
        MemoryObject* object = CreateMemoryObject(new_struct, new_struct.type, _memory, is_const, is_static, is_final, is_global, is_private, is_shadow, struct_name, _memory);
        STATIC_MEMORY.register_object(object);
        _memory->add_object(struct_name, object);
    }
};
