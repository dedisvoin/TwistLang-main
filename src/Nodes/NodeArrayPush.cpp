#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-array.cpp"

#include "NodeObjectResolution.cpp"
#include "NodeScopes.cpp"
#include "NodeLiteral.cpp"
#include "NodeDereference.cpp"
#include "NodeNamespaceResolution.cpp"

struct NodeArrayPush : public Node { NO_EXEC
    unique_ptr<Node> left_expr;
    unique_ptr<Node> right_expr;
    Token start_token;
    Token end_token;

    NodeArrayPush(unique_ptr<Node> left_expr, unique_ptr<Node> right_expr, Token start_token, Token end_token)
        : left_expr(std::move(left_expr)), right_expr(std::move(right_expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_ARRAY_PUSH;
    }

    Value eval_from(Memory& _memory) override {
        if (left_expr->NODE_TYPE == NodeTypes::NODE_SCOPES) {
            left_expr = std::move(((NodeScopes*)(left_expr.get()))->expression);
        }
        // ОПТИМИЗАЦИЯ: Проверяем тип левого выражения
        // Если это простая переменная (NODE_LITERAL), модифицируем её в памяти напрямую
        if (left_expr->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            string var_name = ((NodeLiteral*)left_expr.get())->name;
            MemoryObject* var_obj = _memory.get_variable(var_name);

            if (var_obj && var_obj->value.type.is_array_type()) {
                auto right_value = right_expr->eval_from(_memory);
                // Проверка типа элемента массива
                auto& arr = any_cast<Array&>(var_obj->value.data);
                auto arr_type_pair = arr.type.parse_array_type();
                string elem_type_str = arr_type_pair.first;
                if (elem_type_str != "") {
                    Type expected(elem_type_str);
                    if (!IsTypeCompatible(expected, right_value.type)) {
                        ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                    }
                }
                // Модифицируем массив ПРЯМО в памяти БЕЗ копирования
                arr.values.emplace_back(right_value);
                return var_obj->value;
            }
        }
        // Если это доступ через namespace (NODE_NAME_RESOLUTION), тоже модифицируем напрямую
        else if (left_expr->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            NodeNamespaceResolution* resolution = (NodeNamespaceResolution*)left_expr.get();
            Value ns_value = resolution->namespace_expr->eval_from(_memory);

            if (ns_value.type == STANDART_TYPE::NAMESPACE) {
                auto& ns = any_cast<Namespace&>(ns_value.data);
                string var_name = resolution->current_name;

                if (ns.memory->check_literal(var_name)) {
                    MemoryObject* var_obj = ns.memory->get_variable(var_name);
                    if (var_obj && var_obj->value.type.is_array_type()) {
                        auto right_value = right_expr->eval_from(_memory);
                        auto& arr = any_cast<Array&>(var_obj->value.data);
                        auto arr_type_pair = arr.type.parse_array_type();
                        string elem_type_str = arr_type_pair.first;
                        if (elem_type_str != "") {
                            Type expected(elem_type_str);
                            if (!IsTypeCompatible(expected, right_value.type)) {
                                ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                            }
                        }
                        arr.values.emplace_back(right_value);
                        return var_obj->value;
                    }
                }
            }
        }

        else if (left_expr->NODE_TYPE == NodeTypes::NODE_OBJECT_RESOLUTION) {
            NodeObjectResolution* resolution = (NodeObjectResolution*)left_expr.get();
            Value ns_value = resolution->obj_expr->eval_from(_memory);

            if (!STANDART_TYPE::UNTYPED.is_sub_type(ns_value.type)) {
                auto& ns = any_cast<Struct&>(ns_value.data);
                string var_name = resolution->current_name;

                if (ns.memory->check_literal(var_name)) {
                    MemoryObject* var_obj = ns.memory->get_variable(var_name);
                    if (var_obj && var_obj->value.type.is_array_type()) {
                        auto right_value = right_expr->eval_from(_memory);
                        auto& arr = any_cast<Array&>(var_obj->value.data);
                        auto arr_type_pair = arr.type.parse_array_type();
                        string elem_type_str = arr_type_pair.first;
                        if (elem_type_str != "") {
                            Type expected(elem_type_str);
                            if (!IsTypeCompatible(expected, right_value.type)) {
                                ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                            }
                        }
                        arr.values.emplace_back(right_value);
                        return var_obj->value;
                    }
                }
            }
        }
        // ОБРАБОТКА РАЗЫМЕНОВАНИЯ УКАЗАТЕЛЯ
        else if (left_expr->NODE_TYPE == NodeTypes::NODE_DEREFERENCE) {
            NodeDereference* deref = (NodeDereference*)left_expr.get();

            // Вычисляем выражение, которое должно быть указателем
            Value ptr_value = deref->expr->eval_from(_memory);

            if (!ptr_value.type.is_pointer()) {
                ERROR::InvalidArrayPushType(start_token, end_token, ptr_value.type.pool);
            }

            // Получаем адрес из указателя
            int address = any_cast<int>(ptr_value.data);

            // Находим объект в STATIC_MEMORY
            MemoryObject* obj = STATIC_MEMORY.get_by_address(address);
            if (!obj) {
                ERROR::InvalidArrayPushType(start_token, end_token, "null pointer");
            }

            if (!obj->value.type.is_array_type()) {
                ERROR::InvalidArrayPushType(start_token, end_token, obj->value.type.pool);
            }

            auto right_value = right_expr->eval_from(_memory);
            auto& arr = any_cast<Array&>(obj->value.data);
            auto arr_type_pair = arr.type.parse_array_type();
            string elem_type_str = arr_type_pair.first;
            if (elem_type_str != "") {
                Type expected(elem_type_str);
                if (!IsTypeCompatible(expected, right_value.type)) {
                    ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                }
            }
            arr.values.emplace_back(right_value);

            // Возвращаем значение из памяти (не копию)
            return obj->value;
        }
        // ОБРАБОТКА СЛОЖНЫХ ВЫРАЖЕНИЙ С РАЗЫМЕНОВАНИЕМ И ДОСТУПОМ К ПОЛЯМ
        else if (left_expr->NODE_TYPE == NodeTypes::NODE_GET_BY_INDEX) {
            // Если это индексация, модифицируем копию
            auto left_value = left_expr->eval_from(_memory);
            auto right_value = right_expr->eval_from(_memory);

            if (!left_value.type.is_array_type()) {
                ERROR::InvalidArrayPushType(start_token, end_token, left_value.type.pool);
            }
            auto& arr = any_cast<Array&>(left_value.data);
            auto arr_type_pair = arr.type.parse_array_type();
            string elem_type_str = arr_type_pair.first;
            if (elem_type_str != "") {
                Type expected(elem_type_str);
                if (!IsTypeCompatible(expected, right_value.type)) {
                    ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                }
            }
            arr.values.emplace_back(right_value);
            return left_value;
        }
        
        // FALLBACK: Для других случаев (выражений) работаем с копией
        auto left_value = left_expr->eval_from(_memory);
        auto right_value = right_expr->eval_from(_memory);

        if (!left_value.type.is_array_type()) {
            ERROR::InvalidArrayPushType(start_token, end_token, left_value.type.pool);
        }

        // Модифицируем копию и возвращаем её
        {
            auto& arr = any_cast<Array&>(left_value.data);
            auto arr_type_pair = arr.type.parse_array_type();
            string elem_type_str = arr_type_pair.first;
            if (elem_type_str != "") {
                Type expected(elem_type_str);
                if (!IsTypeCompatible(expected, right_value.type)) {
                    ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                }
            }
            arr.values.emplace_back(right_value);
        }
        return left_value;
    }

};
