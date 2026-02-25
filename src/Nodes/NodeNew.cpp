#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"


struct NodeNew : public Node { NO_EXEC
    unique_ptr<Node> expr;
    unique_ptr<Node> type_expr;
    bool is_const = false;
    bool is_static = false;

    Token start_type;
    Token end_type;

    NodeNew(unique_ptr<Node> expr, unique_ptr<Node> type_expr, bool is_static, bool is_const,
            Token start_type, Token end_type) :
    expr(std::move(expr)), type_expr(std::move(type_expr)), is_const(is_const), is_static(is_static),
    start_type(start_type), end_type(end_type) {
        this->NODE_TYPE = NodeTypes::NODE_NEW;
    }

    Value eval_from(Memory& _memory) override {

        if (type_expr && expr) {
            auto result = expr->eval_from(_memory);
            auto super_type_value = type_expr->eval_from(_memory);
            if (!result.type.is_sub_type(any_cast<Type>(super_type_value.data))) {
                ERROR::StaticTypesMisMatch(start_type, end_type, any_cast<Type>(super_type_value.data), result.type);
            }

            auto object = CreateMemoryObject(result, any_cast<Type>(super_type_value.data), nullptr, is_const, is_static, false, false, false);
            auto addres = NewPointer(object->address, any_cast<Type>(super_type_value.data), true);
            STATIC_MEMORY.register_object(object);
            return addres;
        } else if (type_expr && !expr) {
            auto result = NewNull();
            auto super_type_value = type_expr->eval_from(_memory);
            auto object = CreateMemoryObject(result, any_cast<Type>(super_type_value.data), nullptr, is_const, is_static, false, false, false);
            auto addres = NewPointer(object->address, any_cast<Type>(super_type_value.data), true);
            STATIC_MEMORY.register_object(object);
            return addres;
        } else if (!type_expr && expr) {
            auto result = expr->eval_from(_memory);
            auto object = CreateMemoryObject(result, result.type, nullptr, is_const, is_static, false, false, false);
            auto addres = NewPointer(object->address, result.type, true);
            STATIC_MEMORY.register_object(object);
            return addres;
        } else if (is_static && expr) {
            auto result = expr->eval_from(_memory);
            auto object = CreateMemoryObject(result, result.type, nullptr, is_const, is_static, false, false, false);
            auto addres = NewPointer(object->address, result.type, true);
            STATIC_MEMORY.register_object(object);
            return addres;

        } else {
            ERROR::InvalidNewInstruction(start_type, end_type);
        }
    }
};