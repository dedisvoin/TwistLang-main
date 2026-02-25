#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-array.cpp"

#include "NodeLiteral.cpp"

/*
 * NodeBinary – узел бинарной операции.
 *
 * Поддерживает широкий спектр операторов (+, -, *, /, **, %, ==, !=, <, >, <=, >=,
 * &&, ||, |, <<, >>, <- и др.) для различных комбинаций типов (Int, Double, Bool,
 * String, Char, Type, Pointer, Array).
 *
 * Особенности:
 * - Для логических операций (&&, ||) используется короткое замыкание: правый операнд
 *   не вычисляется, если результат уже определён.
 * - Для операций с массивами поддерживается конкатенация (+) и добавление элемента
 *   в массив (<-). При добавлении элемента в переменную массива модификация
 *   происходит напрямую в памяти, без копирования.
 * - Для указателей реализована арифметика (+, -) и сравнения.
 * - Для типов (Type) поддерживаются операции объединения (|), проверка подтипа (<<, >>)
 *   и сравнение на равенство.
 * - В случае несовместимых типов или неподдерживаемого оператора генерируется
 *   ошибка времени выполнения.
 *
 * Поля:
 *   left, right – левое и правое подвыражения.
 *   op – строковое представление оператора.
 *   start_token, end_token, op_token – токены для позиционирования ошибок.
 *
 * eval_from() вычисляет левый операнд, затем, если требуется, правый, и возвращает
 * результат операции в виде нового Value.
 */
 
struct NodeBinary : public Node { NO_EXEC
    unique_ptr<Node> left;
    unique_ptr<Node> right;
    string op;

    Token& start_token;
    Token& end_token;
    Token& op_token;

    NodeBinary(unique_ptr<Node> left, const string& operation, unique_ptr<Node> right, Token& start_token, Token& end_token, Token& op_token)
        : left(std::move(left)), right(std::move(right)), op(operation), start_token(start_token), end_token(end_token), op_token(op_token) {
            this->NODE_TYPE = NodeTypes::NODE_BINARY;
        }

