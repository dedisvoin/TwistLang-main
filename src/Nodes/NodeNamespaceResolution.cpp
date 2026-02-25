#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include "../twist-namespace.cpp"
#include "NodeValueHolder.cpp"

#pragma once
/*
 * NodeNamespaceResolution – разрешение имени через оператор :: (пространство имён).
 *
 * Вычисляет выражение, которое должно возвращать значение типа NAMESPACE,
 * затем ищет в его памяти переменную с указанным именем. Поддерживает цепочки
 * (namespace::name::subname), обрабатывая приватность и проверку типов на каждом шаге.
 *
 * Поля:
 *   namespace_expr – выражение, возвращающее Namespace.
 *   current_name – имя текущего элемента в цепочке.
 *   remaining_chain – оставшаяся часть цепочки для рекурсивного разрешения.
 *   start, end – токены для позиционирования ошибок.
 *
 * eval_from() рекурсивно проходит по цепочке, пока не достигнет конечного элемента.
 * Для каждого промежуточного элемента проверяет, что он является Namespace, и что
 * к нему есть доступ (не private). Финальный результат – значение переменной.
 */

struct NodeNamespaceResolution : public Node { NO_EXEC
    unique_ptr<Node>    namespace_expr;
    string              current_name;
    vector<string>      remaining_chain; // Оставшаяся цепочка имен

    Token               start;
    Token               end;

    NodeNamespaceResolution(unique_ptr<Node> namespace_expr, const string& current_name, Token start, Token end,
                      const vector<string>& remaining_chain = {}) :
        namespace_expr(std::move(namespace_expr)), current_name(current_name), start(start), end(end) ,
        remaining_chain(remaining_chain) {
        this->NODE_TYPE = NodeTypes::NODE_NAME_RESOLUTION;
    }

    Value eval_from(Memory& _memory) override {
        // Получаем значение namespace
        Value ns_value = namespace_expr->eval_from(_memory);

        // Проверяем тип
        if (ns_value.type != STANDART_TYPE::NAMESPACE) {
            ERROR::InvalidAccessorType(start, end, ns_value.type.pool);
        }

        // ИСПРАВЛЕНО: используем ссылку вместо копии
        auto& ns = any_cast<Namespace&>(ns_value.data);

        // Теперь memory - shared_ptr, получаем доступ через get()
        Memory* ns_memory = ns.memory.get();

        // Проверяем существование переменной/namespace
        if (!ns_memory->check_literal(current_name)) {
            ERROR::UndefinedProperty(start, end, current_name, "namespace");
        }

        // Получаем значение
        auto result = ns_memory->get_variable(current_name);

        if (result->modifiers.is_private)
            ERROR::PrivatePropertyAccess(start, end, current_name);

        // Если есть оставшаяся цепочка, продолжаем рекурсивно
        if (!remaining_chain.empty()) {
            // Создаем новую ноду для следующего уровня
            vector<string> next_chain(remaining_chain.begin() + 1, remaining_chain.end());
            auto next_node = make_unique<NodeNamespaceResolution>(
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
