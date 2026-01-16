#include "twist-tokenwalker.cpp"
#include "twist-namespace.cpp"
#include "twist-tokens.cpp"
#include "twist-errors.cpp"
#include "twist-values.cpp"
#include "twist-memory.cpp"

#include <type_traits>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <any>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"

using namespace std;


/*
    Base node structure
*/
struct Node {
    string NAME;                    // Node name
    virtual ~Node() = default;      // Destructor
    virtual void exec() = 0;
    virtual Value eval() = 0;
};

struct NodeNumber : public Node {
    enum ValueType { INT, DOUBLE } type;
    int64_t int_value;
    long double double_value;


    NodeNumber(Token& token) {
        this->NAME = "Number";
        string value = token.value;

        size_t dot_count = count(value.begin(), value.end(), '.');
        if (dot_count > 1) {
            ERROR::InvalidNumber(token, value);
        }

        if (dot_count == 1) {
            this->double_value = stold(value);
            this->type = DOUBLE;
        } else {
            this->int_value = stoll(value);
            this->type = INT;
        }
    }

    Value eval() override {
        if (type == INT) {
            return NewInt(int_value);
        } else {
            return NewDouble(double_value);
        }
    }

    void exec() override {}
};

struct NodeString : public Node {
    string value;
    NodeString(string value) {
        this->value = value;
    }

    Value eval() override {
        return NewString(value);
    }

    void exec() override {}
};

struct NodeChar : public Node {
    char value;
    NodeChar(char value) {
        this->value = value;
    }

    Value eval() override {
        return NewChar(value);
    }

    void exec() override {}
};

struct NodeBool : public Node {
    bool value;

    NodeBool(Token& token) {
        if (token.value == "true") value = true;
        if (token.value == "false") value = false;
    }

    Value eval() override { return Value(make_unique<Type>(STANDART_TYPE::BOOL), value); }
    void exec() override {}
};

struct NodeNull : public Node {
    Value value;

    NodeNull() {
        this->value = NewNull();
    }

    Value eval() override {
        return this->value;
    }
    void exec() override {}
};

struct NodeLiteral : public Node {
    string name;
    Token& token;
    Memory& memory;

    NodeLiteral(Token& token, Memory& memory) : token(token), memory(memory) {
        name = token.value;
        this->NAME = "Literal";
    }

    Value eval() override {
        if (!memory.check_literal(name))
            ERROR::UndefinedVariable(token);

        return memory.get_variable(name)->value;
    }

    MemoryObject* get_memory_object() {
        return memory.get_variable(name);
    }

    void exec() override {};
};

struct NodeValueHolder : public Node {
    Value value;

    NodeValueHolder(Value val) : value(std::move(val)) {
        this->NAME = "ValueHolder";
    }

    Value eval() override {
        return value;
    }

    void exec() override {}
};

struct NodeNameResolution : public Node {
    unique_ptr<Node> namespace_expr;
    string current_name;
    vector<string> remaining_chain; // Оставшаяся цепочка имен

    Token start;
    Token end;

    NodeNameResolution(unique_ptr<Node> namespace_expr, const string& current_name, Token start, Token end,
                      const vector<string>& remaining_chain = {}) :
        namespace_expr(std::move(namespace_expr)), current_name(current_name), start(start), end(end) ,
        remaining_chain(remaining_chain) {
        this->NAME = "NameResolutionLiteral";
    }

    Value eval() override {
        // Получаем значение namespace
        Value ns_value = namespace_expr->eval();

        // Проверяем тип
        if (*ns_value.type != STANDART_TYPE::NAMESPACE) {
            cout << "ERROR TYPE" << endl;
            exit(0);
        }

        auto ns = any_cast<Namespace>(ns_value.data);

        // Проверяем существование переменной/namespace
        if (!ns.memory->check_literal(current_name)) {
            ERROR::UndefinedLeftVariable(start, end, current_name);
        }

        // Получаем значение
        Value result = ns.memory->get_variable(current_name)->value;

        // Если есть оставшаяся цепочка, продолжаем рекурсивно
        if (!remaining_chain.empty()) {
            // Создаем новую ноду для следующего уровня
            vector<string> next_chain(remaining_chain.begin() + 1, remaining_chain.end());
            auto next_node = make_unique<NodeNameResolution>(
                make_unique<NodeValueHolder>(result),
                remaining_chain[0],
                start, end,
                next_chain
            );

            return next_node->eval();
        }

        return result;
    }

    void exec() override {}
};

// + SUCCESS WORKS
struct NodeScopes : public Node {
    unique_ptr<Node> expression;

    NodeScopes(unique_ptr<Node> expr) : expression(std::move(expr)) {
        this->NAME = "Scopes";
    }

    void exec() override {
        expression->eval();
    }

    Value eval() override {
        return expression->eval();
    }
};


// + SUCCESS WORKDS
struct NodeUnary : public Node {
    unique_ptr<Node> operand;
    string op;

    Token start;
    Token end;
    Token operator_token;

    NodeUnary(unique_ptr<Node> operand, Token start, Token end, Token operator_token)
        : operand(std::move(operand)), start(start), end(end), operator_token(operator_token) {
            this->NAME = "UnaryExpression";
            op = operator_token.value;
        }

    void exec() override {}

    Value eval() override {
        Value val = operand->eval();

        if (*val.type == STANDART_TYPE::INT) {
            if (op == "-") {
                int64_t v = any_cast<int64_t>(val.data);
                return NewInt(-v);
            } else if (op == "+") {
                int64_t v = any_cast<int64_t>(val.data);
                return NewInt(+v);
            }
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, val);
        } else if (*val.type == STANDART_TYPE::BOOL) {
            if (op == "!" || op == "not")
                return NewBool(!any_cast<bool>(val.data));
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, val);
        } else if (*val.type == STANDART_TYPE::NULL_T) {
            if (op == "!")
                return NewBool(true);
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, val);
        } else if (*val.type == STANDART_TYPE::DOUBLE) {
            if (op == "-") {
                long double v = any_cast<long double>(val.data);
                return NewDouble(-v);
            } else if (op == "+") {
                long double v = any_cast<long double>(val.data);
                return NewDouble(+v);
            }
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, val);
        } else if (val.type->is_pointer) {
            if (op == "--") {
                long double v = any_cast<int>(val.data);
                return NewPointer(v - 1, val.type->pointer_type->name);
            } else if (op == "++") {
                long double v = any_cast<int>(val.data);
                return NewPointer(v + 1, val.type->pointer_type->name);
            }
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, val);
        } else
            ERROR::UnsupportedUnaryOperator(operator_token, start, end, val);



        // Implement unary operation evaluation here
        return val;
    }
};

// + SUCCESS WORKS
struct NodeBinary : public Node {
    unique_ptr<Node> left;
    unique_ptr<Node> right;
    string op;

    Token& start_token;
    Token& end_token;
    Token& op_token;

    NodeBinary(unique_ptr<Node> left, const string& operation, unique_ptr<Node> right, Token& start_token, Token& end_token, Token& op_token)
        : left(std::move(left)), right(std::move(right)), op(operation), start_token(start_token), end_token(end_token), op_token(op_token) {
            this->NAME = "BinaryExpression";
        }

    void exec() override {}

