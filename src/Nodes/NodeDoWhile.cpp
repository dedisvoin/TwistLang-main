#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"

#include "NodeBreak.cpp"
#include "NodeContinue.cpp"

#define MAX_LSP_ITER_COUNT 300

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
    Node* condition;
    Node* body;

    Token start;
    Token end;

    NodeDoWhile(Node* condition, Node* body, Token start, Token end) :
        condition(condition), body(body), start(start), end(end) {
        this->NODE_TYPE = NodeTypes::NODE_DO_WHILE;
    }

    void exec_from(Memory* _memory) override {
        #ifndef SERVER
        while (true) {
            try { body->exec_from(_memory); }
            catch (Break) { break; }
            catch (Continue) { continue; }

            if (condition) {
                auto value = condition->eval_from(_memory);
                if (value.type == STANDART_TYPE::BOOL) {
                    if (any_cast<bool>(value.data) == false)
                        break;
                } else if (value.type == STANDART_TYPE::INT) {
                    if (any_cast<int64_t>(value.data) == 0)
                        break;
                } else if (value.type == STANDART_TYPE::DOUBLE) {
                    if (any_cast<NUMBER_ACCURACY>(value.data) == 0)
                        break;
                } else {
                    break;
                }
            }
        }
        #else
        if (condition->NODE_TYPE == NODE_BOOL) {
            if (!any_cast<bool>(condition->eval_from(_memory).data)) {
                ERROR_THROW::UnusedLoopWarning(start, end).Write();
            }
        }
        int i = 0;
        while (true) {
            i++;
            if (i > MAX_LSP_ITER_COUNT) {
                ERROR_THROW::InfinityLoopWarning(start, end).Write();
                break;
            }
            try { body->exec_from(_memory); }
            catch (Break) { break; }
            catch (Continue) { continue; }

            if (condition) {
                auto value = condition->eval_from(_memory);
                if (value.type == STANDART_TYPE::BOOL) {
                    if (any_cast<bool>(value.data) == false)
                        break;
                } else if (value.type == STANDART_TYPE::INT) {
                    if (any_cast<int64_t>(value.data) == 0)
                        break;
                } else if (value.type == STANDART_TYPE::DOUBLE) {
                    if (any_cast<NUMBER_ACCURACY>(value.data) == 0)
                        break;
                } else {
                    break;
                }
            }
        }
        #endif
    }
};