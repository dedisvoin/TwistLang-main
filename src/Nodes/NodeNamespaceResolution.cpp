#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"
#include "../twist-namespace.cpp"

#pragma once

/*
 * NodeNamespaceResolution – разрешение имени через оператор :: (один уровень).
 *
 * Вычисляет левое выражение (должно возвращать Namespace) и извлекает из его
 * памяти переменную с заданным именем. Проверяет приватность.
 *
 * Поля:
 *   namespace_expr – выражение, возвращающее Namespace.
 *   name           – имя, которое извлекается из этого Namespace.
 *   start, end     – токены для позиционирования ошибок.
 */

struct NodeNamespaceResolution : public Node { NO_EXEC
    Node*            namespace_expr;
    string           name;
    Token            start;
    Token            end;

    NodeNamespaceResolution(Node* namespace_expr, const string& name, Token start, Token end)
        : namespace_expr(namespace_expr), name(name), start(start), end(end) {
        NODE_TYPE = NodeTypes::NODE_NAME_RESOLUTION;
    }

    Value eval_from(Memory& _memory) override { 
        // Получаем значение левой части
        Value ns_value = namespace_expr->eval_from(_memory);

        // Проверяем, что это действительно Namespace
        if (ns_value.type != STANDART_TYPE::NAMESPACE)
            throw ERROR_THROW::NamespaceInvalidAccessorType(start, end, ns_value.type);

        auto ns = any_cast<Namespace*>(ns_value.data);
        Memory* ns_memory = ns->memory;

        // Проверяем существование имени
        if (!ns_memory->check_literal(name))
            throw ERROR_THROW::NamespaceUndefinedVariable(start, end, name);

        auto result = ns_memory->get_variable(name);
        if (result->modifiers.is_private)
            throw ERROR_THROW::NamespacePrivateVariable(start, end, name);

       
        return result->value;
    }
};