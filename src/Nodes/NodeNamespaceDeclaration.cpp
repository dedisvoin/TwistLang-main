#include "../twist-nodetemp.cpp"
#include "../twist-namespace.cpp"
#include "../twist-err.cpp"

#pragma once

struct NodeNamespaceDeclaration : public Node { NO_EVAL
    shared_ptr<Memory> namespace_memory;
    Node* statement = nullptr;

    string name;

    Token decl_token;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;
    bool is_private = false;

    NodeNamespaceDeclaration(Node* statement, string name, Token decl_token) :
         statement(statement), name(name), decl_token(decl_token) {
            this->NODE_TYPE = NodeTypes::NODE_NAMESPACE_DECLARATION;
        }

    void exec_from(Memory* _memory) override {
    auto new_namespace_memory = new Memory();
    // Создаём Namespace с этой памятью
    auto new_namespace = NewNamespace(new_namespace_memory, name);
    
    // Добавляем его в свою память (чтобы он был доступен внутри)
    new_namespace_memory->add_object(name, new_namespace, STANDART_TYPE::NAMESPACE, is_const, is_static, is_final, is_global, is_private);
    
    // Линкуем глобальные объекты из родительской памяти
    _memory->link_objects(new_namespace_memory);
    
    if (statement) {
        statement->exec_from(new_namespace_memory);
    }
    
    // Проверяем, не было ли уже объявлено имя namespace
    if (_memory->check_literal(name)) {
        if (_memory->is_final(name)) {
            throw ERROR_THROW::VariableAlreadyDefined(decl_token);
        }
        _memory->delete_variable(name);
    }
    
    // Добавляем namespace в родительскую память
    _memory->add_object(name, new_namespace, STANDART_TYPE::NAMESPACE, is_const, is_static, is_final, is_global, is_private);
}
};