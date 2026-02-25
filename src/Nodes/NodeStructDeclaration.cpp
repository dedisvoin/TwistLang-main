#include "../twist-nodetemp.cpp"
#include "../twist-structs.cpp"
#include "../twist-errors.cpp"

#pragma once

struct NodeStructDeclaration : public Node { NO_EVAL
    unique_ptr<Node> statement;
    string name;

    Token decl_token;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = true;
    bool is_private = false;

    NodeStructDeclaration(unique_ptr<Node> statement, string name, Token decl_token) :
         statement(std::move(statement)), name(name), decl_token(decl_token) {
            this->NODE_TYPE = NodeTypes::NODE_STRUCT_DECLARATION;
        }

    void exec_from(Memory& _memory) override {
        auto new_struct_memory = make_shared<Memory>();
        auto new_struct = NewStruct(name);
        
        // 1. Сначала устанавливаем память у самой структуры
        any_cast<Struct&>(new_struct.data).memory = new_struct_memory;
        
        // 2. Теперь добавляем объект в память структуры (копия будет иметь тот же shared_ptr)
        new_struct_memory->add_object_in_struct(name, new_struct, false, false, false, true);

        // 3. Линкуем глобальные объекты
        _memory.link_objects(*new_struct_memory);
        
        // 4. Выполняем тело структуры (поля добавляются в new_struct_memory)
        if (statement) {
            statement->exec_from(*new_struct_memory);
        }

        // 5. Проверяем, не было ли уже объявлено имя структуры
        if (_memory.check_literal(name)) {
            if (_memory.is_final(name)) {
                ERROR::VariableAlreadyDefined(decl_token, name);
            }
            _memory.delete_variable(name);
        }
        
        // 6. Регистрируем структуру в родительской памяти
        _memory.add_object(name, new_struct, new_struct.type, is_const, is_static, is_final, is_global, is_private);
    }
};