    Value eval() override {
        auto left_val = left->eval();

        if (*left_val.type == STANDART_TYPE::BOOL) {
            bool l = any_cast<bool>(left_val.data);
            if ((op == "||" || op == "or") && l)
                return NewBool(true);
            else if ((op == "&&" || op == "and") && !l)
                return NewBool(false);
        }

        auto right_val = right->eval();

        if (*left_val.type == STANDART_TYPE::INT && *right_val.type == STANDART_TYPE::INT) {
            int64_t l = any_cast<int64_t>(left_val.data);
            int64_t r = any_cast<int64_t>(right_val.data);

            if (op == "+")
                return NewInt(l + r);
            else if (op == "-")
                return NewInt(l - r);
            else if (op == "*")
                return NewInt(l * r);
            else if (op == "/")
                return NewInt(l / r);
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

        } else if ((*left_val.type == STANDART_TYPE::DOUBLE || *left_val.type == STANDART_TYPE::INT) &&
                 (*right_val.type == STANDART_TYPE::DOUBLE || *right_val.type == STANDART_TYPE::INT)) {
            long double l = (*left_val.type == STANDART_TYPE::DOUBLE) ? any_cast<long double>(left_val.data) : static_cast<long double>(any_cast<int64_t>(left_val.data));
            long double r = (*right_val.type == STANDART_TYPE::DOUBLE) ? any_cast<long double>(right_val.data) : static_cast<long double>(any_cast<int64_t>(right_val.data));

            if (op == "+")
                return NewDouble(l + r);
            else if (op == "-")
                return NewDouble(l - r);
            else if (op == "*")
                return NewDouble(l * r);
            else if (op == "/")
                return NewDouble(l / r);
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

        } else if (*left_val.type == STANDART_TYPE::BOOL && *right_val.type == STANDART_TYPE::BOOL) {
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
        } else if (*left_val.type == STANDART_TYPE::TYPE && *right_val.type == STANDART_TYPE::TYPE) {
            Type l = any_cast<Type>(left_val.data);
            Type r = any_cast<Type>(right_val.data);
            
            if (op == "|")
                return Value(make_unique<Type>(STANDART_TYPE::TYPE), l | r);
            if (op == "==")
                return NewBool(l == r);
            if (op == "!=")
                return NewBool(l != r);
            if (op == "<<")
                return NewBool(IsSubType(r, l));
            if (op == ">>")
                return NewBool(IsSubType(l, r));
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if ((*left_val.type == STANDART_TYPE::NULL_T && *right_val.type != STANDART_TYPE::NULL_T) ||
                   (*left_val.type != STANDART_TYPE::NULL_T && *right_val.type == STANDART_TYPE::NULL_T) ||
                   (*left_val.type == STANDART_TYPE::NULL_T && *right_val.type == STANDART_TYPE::NULL_T)) {
            if (op == "==") {
                if (*left_val.type == *right_val.type)
                    return NewBool(true);
                else
                    return NewBool(false);
            }
            else if (op == "!=") {
                if (*left_val.type == *right_val.type)
                    return NewBool(false);
                else
                    return NewBool(true);
            }
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if (*left_val.type == STANDART_TYPE::STRING && *right_val.type == STANDART_TYPE::STRING) {
            if (op == "==")
                return NewBool(any_cast<string>(left_val.data) == any_cast<string>(right_val.data));

            if (op == "!=")
                return NewBool(any_cast<string>(left_val.data) != any_cast<string>(right_val.data));

            if (op == "!=")
                return NewBool(any_cast<string>(left_val.data) != any_cast<string>(right_val.data));

            if (op == "+")
                return NewString(any_cast<string>(left_val.data) + any_cast<string>(right_val.data));
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
        } else if (*left_val.type == STANDART_TYPE::STRING && *right_val.type == STANDART_TYPE::INT) {
            if (op == "*") {
                string dummy;

                for (int i = 0; i < any_cast<int64_t>(right_val.data); i++) {
                    dummy += any_cast<string>(left_val.data);
                }
                return NewString(dummy);
            }
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

        } else if (left_val.type->is_pointer && right_val.type->is_pointer) {
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
        } else
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);

    }
};

// + SUCCESS WORKS
struct NodeBaseVariableDecl : public Node {
    string var_name;
    unique_ptr<Node> value_expr;
    Memory& memory;
    Token decl_token;
    unique_ptr<Node> type_expr;
    Token type_start_token;
    Token type_end_token;

    Token start_expr_token;
    Token end_expr_token;

    bool nullable = false;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;

    NodeBaseVariableDecl(const string& name, unique_ptr<Node> expr, Memory& memory, Token decl_token, unique_ptr<Node> type_expr,
                            Token type_start_token, Token type_end_token, bool nullable, Token start_expr_token, Token end_expr_token)
        : var_name(name), memory(memory), decl_token(decl_token), type_start_token(type_start_token), type_end_token(type_end_token), 
        nullable(nullable), start_expr_token(start_expr_token), end_expr_token(end_expr_token) {
            this->NAME = "BaseVariableDecl";
            this->value_expr = std::move(expr);
            this->type_expr = std::move(type_expr);
        }

    void exec() override {
        Value value = value_expr->eval();

        if (memory.check_literal(var_name)) {
            if (memory.is_final(var_name)) {
                ERROR::VariableAlreadyDefined(decl_token, var_name);
            }
            memory.delete_variable(var_name);
        }

        Type static_type = *value.type.get();
        if (is_static) {
            if (!type_expr) ERROR::WaitedTypeExpression(type_end_token);

            if (type_expr->NAME == "Literal" && ((NodeLiteral*)type_expr.get())->name == "auto") {
                static_type = *value.type.get();
            } else {
                auto type_value = type_expr->eval();
                
                if (*type_value.type != STANDART_TYPE::TYPE)
                    ERROR::InvalidType(type_start_token, type_end_token);
                
                static_type = any_cast<Type>(type_value.data);
                if (nullable) 
                    static_type = static_type | STANDART_TYPE::NULL_T;
                
            }

            // Используем IsTypeCompatible вместо прямого сравнения
            if (!IsTypeCompatible(static_type, *value.type.get())) {
                ERROR::IncompartableTypeVarDeclaration(type_start_token, type_end_token, start_expr_token, end_expr_token, static_type, *value.type.get());
            }
        }

        MemoryObject* object = new MemoryObject(value, static_type, is_const, is_static, is_final, is_global, &memory);
        STATIC_MEMORY.register_object(object);
        memory.add_object(var_name, object);
    }
    Value eval() override {};
};

// + SUCCESS WORKS
struct NodeBaseOut : public Node {
    unique_ptr<Node> expression;
    NodeBaseOut(unique_ptr<Node> expr) : expression(std::move(expr)) {
        this->NAME = "BaseOut";
    }

    Value eval() override { }

    void exec() override{
        auto value = expression->eval();

        if (*value.type == STANDART_TYPE::INT)
            cout << any_cast<int64_t>(value.data);
        if (*value.type == STANDART_TYPE::DOUBLE)
            cout << any_cast<long double>(value.data);
        if (*value.type == STANDART_TYPE::BOOL) {
            auto v = any_cast<bool>(value.data);
            if (v == true) cout << "true";
            if (v == false) cout << "false";
        }
        if (*value.type == STANDART_TYPE::TYPE) {
            cout << any_cast<Type>(value.data).name;
        }
        if (*value.type == STANDART_TYPE::NULL_T) {
            cout << "null";
        }
        if (*value.type == STANDART_TYPE::NAMESPACE) {
            cout << any_cast<Namespace>(value.data).name;
        }
        if (*value.type == STANDART_TYPE::STRING) {
            cout << any_cast<string>(value.data);
        }
        if (*value.type == STANDART_TYPE::CHAR) {
            cout << any_cast<char>(value.data);
        }
        if (value.type->is_pointer) {
            cout << value.type->name << "[0x" + to_string(any_cast<int>(value.data)) << "]";
        }
    }
};

// + SUCCESS WORKS
struct NodeBaseOutLn : public Node {
    unique_ptr<Node> expression;
    NodeBaseOutLn(unique_ptr<Node> expr) : expression(std::move(expr)) {
        this->NAME = "BaseOut";
    }

    Value eval() override { }

    void exec() override{
        auto value = expression->eval();

        if (*value.type == STANDART_TYPE::INT)
            cout << any_cast<int64_t>(value.data) << endl;
        if (*value.type == STANDART_TYPE::DOUBLE)
            cout << any_cast<long double>(value.data) << endl;
        if (*value.type == STANDART_TYPE::BOOL) {
            auto v = any_cast<bool>(value.data) ;
            if (v == true) cout << "true" << endl;
            if (v == false) cout << "false" << endl;
        }
        if (*value.type == STANDART_TYPE::TYPE) {
            cout << any_cast<Type>(value.data).name << endl;
        }
        if (*value.type == STANDART_TYPE::NULL_T) {
            cout << "null" << endl;
        }
        if (*value.type == STANDART_TYPE::NAMESPACE) {
            cout << any_cast<Namespace>(value.data).name << endl;
        }
        if (*value.type == STANDART_TYPE::STRING) {
            cout << any_cast<string>(value.data) << endl;
        }
        if (*value.type == STANDART_TYPE::CHAR) {
            cout << any_cast<char>(value.data) << endl;
        }
        if (value.type->is_pointer) {
            cout << value.type->name << "[0x" + to_string(any_cast<int>(value.data)) << "]" << endl;
        }
    }
};

// + UNSTABLE WORKS
struct NodeVariableEqual : public Node {
    unique_ptr<Node> expression;
    unique_ptr<Node> variable;

    Token start_left_value_token;
    Token end_left_value_token;

