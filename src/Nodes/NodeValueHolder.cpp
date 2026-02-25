#include "../twist-nodetemp.cpp"
#pragma once

/*
 * NodeValueHolder – вспомогательный узел, хранящий готовое значение.
 *
 * Используется в основном для передачи значений по цепочке разрешения имён
 * (например, при рекурсивном разрешении). Просто возвращает сохранённое value.
 *
 * Поля:
 *   value – готовое Value.
 */

struct NodeValueHolder : public Node { NO_EXEC
    Value value;

    NodeValueHolder(Value val) : value(std::move(val)) {
        this->NODE_TYPE = NodeTypes::NODE_VALUE_HOLDER;
    }

    Value eval_from(Memory& _memory) override {
        return value;
    }
};
