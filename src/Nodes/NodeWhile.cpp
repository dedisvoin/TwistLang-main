#include "../twist-nodetemp.cpp"
#include "NodeBreak.cpp"
#include "NodeContinue.cpp"
#include "../twist-err.cpp"

#define MAX_LSP_ITER_COUNT 300

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
    Node* condition;
    Node* body;

    Token start;
    Token end;

    NodeWhile(Node* eq_expression, Node* condition, Token start, Token end) :
        condition(eq_expression), body(condition), start(start), end(end){
        this->NODE_TYPE = NodeTypes::NODE_WHILE;
        
    }


    void exec_from(Memory* _memory) override {
        #ifndef SERVER
        while (true) {
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
            try {
                body->exec_from(_memory);
            }
            catch (Break) { break; }
            catch (Continue) { continue; }
        }
        #else
        if (condition->NODE_TYPE == NODE_BOOL) {
            if (!any_cast<bool>(condition->eval_from(_memory).data)) {
                ERROR_THROW::UnusedLoopWarning(start, end).Write();
            }
        }
        int i = 0;
        while (true) {
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
            try {
                body->exec_from(_memory);
            }
            catch (Break) { break; }
            catch (Continue) { continue; }

            i++;
            if (i > MAX_LSP_ITER_COUNT) {
                ERROR_THROW::InfinityLoopWarning(start, end).Write();
                break;
            }
        }
        

        #endif
    }
};