    Token start_value_token;
    Token end_value_token;

    NodeVariableEqual(unique_ptr<Node> variable, unique_ptr<Node> expression, Token start_left_value_token, Token end_left_value_token,
                        Token start_value_token, Token end_value_token)  :
        expression(std::move(expression)), variable(std::move(variable)),
        start_left_value_token(start_left_value_token), end_left_value_token(end_left_value_token),
        start_value_token(start_value_token), end_value_token(end_value_token) {};

    void exec() override {
        auto right_value = expression->eval();

        // НЕ перемещаем variable, используем сырой указатель
        Node* variable_ptr = variable.get();

        // Получаем целевую память и имя переменной
        pair<Memory*, string> target = resolveAssignmentTarget(variable_ptr);

        Memory* target_memory = target.first;
        string target_var_name = target.second;

        if (!target_memory->check_literal(target_var_name))
            ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, target_var_name);

        // Проверяем константность
        if (target_memory->is_const(target_var_name)) {
            ERROR::ConstRedefinition(start_left_value_token, end_value_token, target_var_name);
        }

        // Проверяем типизацию для статических переменных
        if (target_memory->is_static(target_var_name)) {
            auto wait_type = target_memory->get_wait_type(target_var_name);
            auto value_type = *right_value.type;
            if (!IsTypeCompatible(wait_type, value_type)) {
                exit(-1);
            }
        }

        // Выполняем присваивание
        target_memory->set_object_value(target_var_name, right_value);
    }


    pair<Memory*, string> resolveAssignmentTarget(Node* node) {
        if (node->NAME == "Literal") {
            auto memory_object = ((NodeLiteral*)node)->get_memory_object();
            if (!memory_object) {
                ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, ((NodeLiteral*)node)->name);
            }
            return {static_cast<Memory*>(memory_object->memory_pointer), ((NodeLiteral*)node)->name};
        }
        else if (node->NAME == "NameResolutionLiteral") {
            NodeNameResolution* resolution = (NodeNameResolution*)node;

            // Получаем namespace
            Value ns_value = resolution->namespace_expr->eval();

            if (*ns_value.type != STANDART_TYPE::NAMESPACE) {
                cout << "ERROR TYPE" << endl;
                exit(0);
            }

            auto ns = any_cast<Namespace>(ns_value.data);

            // Если есть цепочка, идем по ней
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            if (full_chain.size() > 1) {
                // Ищем конечный namespace через цепочку
                Memory* current_memory = ns.memory.get();

                // Проходим по всем промежуточным namespace кроме последнего
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& ns_name = full_chain[i];

                    if (!current_memory->check_literal(ns_name)) {
                        ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                    }

                    Value next_ns_value = current_memory->get_variable(ns_name)->value;

                    if (*next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                        cout << "ERROR TYPE" << endl;
                        exit(0);
                    }

                    current_memory = any_cast<Namespace>(next_ns_value.data).memory.get();
                }

                return {current_memory, full_chain.back()};
            }

            // Если цепочки нет, возвращаем текущий namespace и имя
            return {ns.memory.get(), resolution->current_name};
        }

        // Ошибка для неизвестного типа ноды
        cout << "ERROR TYPE" << endl;
        exit(0);
        return {nullptr, ""}; // Для компилятора
    }

    Value eval() override {}
};

// + SUCCESS WORKS
struct NodeBlock : public Node {
    vector<unique_ptr<Node>> nodes_array;

    NodeBlock(vector<unique_ptr<Node>> &nodes_array) : nodes_array(std::move(nodes_array)) {}

    void exec() override {
        for (int i = 0; i < nodes_array.size(); i++) {
            nodes_array[i]->exec();
        }
    }

    Value eval() override {}
};

// + SUCCESS WORKS
struct NodeIf : public Node {
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> true_statement;
    unique_ptr<Node> else_statement = nullptr;

    NodeIf(unique_ptr<Node> eq_expression, unique_ptr<Node> true_statement, unique_ptr<Node> else_statement = nullptr) :
        eq_expression(std::move(eq_expression)), true_statement(std::move(true_statement)),
        else_statement(std::move(else_statement)) {}

    void exec() override {
        auto value = eq_expression->eval();

        if (*value.type == STANDART_TYPE::BOOL) {
            if (any_cast<bool>(value.data)) {
                true_statement->exec();
                return;
            }
            if (else_statement) else_statement->exec();
        } else if (*value.type == STANDART_TYPE::NULL_T) {
            else_statement->exec();
        } else {
            true_statement->exec();
        }
    }

    Value eval() override {}
};

// + SUCCESS WORKS
struct NodeNamespaceDecl : public Node {
    shared_ptr<Memory> namespace_memory;
    unique_ptr<Node> statement;
    Memory& parent_memory;
    string name;

    Token decl_token;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;

    NodeNamespaceDecl(Memory& parent_memory, unique_ptr<Node> statement, string name, Token decl_token) :
        parent_memory(parent_memory), statement(std::move(statement)), name(name), decl_token(decl_token) {
            this->NAME = "NamespaceDecl";
            // Создаем новую память для namespace
            namespace_memory = make_unique<Memory>();
            // Копируем глобальные переменные из родительской области видимости

        }

    void exec() override {
        // Выполняем statement в контексте namespace памяти
        parent_memory.link_objects(*namespace_memory);
        if (statement) {
            statement->exec();
        }

        if (parent_memory.check_literal(name)) {
            if (parent_memory.is_final(name)) {
                ERROR::VariableAlreadyDefined(decl_token, name);
            }
            parent_memory.delete_variable(name);
        }

        auto new_namespace = NewNamespace(namespace_memory, name);
        parent_memory.add_object(name, new_namespace, STANDART_TYPE::NAMESPACE, is_const, is_static, is_final, is_global);
    }

    Value eval() override {
        // Namespace не возвращает значение
        return NewNull();
    }
};

struct Break {};

// + SUCCESS WORKS
struct NodeBreak : public Node {
    NodeBreak() {}

    void exec() override {
        throw Break();
    }

    Value eval() override {}
};

struct Continue {};

// + SUCCESS WORKS
struct NodeContinue : public Node {
    NodeContinue() {}

    void exec() override {
        throw Continue();
    }

    Value eval() override {}
};


// + SUCCESS WORKS
struct NodeWhile : public Node {
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> statement;

    NodeWhile(unique_ptr<Node> eq_expression, unique_ptr<Node> statement) :
        eq_expression(std::move(eq_expression)), statement(std::move(statement)) {}

