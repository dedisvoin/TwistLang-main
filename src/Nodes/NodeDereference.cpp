#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "../twist-structs.cpp"

#pragma once

struct NodeDereference : public Node { NO_EXEC
    Node* expr;
    Token start;
    Token end;

    NodeDereference(Node* expr, Token start, Token end) : expr(expr), start(start), end(end) {
        this->NODE_TYPE = NodeTypes::NODE_DEREFERENCE;
    }

    Value eval_from(Memory* _memory) override {
        auto value = expr->eval_from(_memory);
        if (value.type.is_pointer()) {
            auto object = STATIC_MEMORY.get_by_address(any_cast<int>(value.data));
            if (!object)
                return NewNull();
            return object->value;
        }
        if (value.type == STANDART_TYPE::TYPE) {
            return NewType(MakePointerType(any_cast<Type>(value.data)));
        }
        if (!STANDART_TYPE::TYPES.is_sub_type(value.type)) {
            Type T = MakePointerType(any_cast<Struct>(value.data).type);
            return Value(STANDART_TYPE::TYPE, T);
        }
        ERROR::InvalidDereferenceValue(start, end, value.type);
    }
};