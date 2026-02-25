#include "../twist-nodetemp.cpp"

#include "NodeBreak.cpp"
#include "NodeContinue.cpp"

/*
 * NodeDoWhile – цикл с постусловием (do-while).
 *
 * Выполняет тело цикла один раз, затем проверяет условие.
 * Поддерживает break и continue через исключения.
 *
 * Поля:
 *   eq_expression – выражение-условие.
 *   statement – тело цикла.
 *
 * exec_from() работает в бесконечном цикле:
 *   1. Выполняет тело, перехватывая Break и Continue.
 *   2. Вычисляет условие.
 *   3. Если условие ложно (с учётом преобразования к булеву типу), выходит из цикла.
 * Преобразование условия:
 *   - BOOL: false → выход
 *   - INT: 0 → выход
 *   - DOUBLE: 0.0 → выход
 *   - остальные типы → выход (считаются ложными)
 */

struct NodeDoWhile : public Node { NO_EVAL
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> statement;

    NodeDoWhile(unique_ptr<Node> eq_expression, unique_ptr<Node> statement) :
        eq_expression(std::move(eq_expression)), statement(std::move(statement)) {
        this->NODE_TYPE = NodeTypes::NODE_DO_WHILE;
    }

    void exec_from(Memory& _memory) override {
        while (true) {
            try { statement->exec_from(_memory); }
            catch (Break) { break; }
            catch (Continue) { continue; }

            auto value = eq_expression->eval_from(_memory);
            if (value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }
        }
    }
};