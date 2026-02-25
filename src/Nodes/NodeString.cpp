#include "../twist-nodetemp.cpp"

/*
 * NodeString – узел строкового литерала.
 *
 * При создании сразу формирует Value с типом STRING.
 *
 * Поля:
 *   value – готовое Value, содержащее строку.
 */

struct NodeString : public Node { NO_EXEC
    Value value;  // Храним сразу Value

    NodeString(string& val) : value(NewString(std::move(val))) {
        this->NODE_TYPE = NodeTypes::NODE_STRING;
    }

    Value eval_from(Memory& _memory) override {
        return value;
    }
};
