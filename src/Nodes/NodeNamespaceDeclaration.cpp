#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-namespace.cpp"

struct NodeNamespaceDeclaration : public Node { NO_EVAL
    shared_ptr<Memory> namespace_memory;
    unique_ptr<Node> statement;

    string name;

    Token decl_token;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;
    bool is_private = false;

    NodeNamespaceDeclaration(unique_ptr<Node> statement, string name, Token decl_token) :
         statement(std::move(statement)), name(name), decl_token(decl_token) {
            this->NODE_TYPE = NodeTypes::NODE_NAMESPACE_DECLARATION;
        }

    void exec_from(Memory& _memory) override {
    auto new_namespace_memory = make_shared<Memory>();
    // Создаём Namespace с этой памятью
    auto new_namespace = NewNamespace(new_namespace_memory, name);
    
    // Добавляем его в свою память (чтобы он был доступен внутри)
    new_namespace_memory->add_object(name, new_namespace, STANDART_TYPE::NAMESPACE, is_const, is_static, is_final, is_global, is_private);
    
    // Линкуем глобальные объекты из родительской памяти
    _memory.link_objects(*new_namespace_memory);
    
    if (statement) {
        statement->exec_from(*new_namespace_memory);
    }
    
    // Проверяем, не было ли уже объявлено имя namespace
    if (_memory.check_literal(name)) {
        if (_memory.is_final(name)) {
            ERROR::VariableAlreadyDefined(decl_token, name);
        }
        _memory.delete_variable(name);
    }
    
    // Добавляем namespace в родительскую память
    _memory.add_object(name, new_namespace, STANDART_TYPE::NAMESPACE, is_const, is_static, is_final, is_global, is_private);
}
};