#include "../twist-nodetemp.cpp"
#pragma once

/*
 * NodeBreak – оператор break для прерывания циклов.
 *
 * При выполнении генерирует исключение Break, которое перехватывается
 * узлами циклов (NodeWhile, NodeFor, NodeDoWhile) для немедленного выхода.
 *
 * Не содержит дополнительных полей.
 */

struct Break {};

struct NodeBreak : public Node { NO_EVAL
    NodeBreak() {
        this->NODE_TYPE = NodeTypes::NODE_BREAK;
    }

    void exec_from(Memory& _memory) override {
        throw Break();
    }
};