#include "../twist-nodetemp.cpp"

/*
 * NodeNull – узел литерала null.
 *
 * Возвращает глобальное значение Null, создаваемое один раз.
 *
 * Поле:
 *   (нет)
 *
 * eval_from() возвращает предопределённое ValueNull.
 */

auto ValueNull = NewNull();
struct NodeNull : public Node { NO_EXEC
    NodeNull() {
        this->NODE_TYPE = NodeTypes::NODE_NULL;
    }

    Value eval_from(Memory& _memory) override {
        return ValueNull;
    }
};