    void exec() override {
        while (true) {
            auto value = eq_expression->eval();
            if (*value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (*value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (*value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }

            try {
                statement->exec();
            }
            catch (Break) { break; }
            catch (Continue) { continue; }
        }
    }

    Value eval() override {}
};


// + SUCCESS WORKS
struct NodeDoWhile : public Node {
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> statement;

    NodeDoWhile(unique_ptr<Node> eq_expression, unique_ptr<Node> statement) :
        eq_expression(std::move(eq_expression)), statement(std::move(statement)) {}

    void exec() override {
        while (true) {
            try { statement->exec(); }
            catch (Break) { break; }
            catch (Continue) { continue; }

            auto value = eq_expression->eval();
            if (*value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (*value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (*value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }
        }
    }

    Value eval() override {}
};


// + SUCCESS WORKS
struct NodeFor : public Node {
    unique_ptr<Node> start_state;
    unique_ptr<Node> check_expr;
    unique_ptr<Node> update_state;
    unique_ptr<Node> body;

    NodeFor(unique_ptr<Node> start_state, unique_ptr<Node> check_expr, unique_ptr<Node> update_state, unique_ptr<Node> body) :
        start_state(std::move(start_state)), check_expr(std::move(check_expr)),
        update_state(std::move(update_state)), body(std::move(body)) {}

    void exec() override {
        start_state->exec();

        while (true) {
            auto value = check_expr->eval();
            if (*value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (*value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (*value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }

        try {
            body->exec();
        }
        catch (Break) { break; }
        catch (Continue) {
            // Пропускаем оставшуюся часть итерации
            // но ВСЕГДА выполняем update_state перед continue
            update_state->exec();
            continue;
        }

        // Нормальное выполнение (без continue)
        update_state->exec();

        }
    }

    Value eval() override {}
};


// + SUCCESS WORKS
struct NodeBlockDecl : public Node {
    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;

    vector<unique_ptr<Node>> decls;

    NodeBlockDecl(vector<unique_ptr<Node>> decls) : decls(std::move(decls)) {
        this->NAME = "BlockDecl";
    }


    void exec() override {
        for (int i = 0; i < decls.size(); i++) {
            if (decls[i]->NAME == "BaseVariableDecl") {
                ((NodeBaseVariableDecl*)decls[i].get())->is_const = is_const;
                ((NodeBaseVariableDecl*)decls[i].get())->is_static = is_static;
                ((NodeBaseVariableDecl*)decls[i].get())->is_final = is_final;
                ((NodeBaseVariableDecl*)decls[i].get())->is_global = is_global;
            }
            if (decls[i]->NAME == "NamespaceDecl") {
                ((NodeNamespaceDecl*)decls[i].get())->is_const = is_const;
                ((NodeNamespaceDecl*)decls[i].get())->is_static = is_static;
                ((NodeNamespaceDecl*)decls[i].get())->is_final = is_final;
                ((NodeNamespaceDecl*)decls[i].get())->is_global = is_global;
            }
            decls[i]->exec();

        }
    }

    Value eval() override {}
};

struct NodeAddressOf : public Node {
    unique_ptr<Node> expr;

    NodeAddressOf(unique_ptr<Node> expr) : expr(std::move(expr)) {}

    void exec() override {}

    pair<Memory*, string> resolveAssignmentTarget(Node* node) {
        NodeNameResolution* resolution = (NodeNameResolution*)node;

        // Получаем namespace
        Value ns_value = resolution->namespace_expr->eval();

        if (*ns_value.type != STANDART_TYPE::NAMESPACE) {
            cout << "ERROR TYPE" << endl;
            exit(0);
        }

        auto ns = any_cast<Namespace>(ns_value.data);

        // Если есть цепочка, идем по ней
        vector<string> full_chain = resolution->remaining_chain;
        full_chain.insert(full_chain.begin(), resolution->current_name);

        if (full_chain.size() > 1) {
            // Ищем конечный namespace через цепочку
            Memory* current_memory = ns.memory.get();

            // Проходим по всем промежуточным namespace кроме последнего
            for (size_t i = 0; i < full_chain.size() - 1; i++) {
                const string& ns_name = full_chain[i];

                if (!current_memory->check_literal(ns_name)) {
                    ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                }

                Value next_ns_value = current_memory->get_variable(ns_name)->value;

                if (*next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                    cout << "ERROR TYPE" << endl;
                    exit(0);
                }

                current_memory = any_cast<Namespace>(next_ns_value.data).memory.get();
            }

            return {current_memory, full_chain.back()};
        }

        // Если цепочки нет, возвращаем текущий namespace и имя
        return {ns.memory.get(), resolution->current_name};
    }

    Value eval() override {
        auto value = expr->eval();
        if (expr->NAME == "Scopes") {
            while (expr->NAME == "Scopes")
                expr = std::move(((NodeScopes*)(expr.get()))->expression);
        }
        if (expr->NAME == "Literal") {
            auto var_name = ((NodeLiteral*)(expr.get()))->name;
            auto addr = ((NodeLiteral*)(expr.get()))->memory.get_variable(var_name)->address;
            return NewPointer(addr, *value.type);
        }  
        if (expr->NAME == "NameResolutionLiteral") {
            auto [memory, var_name] = resolveAssignmentTarget(expr.get());
            auto addr = memory->get_variable(var_name)->address;
            return NewPointer(addr, *value.type);
        }
    }
};


struct NodeDereference : public Node {
    unique_ptr<Node> expr;

    NodeDereference(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NAME = "Dereference";
    }

    void exec() override {
        
    }

    Value eval() override {
        auto value = expr->eval();
        if (value.type->is_pointer) {
            auto object = STATIC_MEMORY.get_by_address(any_cast<int>(value.data));
            if (!object)
                return NewNull();
            return object->value;
        }
        if (*value.type == STANDART_TYPE::TYPE) {
            return NewType(PointerType(any_cast<Type>(value.data)));
        }
    }
};

struct NodeLeftDereference : public Node {
    unique_ptr<Node> left_expr;
    unique_ptr<Node> right_expr;

    Token start_left_value_token;
    Token end_left_value_token;

    Token start_value_token;
    Token end_value_token;


    NodeLeftDereference(unique_ptr<Node> variable, unique_ptr<Node> expression, Token start_left_value_token, Token end_left_value_token,
                        Token start_value_token, Token end_value_token)  :
        left_expr(std::move(variable)), right_expr(std::move(expression)),
        start_left_value_token(start_left_value_token), end_left_value_token(end_left_value_token),
        start_value_token(start_value_token), end_value_token(end_value_token) {
            this->NAME = "LeftDereference";
        };

    pair<Memory*, string> resolveAssignmentTarget(Node* node) {
        
        if (node->NAME == "Literal") {
            auto memory_object = ((NodeLiteral*)node)->get_memory_object();
            if (!memory_object) {
                ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, ((NodeLiteral*)node)->name);
            }
            return {static_cast<Memory*>(memory_object->memory_pointer), ((NodeLiteral*)node)->name};
        }
        else if (node->NAME == "NameResolutionLiteral") {
            NodeNameResolution* resolution = (NodeNameResolution*)node;

            // Получаем namespace
            Value ns_value = resolution->namespace_expr->eval();

            if (*ns_value.type != STANDART_TYPE::NAMESPACE) {
                cout << "ERROR TYPE" << endl;
                exit(0);
            }

            auto ns = any_cast<Namespace>(ns_value.data);

            // Если есть цепочка, идем по ней
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            if (full_chain.size() > 1) {
                // Ищем конечный namespace через цепочку
                Memory* current_memory = ns.memory.get();

                // Проходим по всем промежуточным namespace кроме последнего
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& ns_name = full_chain[i];

                    if (!current_memory->check_literal(ns_name)) {
                        ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                    }

                    Value next_ns_value = current_memory->get_variable(ns_name)->value;

                    if (*next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                        cout << "ERROR TYPE" << endl;
                        exit(0);
                    }

                    current_memory = any_cast<Namespace>(next_ns_value.data).memory.get();
                }

                return {current_memory, full_chain.back()};
            }

            // Если цепочки нет, возвращаем текущий namespace и имя
            return {ns.memory.get(), resolution->current_name};
        }

        // Ошибка для неизвестного типа ноды
        cout << "ERROR TYPE" << endl;
        exit(0);
        return {nullptr, ""}; // Для компилятора
    }

    void exec() override {
        auto right_value = right_expr->eval();

        // НЕ перемещаем variable, используем сырой указатель
        Node* variable_ptr = left_expr.get();

        auto value = left_expr->eval();

        if (value.type->is_pointer) {
            auto address = any_cast<int>(value.data);
            auto object = STATIC_MEMORY.get_by_address(address);
            auto modifiers = object->modifiers;
        
        
        
            // Проверяем константность
            if (modifiers.is_const) {
                exit(-1);
            }

            // Проверяем типизацию для статических переменных
            if (modifiers.is_static) {
                auto wait_type = object->wait_type;
                auto value_type = *right_value.type;
                if (!IsTypeCompatible(wait_type, value_type)) {
                    exit(-1);
                }
            }

            // Выполняем присваивание
            STATIC_MEMORY.set_object_value(address, right_value);
        }
    }

    Value eval() override {}
};

struct NodeTypeof : public Node {
    unique_ptr<Node> expr;

    NodeTypeof(unique_ptr<Node> expr) : expr(std::move(expr)) {}

    void exec() override {}

    Value eval() override {
        auto value = expr->eval();
        return NewType(*value.type);
    }
};


// TODO AND TESTS
// +- UNSTABLE WORKS
struct NodeDelete : public Node {
    unique_ptr<Node> target;
    Token start_token;
    Token end_token;

    NodeDelete(unique_ptr<Node> target, Token start_token, Token end_token)
        : target(std::move(target)), start_token(start_token), end_token(end_token) {
        this->NAME = "Delete";
    }

    void exec() override {
        // Рекурсивно обрабатываем цель удаления

        if (target->NAME != "Literal" && target->NAME != "NameResolutionLiteral" && target->NAME != "Dereference") {
            ERROR::InvalidDeleteInstruction(start_token, end_token);
        }

        if (target->NAME == "Dereference") {
            auto value = ((NodeDereference*)(target.get()))->expr->eval();
            if (value.type->is_pointer) {
                auto address = any_cast<int>(value.data);
                STATIC_MEMORY.unregister_object(address);
            }
            
        } else {
            pair<Memory*, string> target_info = resolveDeleteTarget(target.get());

            Memory* target_memory = target_info.first;
            string target_name = target_info.second;

            if (!target_memory->check_literal(target_name)) {
                ERROR::UndefinedVariable(start_token);
            }

            // Выполняем удаление
            auto object = target_memory->get_variable(target_name);
            target_memory->delete_variable(target_name, object->address);
            STATIC_MEMORY.unregister_object(object->address);
        }
    }

    Value eval() override {}

    pair<Memory*, string> resolveDeleteTarget(Node* node) {
        if (node->NAME == "Literal") {
            auto memory_object = ((NodeLiteral*)node)->get_memory_object();
            if (!memory_object) {
                ERROR::UndefinedVariable(start_token);
            }
            return {static_cast<Memory*>(memory_object->memory_pointer), ((NodeLiteral*)node)->name};
        }
        else if (node->NAME == "NameResolutionLiteral") {
            NodeNameResolution* resolution = (NodeNameResolution*)node;

            // Получаем namespace
            Value ns_value = resolution->namespace_expr->eval();

            if (*ns_value.type != STANDART_TYPE::NAMESPACE) {
                cout << "ERROR TYPE" << endl;
                exit(0);
            }

            auto ns = any_cast<Namespace>(ns_value.data);

            // Если есть цепочка, идем по ней
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            if (full_chain.size() > 1) {
                // Ищем конечный namespace через цепочку
                Memory* current_memory = ns.memory.get();

                // Проходим по всем промежуточным namespace кроме последнего
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& ns_name = full_chain[i];

                    if (!current_memory->check_literal(ns_name)) {
                        ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                    }

                    Value next_ns_value = current_memory->get_variable(ns_name)->value;

                    if (*next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                        cout << "ERROR TYPE" << endl;
                        exit(0);
                    }

                    current_memory = any_cast<Namespace>(next_ns_value.data).memory.get();
                }

                return {current_memory, full_chain.back()};
            }

            // Если цепочки нет, возвращаем текущий namespace и имя
            return {ns.memory.get(), resolution->current_name};
        }

        // Ошибка для неизвестного типа ноды
        cout << "ERROR TYPE" << endl;
        exit(0);
        return {nullptr, ""}; // Для компилятора
    }
};


struct NodeIfExpr : public Node {
    unique_ptr<Node> condition;
    unique_ptr<Node> true_expr;
    unique_ptr<Node> else_expr;

    NodeIfExpr(unique_ptr<Node> condition, unique_ptr<Node> true_expr, unique_ptr<Node> else_expr)
        : condition(std::move(condition)), true_expr(std::move(true_expr)), else_expr(std::move(else_expr)) {
            NAME = "IfExpr";
    }

    Value eval() override {
        auto condition_value = condition->eval();
        if (*condition_value.type == STANDART_TYPE::BOOL) {
            if (any_cast<bool>(condition_value.data)) {
                return true_expr->eval();
            }
            if (else_expr) return else_expr->eval();
        } else if (*condition_value.type == STANDART_TYPE::NULL_T) {
            return else_expr->eval();
        } else {
            return true_expr->eval();
        }
    }

    void exec() override {}
};


struct NodeInput : public Node {
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;

    NodeInput(unique_ptr<Node> expr, Token start_token, Token end_token)
        : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
            NAME = "Input";
    }

    Value eval() override {
        auto value = expr->eval();
        if (*value.type == STANDART_TYPE::STRING) {
            cout << any_cast<string>(value.data);
        } else if (*value.type == STANDART_TYPE::CHAR) {
            cout << any_cast<char>(value.data);
        } else {
            ERROR::IncompartableTypeInput(start_token, end_token, *value.type);
        }

        string _input;
        getline(cin, _input);

        if (_input.empty())
            return NewNull();

        if (_input.length() == 1) {
            // Один символ - может быть char или digit
            char* ch = new char(_input[0]);
            if (isdigit(*ch)) {
                auto value = NewInt(atoi(ch));
                delete ch;
                return value;
            } 
            else { 
                auto value = NewChar(*ch);
                delete ch;
                return value;
            }
        } else {
            bool isNumber = true;
            bool isDOuble = false;
            for (char c : _input) {
                if (!isdigit(c) && c != '.') {
                    isNumber = false;
                    break;
                }
                if (c == '.') isDOuble = true;
            }
            if (isNumber) {
                if (isDOuble) {
                    auto value = NewDouble(atof(_input.c_str()));
                    return value;
                } else {
                    auto value = NewInt(atoi(_input.c_str()));
                    return value;
                }
            } else {
                auto value = NewString(_input);
                return value;
            }
        }
    }

    void exec() override {}
};

struct Context {
    Memory memory;
    vector<unique_ptr<Node>> nodes;

    Context() = default;

    // Конструктор перемещения
    Context(vector<unique_ptr<Node>>&& nodes, Memory&& mem)
        : nodes(std::move(nodes)), memory(std::move(mem)) {}

    Context(vector<unique_ptr<Node>>&& nodes, Memory& mem)
        : nodes(std::move(nodes)), memory(std::move(mem)) {}

    // Запрещаем копирование
    Context(const Context&) = delete;

    Context& operator=(const Context&) = delete;

    // Разрешаем перемещение
    Context(Context&& other) noexcept
        : nodes(std::move(other.nodes)), memory(std::move(other.memory)) {}

    Context& operator=(Context&& other) noexcept {
        if (this != &other) {
            nodes = std::move(other.nodes);
            memory = std::move(other.memory);
        }
        return *this;
    }

    void set_nodes(vector<unique_ptr<Node>>&& new_nodes) {
        nodes = std::move(new_nodes);  // Используем move
    }

    void set_memory(Memory&& new_memory) {
        memory = std::move(new_memory);  // Используем move
    }
};

struct ASTGenerator {
    vector<unique_ptr<Node>> nodes;
    TokenWalker& walker;

    unique_ptr<Node> ParseBaseVariableDecl(Memory& memory);
    unique_ptr<Node> ParseFinalVariableDecl(Memory& memory);
    unique_ptr<Node> ParseStaticVariableDecl(Memory& memory);
    unique_ptr<Node> ParseConstVariableDecl(Memory& memory);
    unique_ptr<Node> ParseGlobalVariableDecl(Memory& memory);
    unique_ptr<Node> ParseBlockDecl(Memory& memory, string modifier);

    unique_ptr<Node> ParseOut(Memory& memory);
    unique_ptr<Node> ParseOutLn(Memory& memory);
    unique_ptr<Node> ParseInput(Memory& memory);

    unique_ptr<Node> ParseDelete(Memory& memory);
    unique_ptr<Node> ParseWhile(Memory& memory);
    unique_ptr<Node> ParseDoWhile(Memory& memory);
    unique_ptr<Node> ParseFor(Memory& memory);

    unique_ptr<Node> ParseBlock(Memory& memory);
    unique_ptr<Node> ParseNameSpaceDecl(Memory& memory);
    unique_ptr<Node> ParseIf(Memory& memory);
    unique_ptr<Node> ParseIfExpr(Memory& memory);

    unique_ptr<Node> ParseNumber();
    unique_ptr<Node> ParseString();
    unique_ptr<Node> ParseChar();
    unique_ptr<Node> ParseBool();
    unique_ptr<Node> ParseNull();
    unique_ptr<Node> ParseBreak();
    unique_ptr<Node> ParseContinue();
    unique_ptr<Node> ParseLiteral(Memory& memory);


    unique_ptr<Node> ParseAddressOf(Memory& memory);
    unique_ptr<Node> ParseDereference(Memory& memory);
    unique_ptr<Node> ParseLeftDereference(Memory& memory);

    unique_ptr<Node> ParseTypeof(Memory& memory);

    unique_ptr<Node> ParseScopes(Memory& memory);

    unique_ptr<Node> ParseNameResolution(unique_ptr<Node> expression);

    Memory GLOBAL_MEMORY;


    ASTGenerator(TokenWalker& walker) : walker(walker) {}

    unique_ptr<Node> parse_primary_expression(Memory &memory) {
        Token current = *walker.get();
        if (walker.CheckType(TokenType::NUMBER))
            return ParseNumber();

        if (walker.CheckType(TokenType::STRING))
            return ParseString();

        if (walker.CheckType(TokenType::CHAR))
            return ParseChar();

        if (walker.CheckType(TokenType::LITERAL) && (walker.CheckValue("true") || walker.CheckValue("false")))
            return ParseBool();

        if (walker.CheckType(TokenType::LITERAL) && walker.CheckValue("null"))
            return ParseNull();

        if (walker.CheckType(TokenType::LITERAL) && walker.CheckValue("input"))
            return ParseInput(memory);

        if (walker.CheckValue("("))
            return ParseScopes(memory);

        if (walker.CheckValue("typeof") && walker.CheckValue("(", 1)) {
            return ParseTypeof(memory);
        }

        if (walker.CheckType(TokenType::LITERAL)) {
            auto expr = ParseLiteral(memory);
            while (true) {
                if (walker.CheckValue(":") && walker.CheckValue(":", 1)){
                    expr = ParseNameResolution(std::move(expr));
                    continue;
                }
                break;
            }
            return expr;
        }


        return nullptr;
    }

    unique_ptr<Node> parse_unary_expression(Memory &memory) {
        if (walker.CheckValue("-") || walker.CheckValue("+") 
        || walker.CheckValue("--") || walker.CheckValue("++") ||
            walker.CheckValue("!") || walker.CheckValue("not")) {
            string op = walker.get()->value;
            Token operator_token = *walker.get();
            walker.next(); // pass operator
            Token start = *walker.get();
            auto operand = parse_primary_expression(memory);
            Token end = *walker.get(-1);
            return make_unique<NodeUnary>(std::move(operand), start, end, operator_token);
        }
        if (walker.CheckValue("&")) {
            return ParseAddressOf(memory);
        }
        if (walker.CheckValue("*")) {
            return ParseDereference(memory);
        }

        return parse_primary_expression(memory);
    }


    // Вспомогательный метод для парсинга бинарных выражений с заданными операторами
    unique_ptr<Node> ParseBinaryLevel(
        Memory& memory, unique_ptr<Node> (ASTGenerator::*parseHigherLevel)(Memory&),
        const vector<string>& operators, string level_name
    ) {
        Token& start_token = *walker.get();
        auto left = (this->*parseHigherLevel)(memory);

        while (true) {
            bool found_operator = false;
            string op;

            for (const auto& candidate : operators) {
                if (walker.CheckValue(candidate)) {
                    found_operator = true;
                    op = candidate;
                    break;
                }
            }

            if (!found_operator) break;

            Token& op_token = *walker.get();
            walker.next(); // pass operator

            auto right = (this->*parseHigherLevel)(memory);
            if (!right) {
                ERROR::UnexpectedToken(*walker.get(), "expression after " + level_name + " operator");
            }

            Token& end_token = *(walker.get() - 1);
            left = make_unique<NodeBinary>(std::move(left), op, std::move(right),
                                        start_token, end_token, op_token);
        }

        return left;
    }

    unique_ptr<Node> parse_binary_expression_or(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_and,
                            {"or", "||"}, "logical OR");
    }

    unique_ptr<Node> parse_binary_expression_and(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_eq_ne_in_ni,
                            {"and", "&&"}, "logical AND");
    }

