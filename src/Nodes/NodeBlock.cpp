#include "../twist-nodetemp.cpp"

/*
 * NodeBlock – блок последовательных инструкций.
 *
 * Содержит вектор дочерних узлов, которые выполняются друг за другом.
 * Используется для группировки нескольких операторов в теле функций, циклов,
 * условных конструкций и т.д.
 *
 * Поля:
 *   nodes_array – вектор уникальных указателей на узлы, составляющие блок.
 *
 * exec_from() последовательно вызывает exec_from() для каждого дочернего узла.
 */

struct NodeBlock : public Node { NO_EVAL
    vector<Node*> nodes_array;

    NodeBlock(vector<Node*> &nodes_array) : nodes_array(nodes_array) {
        this->NODE_TYPE = NodeTypes::NODE_BLOCK_OF_NODES;
    }

    void exec_from(Memory* _memory) override {
        for (int i = 0; i < nodes_array.size(); i++) {
            nodes_array[i]->exec_from(_memory);
        }
    }
};