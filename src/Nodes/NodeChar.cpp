#include "../twist-nodetemp.cpp"

/*
 * NodeChar – узел символьного литерала.
 *
 * Хранит символ и при первом вычислении создаёт Value с типом CHAR.
 * Для оптимизации используется union, хранящий либо сырой char,
 * либо готовый Value после первого eval_from().
 *
 * Поля:
 *   char_value – исходный символ (до кэширования).
 *   cached_value – кэшированное Value (после первого вызова).
 *   is_cached – флаг, указывающий, создано ли уже Value.
 *
 * Деструктор явно вызывает деструктор cached_value, если он был создан.
 */

struct NodeChar : public Node { NO_EXEC
    // Храним как union: либо char, либо готовый Value
    union {
        char char_value;
        Value cached_value;
    };
    bool is_cached = false;

    NodeChar(char val) : char_value(val) {
        this->NODE_TYPE = NodeTypes::NODE_CHAR;
    }

    ~NodeChar() {
        if (is_cached) {
            cached_value.~Value();  // Явно разрушаем Value если был создан
        }
    }

    Value eval_from(Memory& _memory) override {
        if (!is_cached) {
            // Создаем Value на месте (placement new)
            new (&cached_value) Value(NewChar(char_value));
            is_cached = true;
        }
        return cached_value;
    }

};