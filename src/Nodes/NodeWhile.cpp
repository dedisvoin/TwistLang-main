#include "../twist-nodetemp.cpp"

#include "NodeBreak.cpp"
#include "NodeContinue.cpp"

/*
 * NodeWhile – цикл с предусловием (while).
 *
 * Выполняет тело цикла, пока условие истинно.
 * Поддерживает break и continue через исключения.
 *
 * Поля:
 *   eq_expression – выражение-условие.
 *   statement – тело цикла.
 *
 * exec_from():
 *   В бесконечном цикле:
 *     1. Вычисляет условие, преобразует к булеву значению.
 *     2. Если условие ложно – выход.
 *     3. Выполняет тело с перехватом Break и Continue.
 *   Преобразование условия аналогично NodeDoWhile.
 */

struct NodeWhile : public Node { NO_EVAL
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> statement;

    NodeWhile(unique_ptr<Node> eq_expression, unique_ptr<Node> statement) :
        eq_expression(std::move(eq_expression)), statement(std::move(statement)) {
        this->NODE_TYPE = NodeTypes::NODE_WHILE;
    }


    void exec_from(Memory& _memory) override {
        while (true) {
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

            try {
                statement->exec_from(_memory);
            }
            catch (Break) { break; }
            catch (Continue) { continue; }
        }
    }
};