    Value eval_from(Memory& _memory) override {
        auto left_val = left->eval_from(_memory);

        if (left_val.type == STANDART_TYPE::BOOL) {
            bool l = any_cast<bool>(left_val.data);
            if ((op == "||" || op == "or") && l)
                return NewBool(true);
            else if ((op == "&&" || op == "and") && !l)
                return NewBool(false);
        }

        auto right_val = right->eval_from(_memory);

        if (left_val.type == STANDART_TYPE::INT && right_val.type == STANDART_TYPE::INT) {
            int64_t l = any_cast<int64_t>(left_val.data);
            int64_t r = any_cast<int64_t>(right_val.data);

            if (op == "+")
                return NewInt(l + r);
            else if (op == "-")
                return NewInt(l - r);
            else if (op == "*")
                return NewInt(l * r);
            else if (op == "/") {
                if (r == 0) ERROR::ZeroDivision(start_token, end_token, op_token, left_val, right_val);
                return NewInt(l / r);
            }
            else if (op == "**")
                return NewInt(pow(l,r));
            else if (op == "%")
                return NewInt(l % r);
            else if (op == "==")
                return NewBool(l == r);
            else if (op == "!=")
                return NewBool(l != r);
            else if (op == "<=")
                return NewBool(l <= r);
            else if (op == ">=")
                return NewBool(l >= r);
            else if (op == "<")
                return NewBool(l < r);
            else if (op == ">")
                return NewBool(l > r);
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

        } else if ((left_val.type == STANDART_TYPE::DOUBLE || left_val.type == STANDART_TYPE::INT) &&
                 (right_val.type == STANDART_TYPE::DOUBLE || right_val.type == STANDART_TYPE::INT)) {
            float l = (left_val.type == STANDART_TYPE::DOUBLE) ? any_cast<NUMBER_ACCURACY>(left_val.data) : static_cast<NUMBER_ACCURACY>(any_cast<int64_t>(left_val.data));
            float r = (right_val.type == STANDART_TYPE::DOUBLE) ? any_cast<NUMBER_ACCURACY>(right_val.data) : static_cast<NUMBER_ACCURACY>(any_cast<int64_t>(right_val.data));

            if (op == "+")
                return NewDouble(l + r);
            else if (op == "-")
                return NewDouble(l - r);
            else if (op == "*")
                return NewDouble(l * r);
            else if (op == "/") {
                if (r == 0) ERROR::ZeroDivision(start_token, end_token, op_token, left_val, right_val);
                return NewDouble(l / r);
            }
            else if (op == "**")
                return NewDouble(pow(l, r));
            else if (op == "%")
                return NewDouble(l - floor(l / r) * r);
            else if (op == "==")
                return NewBool(l == r);
            else if (op == "!=")
                return NewBool(l != r);
            else if (op == "<=")
                return NewBool(l <= r);
            else if (op == ">=")
                return NewBool(l >= r);
            else if (op == "<")
                return NewBool(l < r);
            else if (op == ">")
                return NewBool(l > r);

            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

        } else if (left_val.type == STANDART_TYPE::BOOL && right_val.type == STANDART_TYPE::BOOL) {
            bool l = any_cast<bool>(left_val.data);
            bool r = any_cast<bool>(right_val.data);
            if (op == "==")
                return NewBool(l == r);
            else if (op == "!=")
                return NewBool(l != r);
            else if (op == "||" || op == "or")
                return NewBool(l || r);
            else if (op == "&&" || op == "and")
                return NewBool(l && r);
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if (left_val.type == STANDART_TYPE::TYPE && right_val.type == STANDART_TYPE::TYPE) {
            Type l = any_cast<Type>(left_val.data);
            Type r = any_cast<Type>(right_val.data);

            if (op == "|")
                return Value(STANDART_TYPE::TYPE, l | r);
            if (op == "==")
                return NewBool(l == r);
            if (op == "!=")
                return NewBool(l != r);
            if (op == "<<")
                return NewBool(r.is_sub_type(l));
            if (op == ">>")
                return NewBool(l.is_sub_type(r));
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if ((left_val.type == STANDART_TYPE::NULL_T && right_val.type != STANDART_TYPE::NULL_T) ||
                   (left_val.type != STANDART_TYPE::NULL_T && right_val.type == STANDART_TYPE::NULL_T) ||
                   (left_val.type == STANDART_TYPE::NULL_T && right_val.type == STANDART_TYPE::NULL_T)) {
            if (op == "==") {
                if (left_val.type == right_val.type)
                    return NewBool(true);
                else
                    return NewBool(false);
            }
            else if (op == "!=") {
                if (left_val.type == right_val.type)
                    return NewBool(false);
                else
                    return NewBool(true);
            }
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if (left_val.type == STANDART_TYPE::STRING && right_val.type == STANDART_TYPE::STRING) {
            if (op == "==")
                return NewBool(any_cast<string>(left_val.data) == any_cast<string>(right_val.data));

            if (op == "!=")
                return NewBool(any_cast<string>(left_val.data) != any_cast<string>(right_val.data));

            if (op == "+")
                return NewString(any_cast<string>(left_val.data) + any_cast<string>(right_val.data));
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

        } else if (left_val.type == STANDART_TYPE::CHAR && right_val.type == STANDART_TYPE::CHAR) {
            if (op == "==")
                return NewBool(any_cast<char>(left_val.data) == any_cast<char>(right_val.data));

            if (op == "!=")
                return NewBool(any_cast<char>(left_val.data) != any_cast<char>(right_val.data));

            if (op == "+")
                return NewString(string()+any_cast<char>(left_val.data) + string()+any_cast<char>(right_val.data));
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if (left_val.type == STANDART_TYPE::STRING && right_val.type == STANDART_TYPE::INT) {
            if (op == "*") {
                string dummy;

                for (int i = 0; i < any_cast<int64_t>(right_val.data); i++) {
                    dummy += any_cast<string>(left_val.data);
                }
                return NewString(dummy);
            }
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

        } else if (left_val.type == STANDART_TYPE::CHAR && right_val.type == STANDART_TYPE::INT) {
            if (op == "*") {
                string dummy;

                for (int i = 0; i < any_cast<int64_t>(right_val.data); i++) {
                    dummy += any_cast<char>(left_val.data);
                }
                return NewString(dummy);
            }
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

        } else if (left_val.type.is_pointer() && right_val.type.is_pointer()) {
            if (op == "==")
                return NewBool(any_cast<int>(left_val.data) == any_cast<int>(right_val.data));
            if (op == "!=")
                return NewBool(any_cast<int>(left_val.data) != any_cast<int>(right_val.data));
            if (op == ">")
                return NewBool(any_cast<int>(left_val.data) > any_cast<int>(right_val.data));
            if (op == "<")
                return NewBool(any_cast<int>(left_val.data) < any_cast<int>(right_val.data));
            if (op == ">=")
                return NewBool(any_cast<int>(left_val.data) >= any_cast<int>(right_val.data));
            if (op == "<=")
                return NewBool(any_cast<int>(left_val.data) <= any_cast<int>(right_val.data));
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if (left_val.type.is_pointer() && right_val.type == STANDART_TYPE::INT) {

            if (op == "+")
                return NewPointer(any_cast<int>(left_val.data) + any_cast<int64_t>(right_val.data), left_val.type, false);;

            if (op == "-")
                return NewPointer(any_cast<int>(left_val.data) - any_cast<int64_t>(right_val.data), left_val.type, false);


        } else if (left_val.type.is_array_type() && right_val.type.is_array_type()) {
            if (op == "+") {
                // ОПТИМИЗАЦИЯ: Кэшируем any_cast результаты чтобы не вызывать их в цикле
                Type T = Type("");

                auto& left_arr = any_cast<Array&>(left_val.data);
                for (int i = 0; i < left_arr.values.size(); i++) {
                    T = T | left_arr.values[i].type;
                }

                auto& right_arr = any_cast<Array&>(right_val.data);
                for (int i = 0; i < right_arr.values.size(); i++) {
                    T = T | right_arr.values[i].type;
                }

                T = Type("[" + T.pool + ", ~]");
                return Value(T, Array(T,
                    concatenate(left_arr.values, right_arr.values)));
            }
        } else if (left_val.type.is_array_type()) {
            if (op == "<-") {
                // ОПТИМИЗАЦИЯ: Для push операции, если левая часть - это переменная,
                // модифицируем её ПРЯМО в памяти БЕЗ промежуточного копирования
                if (left->NODE_TYPE == NodeTypes::NODE_LITERAL) {
                    string var_name = ((NodeLiteral*)left.get())->name;
                    MemoryObject* var_obj = _memory.get_variable(var_name);

                    if (var_obj && var_obj->value.type.is_array_type()) {
                        // Модифицируем массив напрямую в памяти
                        auto& arr = any_cast<Array&>(var_obj->value.data);
                        auto arr_type_pair = arr.type.parse_array_type();
                        string elem_type_str = arr_type_pair.first;
                        if (elem_type_str != "") {
                            Type expected(elem_type_str);
                            if (!IsTypeCompatible(expected, right_val.type)) {
                                ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_val.type.pool);
                            }
                        }
                        arr.values.emplace_back(right_val);
                        // Возвращаем ссылку из памяти
                        return var_obj->value;
                    }
                }

                // Fallback для других случаев (например, если левая часть - выражение)
                {
                    auto& arr = any_cast<Array&>(left_val.data);
                    auto arr_type_pair = arr.type.parse_array_type();
                    string elem_type_str = arr_type_pair.first;
                    if (elem_type_str != "") {
                        Type expected(elem_type_str);
                        if (!IsTypeCompatible(expected, right_val.type)) {
                            ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_val.type.pool);
                        }
                    }
                    arr.values.emplace_back(right_val);
                }
                return left_val;
            }
        } else
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        exit(-10);
    }
    
};