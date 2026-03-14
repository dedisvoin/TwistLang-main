#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-structs.cpp"
#include "../twist-functions.cpp"

#pragma once

/*
 * NodeObjectResolution – доступ к полю объекта/структуры через точку (.).
 *
 * Вычисляет выражение, возвращающее структуру (любой нестандартный тип),
 * затем ищет в её памяти поле с указанным именем. Поддерживает цепочки
 * (obj.field.subfield), обрабатывает приватность и специальный случай для
 * методов (возвращает тип METHOD с привязкой к экземпляру).
 *
 * Поля:
 *   obj_expr – выражение, возвращающее структуру.
 *   current_name – имя текущего поля.
 *   remaining_chain – оставшаяся цепочка для рекурсивного разрешения.
 *   start, end – токены для позиционирования ошибок.
 *
 * eval_from() проходит по цепочке, проверяя на каждом шаге, что поле
 * существует, доступно (не private) и, если это структура, продолжает.
 * Если поле оказывается функцией, формируется метод (Method), связывающий
 * функцию с памятью экземпляра.
 */

struct NodeObjectResolution : public Node { NO_EXEC
    Node*    obj_expr;
    string              current_name;

    Token               start;  // токен имени (для ошибок)
    Token               end;    // он же

    NodeObjectResolution(Node* obj_expr, const string& current_name, Token start, Token end)
        : obj_expr(obj_expr), current_name(current_name), start(start), end(end) {
        this->NODE_TYPE = NodeTypes::NODE_OBJECT_RESOLUTION;
    }

    Value eval_from(Memory& _memory) override {
        Value obj_value = obj_expr->eval_from(_memory);

        // Проверяем, что это структура (не стандартный тип)
        if (STANDART_TYPE::TYPES.is_sub_type(obj_value.type))
            ERROR::InvalidAccessorType(start, end, obj_value.type.pool);

        auto& obj = any_cast<Struct&>(obj_value.data);
        Memory* obj_memory = obj.memory.get();

        if (!obj_memory->check_literal(current_name))
            ERROR::UndefinedStructProperty(start, end, current_name, obj_value.type.pool);

        auto result = obj_memory->get_variable(current_name);

        if (result->modifiers.is_private)
            ERROR::PrivatePropertyAccess(start, end, current_name);

        // Если поле — функция, возвращаем метод
        // if (result->value.type.is_func()) {
        //     Method m;
        //     m.func = any_cast<Function*>(result->value.data);
        //     m.instance_memory = obj.memory;
        //     return Value(STANDART_TYPE::METHOD, m);
        // }

        return result->value;
    }
};