    unique_ptr<Node> parse_binary_expression_eq_ne_in_ni(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_less_more,
                            {"==", "!=", "<<", ">>"}, "equality/innary");
    }

    unique_ptr<Node> parse_binary_expression_less_more(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_modul,
                            {"<", ">", "<=", ">="}, "comparison");
    }

    unique_ptr<Node> parse_binary_expression_modul(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_sum_sub,
                            {"%"}, "modulus");
    }

    unique_ptr<Node> parse_binary_expression_sum_sub(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_mul_div,
                            {"+", "-"}, "addition/subtraction");
    }

    unique_ptr<Node> parse_binary_expression_mul_div(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_binary_expression_exp,
                            {"*", "/"}, "multiplication/division");
    }

    unique_ptr<Node> parse_binary_expression_exp(Memory &memory) {
        return ParseBinaryLevel(memory, &ASTGenerator::parse_unary_expression,
                            {"**", "|"}, "exponentiation/bitwise OR");
    }

    unique_ptr<Node> parse_higher_order_expressions(Memory &memory) {
        if (walker.CheckType(TokenType::LITERAL) && walker.CheckValue("if")) {
            return ParseIfExpr(memory);
        }

        return parse_binary_expression_or(memory);
    }

    unique_ptr<Node> parse_expression(Memory &memory) {
        return parse_higher_order_expressions(memory);
    }

    unique_ptr<Node> parse_left_statement(Memory &memory) {
        auto expr = parse_expression(memory);
    }

    unique_ptr<Node> parse_statement(Memory& memory) {
        Token current = *walker.get();
        if (current.type == TokenType::L_CURVE_BRACKET) {
            return ParseBlock(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "out") {
            return ParseOut(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "outln") {
            return ParseOutLn(memory);
        }
        // block declaration
        if ((current.type == TokenType::LITERAL) &&
            (current.value == "final" || current.value == "const" ||
            current.value == "static" || current.value == "global")) {

            string modifier = current.value;

            walker.next();

            if (walker.get()->type == TokenType::L_CURVE_BRACKET) {
                return ParseBlockDecl(memory, modifier);
            }
            walker.before();
            // Если не блочная декларация, продолжаем как обычно
        }
        if (current.type == TokenType::LITERAL && current.value == "let") {
            return ParseBaseVariableDecl(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "final") {
            return ParseFinalVariableDecl(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "static") {
            return ParseStaticVariableDecl(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "const") {
            return ParseConstVariableDecl(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "global") {
            return ParseGlobalVariableDecl(memory);
        }


        if (current.type == TokenType::LITERAL && current.value == "namespace") {
            return ParseNameSpaceDecl(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "if") {
            return ParseIf(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "del") {
            return ParseDelete(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "while") {
            return ParseWhile(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "do") {
            return ParseDoWhile(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "for") {
            return ParseFor(memory);
        }
        if (current.type == TokenType::LITERAL && current.value == "break") {
            return ParseBreak();
        }
        if (current.type == TokenType::LITERAL && current.value == "continue") {
            return ParseContinue();
        }
        if (current.type == TokenType::LITERAL) {

            auto start_left_value_token = *walker.get();
            auto left_expr = parse_primary_expression(memory);

            while (true) {
                if (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
                    left_expr = ParseNameResolution(std::move(left_expr));
                    continue;
                }
                break;
            }

            auto end_left_value_token = *walker.get(-1);

            if (!walker.CheckValue("="))
                ERROR::UnexpectedToken(*walker.get(), "'='");
            walker.next();
            auto start_value_token = *walker.get();
            auto expr = parse_expression(memory);

            if (!expr)
                ERROR::UnexpectedToken(*walker.get(), "expression");


            if (!walker.CheckValue(";"))
                ERROR::UnexpectedToken(*walker.get(), "';'");
            auto end_value_token = *walker.get();
            walker.next();


            return make_unique<NodeVariableEqual>(std::move(left_expr), std::move(expr), start_left_value_token, end_left_value_token, start_value_token, end_value_token);
        }
        if (current.value == "*") {
            return ParseLeftDereference(memory);
        }
        return nullptr;
    }

    void GenerateStandartTypes() {
        GLOBAL_MEMORY.add_object("Int", NewType("Int"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("Double", NewType("Double"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("Char", NewType("Char"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("String", NewType("String"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("Bool", NewType("Bool"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("Type", NewType("Type"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("Null", NewType("Null"), STANDART_TYPE::TYPE, true, true, true, true);
        GLOBAL_MEMORY.add_object("Void", NewType("Void"), STANDART_TYPE::TYPE, true, true, true, true);
    }

    inline void parse() {
        GenerateStandartTypes();

        while (!walker.isEnd()) {
            auto stmt = parse_statement(GLOBAL_MEMORY);
            if (stmt) {
                this->nodes.push_back(std::move(stmt));
            }
        }
    }

    // В ASTGenerator
    Context run() {
        parse();

        // Создаем копию GLOBAL_MEMORY
        Memory memory_copy;

        // Копируем все переменные
        for (const auto& [name, obj] : GLOBAL_MEMORY.string_pool) {
            memory_copy.add_object(name, obj->value, obj->wait_type,
                                    obj->modifiers.is_const, obj->modifiers.is_static,
                                    obj->modifiers.is_final, obj->modifiers.is_global);
        }

        return Context(std::move(nodes), std::move(memory_copy));
    }
};

struct ContextExecutor {
    Context context;

    ContextExecutor(Context context) : context(std::move(context)) {}
    void run() {
        for (int i = 0; i < (context.nodes).size(); i++) {
            (context.nodes)[i]->exec();
        }
    }
};


unique_ptr<Node> ASTGenerator::ParseInput(Memory& memory) {
    walker.next(); // pass 'input' token
    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();
    auto start_token = *walker.get();
    auto expr = parse_expression(memory);
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    auto end_token = *walker.get();
    walker.next();
    return make_unique<NodeInput>(std::move(expr), start_token, end_token);
}


unique_ptr<Node> ASTGenerator::ParseLeftDereference(Memory& memory) {
    walker.next(); // pass '*' token

    auto start_left_value_token = *walker.get();
    auto left_expr = parse_primary_expression(memory);

    while (true) {
        if (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
            left_expr = ParseNameResolution(std::move(left_expr));
            continue;
        }
        break;
    }

    auto end_left_value_token = *walker.get(-1);

    if (!walker.CheckValue("="))
        ERROR::UnexpectedToken(*walker.get(), "'='");
    walker.next();
    auto start_value_token = *walker.get();
    auto expr = parse_expression(memory);

    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");


    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    auto end_value_token = *walker.get();
    walker.next();

    return make_unique<NodeLeftDereference>(std::move(left_expr), std::move(expr), start_left_value_token, end_left_value_token, start_value_token, end_value_token);
}

unique_ptr<Node> ASTGenerator::ParseTypeof(Memory& memory) {
    walker.next(); // pass 'typeof' token
    walker.next(); // pass '(' token
    auto expr = parse_expression(memory);
    if (!walker.CheckValue(")")) 
        ERROR::UnexpectedToken(*walker.get(), "')' after typeof");
    walker.next();
    return make_unique<NodeTypeof>(std::move(expr));
}

unique_ptr<Node> ASTGenerator::ParseAddressOf(Memory& memory) {
    walker.next(); // pass '&' token
    auto expr = parse_primary_expression(memory);
    return make_unique<NodeAddressOf>(std::move(expr));
}

unique_ptr<Node> ASTGenerator::ParseDereference(Memory& memory) {
    walker.next(); // pass '*' token
    if (walker.CheckValue("(")) {
        walker.next();
        auto expr = parse_expression(memory);
        if (!expr) ERROR::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckValue(")")) ERROR::UnexpectedToken(*walker.get(), "')'");
        walker.next();
        return make_unique<NodeDereference>(std::move(expr));
    } else {
        auto expr = parse_primary_expression(memory);
        if (!expr) ERROR::UnexpectedToken(*walker.get(), "primary expression or (expression)");
        return make_unique<NodeDereference>(std::move(expr));
    }
}


// Parse block declaration
//
// <static global final const {
//     <statement>;
// }
unique_ptr<Node> ASTGenerator::ParseBlockDecl(Memory& memory, string modifier) {
    if (!walker.CheckValue("{")) {
        ERROR::UnexpectedToken(*walker.get(), "'{' after block declaration");
    }
    walker.next();

    // Создаем блок для хранения всех объявлений
    vector<unique_ptr<Node>> declarations;

    // Парсим все объявления внутри блока
    while (!walker.CheckValue("}")) {
        auto stmt = parse_statement(memory);
        if (!stmt) {
            ERROR::UnexpectedToken(*walker.get(), "statement");
        }

        declarations.push_back(std::move(stmt));
    }

    if (!walker.CheckValue("}"))
        ERROR::UnexpectedToken(*walker.get(), "'}'");
    walker.next();

    auto node = make_unique<NodeBlockDecl>(std::move(declarations));
    if (modifier == "static")
        node->is_static = true;

    if (modifier == "const")
        node->is_const = true;

    if (modifier == "global")
        node->is_global = true;

    if (modifier == "final")
        node->is_final = true;

    // Возвращаем блок с примененными модификаторами
    return node;
}

// Parse continue statement
//
// continue;
unique_ptr<Node> ASTGenerator::ParseContinue() {
    walker.next(); // pass 'break'

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    return make_unique<NodeContinue>();
}

// Parse break statement
//
// break;
unique_ptr<Node> ASTGenerator::ParseBreak() {
    walker.next(); // pass 'break'

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    return make_unique<NodeBreak>();
}

// Parse for statement
//
// for (<init>; <check>; <update>;) <statement>;
unique_ptr<Node> ASTGenerator::ParseFor(Memory& memory) {
    walker.next(); // pass 'for' token

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto init_state = parse_statement(memory);

    auto check_expr = parse_expression(memory);

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    auto update_state = parse_statement(memory);
    if (!update_state)
        ERROR::UnexpectedToken(*walker.get(), "update statement");


    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    auto body = parse_statement(memory);

    return make_unique<NodeFor>(std::move(init_state), std::move(check_expr), std::move(update_state), std::move(body));
}

// Parse while statement
//
// while (<expr>) <statement>;
unique_ptr<Node> ASTGenerator::ParseWhile(Memory& memory) {
    walker.next(); // pass 'while' token

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto expr = parse_expression(memory);

    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    auto state = parse_statement(memory);

    return make_unique<NodeWhile>(std::move(expr), std::move(state));
}

// Parse do-while statement
//
// do <statement> while (<expr>);
unique_ptr<Node> ASTGenerator::ParseDoWhile(Memory& memory) {
    walker.next(); // pass 'do' token

    auto state = parse_statement(memory);

    if (!walker.CheckValue("while"))
        ERROR::UnexpectedToken(*walker.get(), "'while'");
    walker.next();

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto expr = parse_expression(memory);

    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    return make_unique<NodeDoWhile>(std::move(expr), std::move(state));
}

// Parse delete statement
//
// del <literal>;
unique_ptr<Node> ASTGenerator::ParseDelete(Memory& memory) {
    walker.next(); // pass 'del' token

    Token start_token = *walker.get();

    // Парсим выражение, которое нужно удалить
    auto target_expr = parse_expression(memory);

    Token end_token = *walker.get(-1);

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");

    walker.next(); // pass ';'

    return make_unique<NodeDelete>(std::move(target_expr), start_token, end_token);
}

// Parse name resolution
//
// A::B::C
unique_ptr<Node> ASTGenerator::ParseNameResolution(unique_ptr<Node> expression) {
    vector<string> chain;
    Token start = *walker.get(-1);
    // Собираем всю цепочку имен
    while (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
        walker.next(); // pass first ':'
        walker.next(); // pass second ':'

        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "literal");

        string literal_name = walker.get()->value;
        chain.push_back(literal_name);
        walker.next();
    }

    // Если есть цепочка, создаем ноду разрешения
    if (!chain.empty()) {
        // Первый элемент цепочки становится первым уровнем
        string first_name = chain[0];
        vector<string> remaining_chain(chain.begin() + 1, chain.end());
        Token end = *walker.get();
        expression = make_unique<NodeNameResolution>(std::move(expression), first_name, start, end, remaining_chain);
    }

    return expression;
}

// Parse namespace decl
//
// namespace <name> <statement>
unique_ptr<Node> ASTGenerator::ParseNameSpaceDecl(Memory& memory) {
    walker.next(); // pass 'namespace' token

    if (!walker.CheckType(TokenType::LITERAL))
        ERROR::UnexpectedToken(*walker.get(), "namespace identifier");
    Token decl_token = *walker.get();
    string namespace_name = walker.get()->value;
    walker.next();

    // Создаем namespace
    auto namespace_node = make_unique<NodeNamespaceDecl>(memory, nullptr, namespace_name, decl_token);

    auto block = parse_statement(*namespace_node->namespace_memory);
    namespace_node->statement = std::move(block);

    return namespace_node;
}

unique_ptr<Node> ASTGenerator::ParseIfExpr(Memory& memory) {
    walker.next(); // pass 'if' token

    if (!walker.CheckType(TokenType::L_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "(");
    walker.next();

    auto eq_expr = parse_expression(memory);
    if (!eq_expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), ")");
    walker.next();

    if (!walker.CheckType(TokenType::L_CURVE_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "{");
    walker.next();

    auto true_expr = parse_expression(memory);
    if (!true_expr) ERROR::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "}");
    walker.next();

    unique_ptr<Node> else_expr;
    if (walker.CheckValue("else")) {
        walker.next();
        if (!walker.CheckType(TokenType::L_CURVE_BRACKET))
            ERROR::UnexpectedToken(*walker.get(), "{");
        walker.next();
        else_expr = parse_expression(memory);
        if (!else_expr) ERROR::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
            ERROR::UnexpectedToken(*walker.get(), "}");
        walker.next();
    }

    return make_unique<NodeIfExpr>(std::move(eq_expr), std::move(true_expr), std::move(else_expr));
}

// Parse if
//
// if (expr) <statement>
// else <statement>
unique_ptr<Node> ASTGenerator::ParseIf(Memory& memory) {
    walker.next(); // pass 'if' token

    if (!walker.CheckType(TokenType::L_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "(");
    walker.next();

    auto eq_expr = parse_expression(memory);
    if (!eq_expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), ")");
    walker.next();

    auto true_state = parse_statement(memory);
    unique_ptr<Node> else_state;

    if (walker.CheckValue("else")) {
        walker.next();
        else_state = parse_statement(memory);
        if (!else_state)
            ERROR::UnexpectedToken(*walker.get(), "statement or statement block");
    }

    return make_unique<NodeIf>(std::move(eq_expr), std::move(true_state), std::move(else_state));
}

// Parse block
//
// {
//  <statement>
//  ...
// }
unique_ptr<Node> ASTGenerator::ParseBlock(Memory& memory) {
    walker.next();
    vector<unique_ptr<Node>> nodes_array;
    while (!walker.CheckType(TokenType::R_CURVE_BRACKET)) {
        nodes_array.push_back(parse_statement(memory));
    }
    walker.next();
    return make_unique<NodeBlock>(nodes_array);
}

// Literal parsing
//
// value, a, name
unique_ptr<Node> ASTGenerator::ParseLiteral(Memory& memory) {
    auto node = make_unique<NodeLiteral>(*walker.get(), memory);
    walker.next();
    return node;
}

// Number parsing
//
// <expr> = <10, .10, 134.134>
unique_ptr<Node> ASTGenerator::ParseNumber() {
    auto node = make_unique<NodeNumber>(*walker.get());
    walker.next();
    return node;
}

// String parsing
//
// <expr> = <"Hello", "World">
unique_ptr<Node> ASTGenerator::ParseString() {
    auto node = make_unique<NodeString>(walker.get()->value);
    walker.next();
    return node;
}

// Char parsing
//
// <expr> = <'a', 'b', 'c'>
unique_ptr<Node> ASTGenerator::ParseChar() {
    auto node = make_unique<NodeChar>(walker.get()->value[0]);
    walker.next();
    return node;
}

// Bool parsing
//
// <expr> = <true, false>
unique_ptr<Node> ASTGenerator::ParseBool() {
    auto node = make_unique<NodeBool>(*walker.get());
    walker.next();
    return node;
}

// Null parsing
//
// <expr> = <true, false>
unique_ptr<Node> ASTGenerator::ParseNull() {
    auto node = make_unique<NodeNull>();
    walker.next();
    return node;
}

// Scopes parsing
//
// (<expr>)
unique_ptr<Node> ASTGenerator::ParseScopes(Memory& memory) {
    walker.next(); // pass '(' token
    auto expr = parse_expression(memory);
    if (!expr) {
        ERROR::UnexpectedToken(*walker.get(), "expression");
    }
    if (!walker.CheckValue(")")) {
        ERROR::UnexpectedToken(*walker.get(), "')'");
    }
    walker.next(); // pass ')' token
    return make_unique<NodeScopes>(std::move(expr));
}


// Variable declaration parsing
// Parse base variable declaration
//
// let <name>:<type expr, none> = <expr>;
//
unique_ptr<Node> ASTGenerator::ParseBaseVariableDecl(Memory& memory) {
    walker.next(); // pass 'let' token

    // Check variable name
    if (!walker.CheckType(TokenType::LITERAL)) {
        ERROR::UnexpectedToken(*walker.get(), "variable name");
    }

    // save variable token
    Token variable_token = *walker.get();
    string var_name = walker.get()->value;
    walker.next(); // pass variable name token

    unique_ptr<Node> type_expr = nullptr; // type expression
    Token type_start_token;
    Token type_end_token;

    

    if (walker.CheckValue(":")) {
        walker.next(); // pass ':' token
        type_start_token = *walker.get();
        type_expr = parse_expression(memory);
        type_end_token = *walker.get(-1);
    } else {
        type_start_token = *walker.get();
        type_end_token = *walker.get();
    }
       

    bool nullable = false;
    if (walker.CheckValue("?")) {
        nullable = true;
        walker.next(); // pass '?' token
    }

    Token start_expr_token = *walker.get();
    Token end_expr_token = *walker.get();


    if (walker.CheckValue(";")) {
        auto expr = make_unique<NodeNull>();
        walker.next();
        return make_unique<NodeBaseVariableDecl>(var_name, std::move(expr), memory, variable_token,
                                             std::move(type_expr), type_start_token, type_end_token, nullable, start_expr_token, end_expr_token);
    }

    if (!walker.CheckValue("="))
        ERROR::UnexpectedToken(*walker.get(), "'='");
    walker.next(); // pass '=' token

    start_expr_token = *walker.get();
    auto expr = parse_expression(memory);
    end_expr_token = *walker.get(-1);


    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next(); // pass ';' token

    return make_unique<NodeBaseVariableDecl>(var_name, std::move(expr), memory, variable_token,
                                             std::move(type_expr), type_start_token, type_end_token, nullable, start_expr_token, end_expr_token);
}

// Variable declaration parsing
// Parse base variable declaration
//
// final let <name>:<type expr, none> = <expr>;
//
unique_ptr<Node> ASTGenerator::ParseFinalVariableDecl(Memory& memory) {
    walker.next(); // pass 'final' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }


    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NAME == "BaseVariableDecl") {
        ((NodeBaseVariableDecl*)decl.get())->is_final = true;
    } else if (decl->NAME == "NamespaceDecl") {
        ((NodeNamespaceDecl*)decl.get())->is_final = true;
    } else if (decl->NAME == "BlockDecl") {
        ((NodeBlockDecl*)decl.get())->is_final = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }

    return decl;
}

// Parsing static declarations
//
// static <statement>
unique_ptr<Node> ASTGenerator::ParseStaticVariableDecl(Memory& memory) {
    walker.next(); // pass 'static' token

    if (!walker.CheckValue("final") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NAME == "BaseVariableDecl") {
        ((NodeBaseVariableDecl*)decl.get())->is_static = true;
    } else if (decl->NAME == "NamespaceDecl") {
        ((NodeNamespaceDecl*)decl.get())->is_static = true;
    } else if (decl->NAME == "BlockDecl") {
        ((NodeBlockDecl*)decl.get())->is_static = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }

    return decl;
}

// Parsing const declarations
//
// const <statement>
unique_ptr<Node> ASTGenerator::ParseConstVariableDecl(Memory& memory) {
    walker.next(); // pass 'const' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("final") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NAME == "BaseVariableDecl") {
        ((NodeBaseVariableDecl*)decl.get())->is_const = true;
    } else if (decl->NAME == "NamespaceDecl") {
        ((NodeNamespaceDecl*)decl.get())->is_const = true;
    } else if (decl->NAME == "BlockDecl") {
        ((NodeBlockDecl*)decl.get())->is_const = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }

    return decl;
}

// Parsing global declarations
//
// global <statement>
unique_ptr<Node> ASTGenerator::ParseGlobalVariableDecl(Memory& memory) {
    walker.next(); // pass 'const' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("final") &&
        !walker.CheckValue("namespace")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NAME == "BaseVariableDecl") {
        ((NodeBaseVariableDecl*)decl.get())->is_global = true;
    } else if (decl->NAME == "NamespaceDecl") {
        ((NodeNamespaceDecl*)decl.get())->is_global = true;
    } else if (decl->NAME == "BlockDecl") {
        ((NodeBlockDecl*)decl.get())->is_global = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }
    return decl;
}

// Base out parsing
//
// out <expr>;
unique_ptr<Node> ASTGenerator::ParseOut(Memory& memory) {
    walker.next(); // pass 'out'
    auto expr = parse_expression(memory);
    if (!walker.CheckValue(";")) {
        ERROR::UnexpectedToken(*walker.get(), "';'");
    }
    walker.next();
    return make_unique<NodeBaseOut>(std::move(expr));
}

// Base out parsing
//
// outln <expr>;
unique_ptr<Node> ASTGenerator::ParseOutLn(Memory& memory) {
    walker.next(); // pass 'outln'
    auto expr = parse_expression(memory);
    if (!walker.CheckValue(";")) {
        ERROR::UnexpectedToken(*walker.get(), "';'");
    }
    walker.next();
    return make_unique<NodeBaseOutLn>(std::move(expr));
}
