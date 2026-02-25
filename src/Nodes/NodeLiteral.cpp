#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#pragma once

/*
 * NodeLiteral – доступ к переменной по имени.
 *
 * Вычисляет значение переменной, хранящейся в текущей памяти.
 * Если переменная не определена, генерирует ошибку.
 *
 * Поля:
 *   name – имя переменной.
 *   token – токен для позиционирования ошибки.
 */

struct NodeLiteral : public Node { NO_EXEC
    string name;
    Token& token;

    NodeLiteral(Token& token) : token(token) {
        name = token.value;
        this->NODE_TYPE = NODE_LITERAL;
    }

    Value eval_from(Memory& _memory) override {
        if (!_memory.check_literal(name))
            ERROR::UndefinedVariable(token);

        return _memory.get_variable(name)->value;
    }
};