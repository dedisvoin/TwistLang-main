#include "../twist-nodetemp.cpp"
#include "../twist-namespace.cpp"

struct NodeNamespace : public Node { NO_EXEC
    Node* statement = nullptr;


    NodeNamespace(Node* statement) : statement(statement) {
            this->NODE_TYPE = NodeTypes::NODE_NAMESPACE_EXPRESSION;
        }

    Value eval_from(Memory* _memory) override {
        // Используем shared_ptr
        auto name_space_mem = new Memory();
        _memory->link_objects(name_space_mem);

        if (statement) {
            statement->exec_from(name_space_mem);
        }

        auto new_namespace = NewNamespace(name_space_mem, "anonymous-namespace");
        return new_namespace;
    }
};
