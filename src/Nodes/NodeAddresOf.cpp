#include "../twist-nodetemp.cpp"

#include "NodeLiteral.cpp"
#include "NodeScopes.cpp"
#include "TargetResolver.cpp"

struct NodeAddressOf : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeAddressOf(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_ADDRESS_OF;
    }

    
    Value eval_from(Memory& _memory) override {
        // Сначала вычислим выражение, чтобы получить его тип (нужен для указателя)
        Value val = expr->eval_from(_memory);

        // Раскрываем скобки, если они есть
        Node* effective = expr.get();
        while (effective->NODE_TYPE == NodeTypes::NODE_SCOPES) {
            effective = static_cast<NodeScopes*>(effective)->expression.get();
        }

        // Обрабатываем литерал (простая переменная)
        if (effective->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            NodeLiteral* lit = static_cast<NodeLiteral*>(effective);
            int addr = _memory.get_variable(lit->name)->address;
            return NewPointer(addr, val.type);
        }
        // Обрабатываем разрешение имени (namespace::var)
        else if (effective->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            auto [mem, var_name] = resolveTargetMemory(effective, _memory);
            int addr = mem->get_variable(var_name)->address;
            return NewPointer(addr, val.type);
        }

        
    }
};