#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-structs.cpp"
#include "../twist-functions.cpp"

#include "NodeValueHolder.cpp"

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
    unique_ptr<Node>    obj_expr;
    string              current_name;
    vector<string>      remaining_chain; // Оставшаяся цепочка имен

    Token               start;
    Token               end;

    NodeObjectResolution(unique_ptr<Node> obj_expr, const string& current_name, Token start, Token end,
                      const vector<string>& remaining_chain = {}) :
        obj_expr(std::move(obj_expr)), current_name(current_name), start(start), end(end) ,
        remaining_chain(remaining_chain) {
        this->NODE_TYPE = NodeTypes::NODE_OBJECT_RESOLUTION;
    }

    Value eval_from(Memory& _memory) override {
        // Получаем значение namespace
        Value ns_value = obj_expr->eval_from(_memory);

        // Проверяем тип
        if (STANDART_TYPE::UNTYPED.is_sub_type(ns_value.type)) 
            ERROR::InvalidAccessorType(start, end, ns_value.type.pool);
        

        // ИСПРАВЛЕНО: используем ссылку вместо копии
        auto& ns = any_cast<Struct&>(ns_value.data);

        // Теперь memory - shared_ptr, получаем доступ через get()
        Memory* ns_memory = ns.memory.get();

        // Проверяем существование переменной/namespace
        if (!ns_memory->check_literal(current_name)) {
            ERROR::UndefinedStructProperty(start, end, current_name, ns_value.type.pool);
        }

        // Получаем значение
        auto result = ns_memory->get_variable(current_name);

        if (result->modifiers.is_private)
            ERROR::PrivatePropertyAccess(start, end, current_name);

        if (result->value.type.is_func()) {
            Method m;
            m.func = any_cast<Function*>(result->value.data);
            m.instance_memory = ns.memory;  // shared_ptr<Memory> экземпляра
            return Value(STANDART_TYPE::METHOD, m);
        }

        // Если есть оставшаяся цепочка, продолжаем рекурсивно
        if (!remaining_chain.empty()) {
            // Создаем новую ноду для следующего уровня
            vector<string> next_chain(remaining_chain.begin() + 1, remaining_chain.end());
            auto next_node = make_unique<NodeObjectResolution>(
                make_unique<NodeValueHolder>(result->value),
                remaining_chain[0],
                start, end,
                next_chain
            );

            return next_node->eval_from(_memory);
        }

        return result->value;
    }
};