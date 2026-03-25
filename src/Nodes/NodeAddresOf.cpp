#include "../twist-nodetemp.cpp"

#include "NodeLiteral.cpp"
#include "NodeScopes.cpp"
#include "TargetResolver.cpp"

struct NodeAddressOf : public Node { NO_EXEC
    Node* expr;
    Token start, end;

    NodeAddressOf(Node* expr, Token start, Token end) : expr(expr), start(start), end(end) {
        this->NODE_TYPE = NodeTypes::NODE_ADDRESS_OF;
    }

    
    Value eval_from(Memory* _memory) override {
        // Сначала вычислим выражение, чтобы получить его тип (нужен для указателя)
        Value val = expr->eval_from(_memory);

        while (expr->NODE_TYPE == NodeTypes::NODE_SCOPES) {
            expr = static_cast<NodeScopes*>(expr)->expression;
        }

        // Обрабатываем литерал (простая переменная)
        if (expr->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            NodeLiteral* lit = static_cast<NodeLiteral*>(expr);
            int addr = _memory->get_variable(lit->name)->address;
            return NewPointer(addr, val.type);
        }
        // Обрабатываем разрешение имени (namespace::var)
        else if (expr->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            auto [mem, var_name] = resolveTargetMemory(expr, _memory);
            int addr = mem->get_variable(var_name)->address;
            return NewPointer(addr, val.type);
        }

        throw ERROR_THROW::CanNotGetAddress(start, end, expr->NODE_TYPE);
    }
};