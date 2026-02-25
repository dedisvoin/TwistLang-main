#include "../twist-nodetemp.cpp"

/*
 * NodeBool – узел булева литерала (true / false).
 *
 * При создании запоминает значение в виде готового объекта Value (ValueTrue или ValueFalse).
 * Это позволяет избежать повторного создания Value при каждом вычислении.
 *
 * Поля:
 *   token – токен, содержащий значение "true" или "false".
 *   value – предварительно созданное Value (true или false).
 *
 * eval_from() возвращает сохранённое value.
 */

auto ValueTrue = NewBool(true);
auto ValueFalse = NewBool(false);

struct NodeBool : public Node { NO_EXEC
    Token token;
    Value value = NewNull();

    NodeBool(Token& token) {
        this->NODE_TYPE = NodeTypes::NODE_BOOL;
        this->token = token;
        if (token.value == "true") this->value = ValueTrue;
        if (token.value == "false") this->value = ValueFalse;
    }

    Value eval_from(Memory& _memory) override {
        if (token.value == "true") return this->value;
        if (token.value == "false") return this->value;
        return ValueFalse;
    }
};
