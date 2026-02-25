#include "../twist-nodetemp.cpp"
#include "../twist-namespace.cpp"

struct NodeNamespace : public Node { NO_EXEC
    unique_ptr<Node> statement;


    NodeNamespace(unique_ptr<Node> statement) : statement(std::move(statement)) {
            this->NODE_TYPE = NodeTypes::NODE_NAMESPACE_EXPRESSION;
        }

    Value eval_from(Memory& _memory) override {
        // Используем shared_ptr
        auto name_space_mem = make_shared<Memory>();
        _memory.link_objects(*name_space_mem);

        if (statement) {
            statement->exec_from(*name_space_mem.get());
        }

        auto new_namespace = NewNamespace(name_space_mem, "anonymous-namespace");
        return new_namespace;
    }
};
