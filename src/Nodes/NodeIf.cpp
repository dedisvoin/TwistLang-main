#include "../twist-nodetemp.cpp"

/*
 * NodeIf – условный оператор if-else.
 *
 * Вычисляет выражение-условие, преобразует его к булеву значению и выполняет
 * соответствующую ветку.
 *
 * Поля:
 *   eq_expression – условие.
 *   true_statement – ветка then.
 *   else_statement – опциональная ветка else.
 *
 * Преобразование условия:
 *   - BOOL: непосредственно значение.
 *   - INT: ноль → false, ненулевое → true.
 *   - DOUBLE: 0.0 → false, иначе → true.
 *   - STRING: пустая строка → false, иначе → true.
 *   - CHAR: '\0' → false, иначе → true.
 *   - NULL_T: false.
 *   - POINTER: нулевой адрес → false, иначе → true.
 *   - остальные типы (Namespace, Type, Array, Function, Lambda) считаются true.
 */

struct NodeIf : public Node { NO_EVAL
    unique_ptr<Node> condition;
    unique_ptr<Node> true_statement;
    unique_ptr<Node> else_statement = nullptr;

    NodeIf(unique_ptr<Node> condition, unique_ptr<Node> true_statement, unique_ptr<Node> else_statement = nullptr) :
        condition(std::move(condition)), true_statement(std::move(true_statement)),
        else_statement(std::move(else_statement)) {
        this->NODE_TYPE = NodeTypes::NODE_IF;
    }

    void exec_from(Memory& _memory) override {
        auto value = condition->eval_from(_memory);

        bool condition = false;

        // Преобразование значения к булеву типу
        if (value.type == STANDART_TYPE::BOOL) {
            condition = any_cast<bool>(value.data);
        }
        else if (value.type == STANDART_TYPE::INT) {
            condition = any_cast<int64_t>(value.data) != 0;
        }
        else if (value.type == STANDART_TYPE::DOUBLE) {
            condition = any_cast<float>(value.data) != 0.0f;
        }
        else if (value.type == STANDART_TYPE::STRING) {
            condition = !any_cast<string>(value.data).empty();
        }
        else if (value.type == STANDART_TYPE::CHAR) {
            condition = any_cast<char>(value.data) != '\0';
        }
        else if (value.type == STANDART_TYPE::NULL_T) {
            condition = false;
        }
        else if (value.type.is_pointer()) {
            int address = any_cast<int>(value.data);
            condition = address != 0;
        }
        else {
            // Для остальных типов (Namespace, Type, Array, Function, Lambda) считаем истиной
            condition = true;
        }

        if (condition) {
            true_statement->exec_from(_memory);
        }
        else if (else_statement) {
            else_statement->exec_from(_memory);
        }
    }
};


struct NodeIfExpr : public Node { NO_EXEC
    unique_ptr<Node> condition;
    unique_ptr<Node> true_expr;
    unique_ptr<Node> else_expr;

    NodeIfExpr(unique_ptr<Node> condition, unique_ptr<Node> true_expr, unique_ptr<Node> else_expr)
        : condition(std::move(condition)), true_expr(std::move(true_expr)), else_expr(std::move(else_expr)) {
            this->NODE_TYPE = NodeTypes::NODE_IF_EXPRESSION;
    }

    Value eval_from(Memory& _memory) override {
        auto value = condition->eval_from(_memory);

        bool condition = false;

        // Преобразование значения к булеву типу
        if (value.type == STANDART_TYPE::BOOL) {
            condition = any_cast<bool>(value.data);
        }
        else if (value.type == STANDART_TYPE::INT) {
            condition = any_cast<int64_t>(value.data) != 0;
        }
        else if (value.type == STANDART_TYPE::DOUBLE) {
            condition = any_cast<float>(value.data) != 0.0f;
        }
        else if (value.type == STANDART_TYPE::STRING) {
            condition = !any_cast<string>(value.data).empty();
        }
        else if (value.type == STANDART_TYPE::CHAR) {
            condition = any_cast<char>(value.data) != '\0';
        }
        else if (value.type == STANDART_TYPE::NULL_T) {
            condition = false;
        }
        else if (value.type.is_pointer()) {
            int address = any_cast<int>(value.data);
            condition = address != 0;
        }
        else {
            // Для остальных типов (Namespace, Type, Array, Function, Lambda) считаем истиной
            condition = true;
        }

        if (condition) {
            return true_expr->eval_from(_memory);
        }
        else if (else_expr) {
            return else_expr->eval_from(_memory);
        }
        return NewNull();
    }
};