#include "../twist-nodetemp.cpp"
#pragma once

/*
 * NodeContinue – оператор continue для перехода к следующей итерации цикла.
 *
 * При выполнении генерирует исключение Continue, которое перехватывается
 * узлами циклов для немедленного перехода к проверке условия или обновлению.
 */

struct Continue {};

struct NodeContinue : public Node { NO_EVAL
    NodeContinue() {
        this->NODE_TYPE = NodeTypes::NODE_CONTINUE;
    }

    void exec_from(Memory& _memory) override {
        throw Continue();
    }
};