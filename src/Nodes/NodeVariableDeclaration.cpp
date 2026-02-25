#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "NodeLiteral.cpp"

/*
 * NodeBaseVariableDecl – узел объявления переменной (let, const, static и т.д.).
 *
 * Вычисляет выражение-инициализатор, обрабатывает модификаторы (const, static, final,
 * global, private), проверяет совместимость типов (если указан статический тип),
 * и регистрирует переменную в текущей памяти и глобальном реестре STATIC_MEMORY.
 *
 * Поля:
 *   var_name – имя переменной.
 *   value_expr – выражение инициализации.
 *   decl_token – токен объявления для сообщений об ошибках.
 *   type_expr – опциональное выражение типа (может быть "auto").
 *   type_start_token, type_end_token – границы выражения типа.
 *   start_expr_token, end_expr_token – границы выражения инициализации.
 *   nullable – флаг, указывающий, что тип может быть null (добавляет NULL_T к типу).
 *   модификаторы (is_static, is_final, is_const, is_global, is_private).
 *
 * exec_from():
 *   1. Вычисляет значение инициализатора.
 *   2. Если переменная уже существует и является final – ошибка.
 *   3. Если есть статический тип (is_static), проверяет совместимость с вычисленным значением.
 *   4. Создаёт MemoryObject с нужными модификаторами.
 *   5. Регистрирует объект в STATIC_MEMORY и добавляет в текущую память.
 */

struct NodeBaseVariableDecl : public Node { NO_EVAL
    string var_name;
    unique_ptr<Node> value_expr;
    Token decl_token;
    unique_ptr<Node> type_expr;
    Token type_start_token;
    Token type_end_token;

    Token start_expr_token;
    Token end_expr_token;

    bool nullable = false;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;
    bool is_private = false;

    NodeBaseVariableDecl(const string& name, unique_ptr<Node> expr, Token decl_token, unique_ptr<Node> type_expr,
                            Token type_start_token, Token type_end_token, bool nullable, Token start_expr_token, Token end_expr_token)
        : var_name(name), decl_token(decl_token), type_start_token(type_start_token), type_end_token(type_end_token),
        nullable(nullable), start_expr_token(start_expr_token), end_expr_token(end_expr_token) {
            this->NODE_TYPE = NodeTypes::NODE_VARIABLE_DECLARATION;
            this->value_expr = std::move(expr);
            this->type_expr = std::move(type_expr);
        }

    void exec_from(Memory& _memory) override {
        Value value = value_expr->eval_from(_memory);

        if (_memory.check_literal(var_name)) {
            if (_memory.is_final(var_name)) {
                ERROR::VariableAlreadyDefined(decl_token, var_name);
            }
            auto addr = _memory.get_variable(var_name)->address;
            STATIC_MEMORY.unregister_object(addr);
            _memory.delete_variable(var_name);

        }

        Type static_type = value.type;
        if (is_static) {
            if (!type_expr) ERROR::WaitedTypeExpression(type_end_token);

            if (type_expr->NODE_TYPE == NodeTypes::NODE_LITERAL && ((NodeLiteral*)type_expr.get())->name == "auto") {
                static_type = value.type;
            } else {
                auto type_value = type_expr->eval_from(_memory);

                if (type_value.type != STANDART_TYPE::TYPE)
                    ERROR::InvalidType(type_start_token, type_end_token);

                static_type = any_cast<Type>(type_value.data);
                if (nullable)
                    static_type = static_type | STANDART_TYPE::NULL_T;

            }

            // Используем IsTypeCompatible вместо прямого сравнения
            if (!IsTypeCompatible(static_type, value.type)) {
                ERROR::IncompartableTypeVarDeclaration(type_start_token, type_end_token, start_expr_token, end_expr_token, static_type, value.type);
            }
        }

        MemoryObject* object = CreateMemoryObject(value, static_type, &_memory, is_const, is_static, is_final, is_global, is_private);
        STATIC_MEMORY.register_object(object);
        _memory.add_object(var_name, object);
    }
};
