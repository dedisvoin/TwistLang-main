#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

/*
 * NodeUnary – узел унарной операции.
 *
 * Поддерживает операторы: +, -, !, not, ++, --.
 * Применяется к операнду соответствующего типа.
 *
 * Поля:
 *   operand – подвыражение.
 *   op – строковое представление оператора.
 *   start, end, operator_token – токены для позиционирования ошибок.
 *
 * eval_from() вычисляет операнд, затем в зависимости от типа и оператора
 * возвращает новое значение:
 *   - INT: +, - (меняет знак).
 *   - BOOL: !, not (логическое отрицание).
 *   - NULL_T: ! возвращает true (null считается ложным).
 *   - DOUBLE: +, - (меняет знак).
 *   - POINTER: ++, -- (инкремент/декремент адреса).
 * В случае несовместимости генерируется ошибка.
 */

struct NodeUnary : public Node { NO_EXEC
    unique_ptr<Node> operand;
    string op;

    Token start;
    Token end;
    Token operator_token;

    NodeUnary(unique_ptr<Node> operand, Token start, Token end, Token operator_token)
        : operand(std::move(operand)), start(start), end(end), operator_token(operator_token) {
            this->NODE_TYPE = NodeTypes::NODE_UNARY;
            op = operator_token.value;
        }

    Value eval_from(Memory& _memory) override {
        Value value = operand->eval_from(_memory);

        if (value.type == STANDART_TYPE::INT) {
            if (op == "-") {
                int64_t v = any_cast<int64_t>(value.data);
                return NewInt(-v);
            } else if (op == "+") {
                int64_t v = any_cast<int64_t>(value.data);
                return NewInt(+v);
            }
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, value);
        } else if (value.type == STANDART_TYPE::BOOL) {
            if (op == "!" || op == "not")
                return NewBool(!any_cast<bool>(value.data));
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, value);
        } else if (value.type == STANDART_TYPE::NULL_T) {
            if (op == "!")
                return NewBool(true);
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, value);
        } else if (value.type == STANDART_TYPE::DOUBLE) {
            if (op == "-") {
                NUMBER_ACCURACY v = any_cast<NUMBER_ACCURACY>(value.data);
                return NewDouble(-v);
            } else if (op == "+") {
                NUMBER_ACCURACY v = any_cast<NUMBER_ACCURACY>(value.data);
                return NewDouble(+v);
            }
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, value);
        } else if (value.type.is_pointer()) {
            if (op == "--") {
                long double v = any_cast<int>(value.data);
                return NewPointer(v - 1, value.type, false);
            } else if (op == "++") {
                long double v = any_cast<int>(value.data);
                return NewPointer(v + 1, value.type, false);
            }
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, value);
        } else
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, value);



        // Implement unary operation evaluation here
        return value;
    }
};