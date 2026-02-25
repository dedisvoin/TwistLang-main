#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

#include <sstream>
#pragma once

/*
 * NodeNumber – узел числового литерала (целого или с плавающей точкой).
 *
 * При создании анализирует токен: если содержит точку, парсит как число
 * с плавающей точкой (используя локаль C для гарантированной точки),
 * иначе как целое. Сохраняет готовое Value.
 *
 * Поля:
 *   value – готовое Value (INT или DOUBLE).
 *
 * Для парсинга double используется std::istringstream с локалью classic(),
 * что обеспечивает корректную обработку десятичной точки независимо от
 * локали системы.
 */

struct NodeNumber : public Node { NO_EXEC
    Value value = NewNull();

    NodeNumber(int value) : value(NewInt(value)) { this->NODE_TYPE = NodeTypes::NODE_NUMBER; }

    NodeNumber(Token& token) {
        this->NODE_TYPE = NodeTypes::NODE_NUMBER;

        size_t dot_count = count(token.value.begin(), token.value.end(), '.');

        if (dot_count > 1) ERROR::InvalidNumber(token, token.value);

        if (dot_count == 1) {
                   
            std::istringstream iss(token.value);
            iss.imbue(std::locale::classic()); // принудительно локаль с точкой
            NUMBER_ACCURACY val;
            if (!(iss >> val)) {
                ERROR::InvalidNumber(token, token.value);
            }
            this->value = NewDouble(val);
            

        } else {
            this->value = NewInt(stoll(token.value));
        }
    }

    Value eval_from(Memory& _memory) override {
        return value;
    }
};
