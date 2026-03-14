#include "../twist-nodetemp.cpp"

#pragma once

/*
 * NodeScopes – узел группировки выражений в скобках ( ).
 *
 * Просто вычисляет вложенное выражение и возвращает его результат.
 * Используется для изменения приоритета операций и для обёртки выражений,
 * к которым затем применяются постфиксные операции (вызов, индексация и т.д.).
 *
 * Поля:
 *   expression – внутреннее выражение.
 */

struct NodeScopes : public Node { NO_EXEC
    Node* expression;

    NodeScopes(Node* expr) : expression(expr) {
        this->NODE_TYPE = NodeTypes::NODE_SCOPES;
    }

    Value eval_from(Memory& _memory) override {
        return expression->eval_from(_memory);
    }
};
