#include "../twist-nodetemp.cpp"

#include "NodeBreak.cpp"
#include "NodeContinue.cpp"

/*
 * NodeFor – цикл со счётчиком (for).
 *
 * Состоит из инициализации, условия, шага и тела.
 * Поддерживает break и continue.
 *
 * Поля:
 *   start_state – инициализация (выполняется один раз в начале).
 *   check_expr – условие продолжения (вычисляется перед каждой итерацией).
 *   update_state – шаг (выполняется после тела, но перед continue).
 *   body – тело цикла.
 *
 * Порядок выполнения:
 *   1. start_state->exec_from()
 *   2. Пока условие истинно (с преобразованием к булеву типу):
 *      a. body->exec_from() с обработкой исключений:
 *         - Break – выход из цикла.
 *         - Continue – выполнение update_state и переход к следующей итерации.
 *      b. Если не было continue, выполняется update_state.
 *   3. Условие вычисляется по тем же правилам, что и в NodeWhile.
 */

struct NodeFor : public Node { NO_EVAL
    unique_ptr<Node> start_state;
    unique_ptr<Node> check_expr;
    unique_ptr<Node> update_state;
    unique_ptr<Node> body;

    NodeFor(unique_ptr<Node> start_state, unique_ptr<Node> check_expr, unique_ptr<Node> update_state, unique_ptr<Node> body) :
        start_state(std::move(start_state)), check_expr(std::move(check_expr)),
        update_state(std::move(update_state)), body(std::move(body)) {
        this->NODE_TYPE = NodeTypes::NODE_FOR;
    }

    void exec_from(Memory& _memory) override {
        start_state->exec_from(_memory);

        while (true) {
            auto value = check_expr->eval_from(_memory);
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
                body->exec_from(_memory);
            }
            catch (Break) { break; }
            catch (Continue) {
                // Пропускаем оставшуюся часть итерации
                // но ВСЕГДА выполняем update_state перед continue
                update_state->exec_from(_memory);
                continue;
            }

            // Нормальное выполнение (без continue)
            update_state->exec_from(_memory);


        }
    }
};