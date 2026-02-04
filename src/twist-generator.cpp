#include "twist-tokenwalker.cpp"
#include "twist-namespace.cpp"
#include "twist-functions.cpp"
#include <vcruntime_startup.h>
#include "twist-lambda.cpp"
#include "twist-tokens.cpp"
#include "twist-errors.cpp"
#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "twist-args.cpp"

#include <algorithm>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <any>


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"



#define NO_EXEC \
    void exec_from(Memory& _memory) override {} \

#define NO_EVAL \
    Value eval_from(Memory& _memory) override {} \


using namespace std;



struct NodeNumber : public Node { NO_EXEC
    enum ValueType { INT, DOUBLE } type;
    int64_t int_value;
    float double_value;


    NodeNumber(Token& token) {
        this->NODE_TYPE = NodeTypes::NODE_NUMBER;
        string value = token.value;

        size_t dot_count = count(value.begin(), value.end(), '.');
        if (dot_count > 1) {
            ERROR::InvalidNumber(token, value);
        }

        if (dot_count == 1) {
            value = value.substr(0, value.find(".")) + "," + value.substr(value.find(".") + 1);
            this->double_value = atof(value.c_str());
            this->type = DOUBLE;
        } else {
            this->int_value = stoll(value);
            this->type = INT;
        }
    }

    Value eval_from(Memory& _memory) override {
        if (type == INT) {
            return NewInt(int_value);
        } else {
            return NewDouble(double_value);
        }
    }
    
};

struct NodeString : public Node { NO_EXEC
    string value;
    NodeString(string value) {
        this->NODE_TYPE = NodeTypes::NODE_STRING;
        this->value = value;
    }

    Value eval_from(Memory& _memory) override {
        return NewString(value);
    }
};

struct NodeChar : public Node { NO_EXEC
    char value;
    NodeChar(char value) {
        this->NODE_TYPE = NodeTypes::NODE_CHAR;
        this->value = value;
    }

    Value eval_from(Memory& _memory) override {
        return NewChar(value);
    }
};

struct NodeBool : public Node { NO_EXEC
    bool value;

    NodeBool(Token& token) {
        this->NODE_TYPE = NodeTypes::NODE_BOOL;
        if (token.value == "true") value = true;
        if (token.value == "false") value = false;
    }

    Value eval_from(Memory& _memory) override { return Value(STANDART_TYPE::BOOL, value); }
};

struct NodeNull : public Node { NO_EXEC
    Value value = NewNull();

    NodeNull() {
        this->NODE_TYPE = NodeTypes::NODE_NULL;
    }

    Value eval_from(Memory& _memory) override {
        return this->value;
    }
};

struct NodeLiteral : public Node { NO_EXEC
    string name;
    Token& token;
    Memory& memory;

    NodeLiteral(Token& token, Memory& memory) : token(token), memory(memory) {
        name = token.value;
        this->NODE_TYPE = NODE_LITERAL;
    }

    Value eval_from(Memory& _memory) override {
        if (!_memory.check_literal(name))
            ERROR::UndefinedVariable(token);

        return _memory.get_variable(name)->value;
    }
    MemoryObject* get_memory_object() {
        return memory.get_variable(name);
    }
    
};

struct NodeValueHolder : public Node { NO_EXEC
    Value value;

    NodeValueHolder(Value val) : value(std::move(val)) {
        this->NODE_TYPE = NodeTypes::NODE_VALUE_HOLDER;
    }

    Value eval_from(Memory& _memory) override {
        return value;
    }
};

struct NodeNameResolution : public Node { NO_EXEC
    unique_ptr<Node> namespace_expr;
    string current_name;
    vector<string> remaining_chain; // Оставшаяся цепочка имен

    Token start;
    Token end;

    NodeNameResolution(unique_ptr<Node> namespace_expr, const string& current_name, Token start, Token end,
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
            cout << "ERROR TYPE" << endl;
            exit(0);
        }

        auto ns = any_cast<Namespace>(ns_value.data);

        // Проверяем существование переменной/namespace
        if (!ns.memory.check_literal(current_name)) {
            ERROR::UndefinedLeftVariable(start, end, current_name);
        }

        // Получаем значение
        Value result = ns.memory.get_variable(current_name)->value;

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

            return next_node->eval_from(_memory);
        }

        return result;
    }

    
};

// + SUCCESS WORKS
struct NodeScopes : public Node { NO_EXEC
    unique_ptr<Node> expression;

    NodeScopes(unique_ptr<Node> expr) : expression(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_SCOPES;
    }

    Value eval_from(Memory& _memory) override {
        return expression->eval_from(_memory);
    }
};


// + SUCCESS WORKS
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
                long double v = any_cast<long double>(value.data);
                return NewDouble(-v);
            } else if (op == "+") {
                long double v = any_cast<long double>(value.data);
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

// + SUCCESS WORKS
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

        } else if ((left_val.type == STANDART_TYPE::DOUBLE || left_val.type == STANDART_TYPE::INT) &&
                 (right_val.type == STANDART_TYPE::DOUBLE || right_val.type == STANDART_TYPE::INT)) {
            float l = (left_val.type == STANDART_TYPE::DOUBLE) ? any_cast<float>(left_val.data) : static_cast<long double>(any_cast<int64_t>(left_val.data));
            float r = (right_val.type == STANDART_TYPE::DOUBLE) ? any_cast<float>(right_val.data) : static_cast<long double>(any_cast<int64_t>(right_val.data));

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
        
        
        } else
            ERROR::UnsupportedBinaryOperator(start_token, end_token, op_token, left_val, right_val);
    }
};

// + SUCCESS WORKS
struct NodeBaseVariableDecl : public Node { NO_EVAL
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
            this->NODE_TYPE = NodeTypes::NODE_VARIABLE_DECLARATION;
            this->value_expr = std::move(expr);
            this->type_expr = std::move(type_expr);
        }

    void exec_from(Memory& _memory) override {
        Value value = value_expr->eval_from(_memory);

        if (_memory.check_literal(var_name)) {
            if (_memory.is_final(var_name)) {
                ERROR::VariableAlreadyDefined(decl_token, var_name);
            }
            auto addr = _memory.get_variable(var_name)->address;
            STATIC_MEMORY.unregister_object(addr);
            _memory.delete_variable(var_name);

        }

        Type static_type = value.type;
        if (is_static) {
            if (!type_expr) ERROR::WaitedTypeExpression(type_end_token);

            if (type_expr->NODE_TYPE == NodeTypes::NODE_LITERAL && ((NodeLiteral*)type_expr.get())->name == "auto") {
                static_type = value.type;
            } else {
                auto type_value = type_expr->eval_from(_memory);

                if (type_value.type != STANDART_TYPE::TYPE)
                    ERROR::InvalidType(type_start_token, type_end_token);

                static_type = any_cast<Type>(type_value.data);
                if (nullable)
                    static_type = static_type | STANDART_TYPE::NULL_T;

            }

            // Используем IsTypeCompatible вместо прямого сравнения
            if (!IsTypeCompatible(static_type, value.type)) {
                ERROR::IncompartableTypeVarDeclaration(type_start_token, type_end_token, start_expr_token, end_expr_token, static_type, value.type);
            }
        }

        MemoryObject* object = CreateMemoryObject(value, static_type, is_const, is_static, is_final, is_global, &_memory);
        STATIC_MEMORY.register_object(object);
        _memory.add_object(var_name, object);
    }
    
};

// + SUCCESS WORKS
struct NodeBaseOut : public Node { NO_EVAL
    vector<unique_ptr<Node>> expression;
    NodeBaseOut(vector<unique_ptr<Node>> expr) : expression(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_OUT;
    }

    void print(std::ostream& buf, Value value) {
        if (value.type == STANDART_TYPE::INT) {
            buf << any_cast<int64_t>(value.data);
        } else if (value.type == STANDART_TYPE::DOUBLE) {
            buf << any_cast<float>(value.data);
        } else if (value.type == STANDART_TYPE::BOOL) {
            buf << (any_cast<bool>(value.data) ? "true" : "false");
        } else if (value.type == STANDART_TYPE::TYPE) {
            buf << any_cast<Type>(value.data).pool;
        } else if (value.type == STANDART_TYPE::NULL_T) {
            buf << "null";
        } else if (value.type == STANDART_TYPE::NAMESPACE) {
            buf << any_cast<Namespace>(value.data).name;
        } else if (value.type == STANDART_TYPE::STRING) {
            buf << any_cast<string>(value.data);
        } else if (value.type == STANDART_TYPE::CHAR) {
            buf << any_cast<char>(value.data);
        } else if (value.type == STANDART_TYPE::LAMBDA) {
            buf << "Lambda(";
            auto lambda = any_cast<Lambda*>(value.data);
            for (int i = 0; i < lambda->arguments.size(); i++) {
                buf << lambda->arguments[i]->name;
                if (i != lambda->arguments.size() - 1) buf << ", ";
            }
            buf << ")";
        } else if (value.type.is_pointer()) {
            buf << value.type.pool << "[0x" << any_cast<int>(value.data) << "]";
        }
    }

    void exec_from(Memory& _memory) override {
        for (auto& expr : expression) {
            auto value = expr->eval_from(_memory);
            print(std::cout, value);
        }
    }
};

// + SUCCESS WORKS
struct NodeBaseOutLn : public Node { NO_EVAL
    vector<unique_ptr<Node>> expression;
    NodeBaseOutLn(vector<unique_ptr<Node>> expr) : expression(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_OUTLN;
    }

    void print(std::ostream& buf, Value value) {
        if (value.type == STANDART_TYPE::INT) {
            buf << any_cast<int64_t>(value.data);
        } else if (value.type == STANDART_TYPE::DOUBLE) {
            buf << any_cast<float>(value.data);
        } else if (value.type == STANDART_TYPE::BOOL) {
            buf << (any_cast<bool>(value.data) ? "true" : "false");
        } else if (value.type == STANDART_TYPE::TYPE) {
            buf << any_cast<Type>(value.data).pool;
        } else if (value.type == STANDART_TYPE::NULL_T) {
            buf << "null";
        } else if (value.type == STANDART_TYPE::NAMESPACE) {
            buf << any_cast<Namespace>(value.data).name;
        } else if (value.type == STANDART_TYPE::STRING) {
            buf << any_cast<string>(value.data);
        } else if (value.type == STANDART_TYPE::CHAR) {
            buf << any_cast<char>(value.data);
        } else if (value.type == STANDART_TYPE::LAMBDA) {
            buf << "Lambda(";
            auto lambda = any_cast<Lambda*>(value.data);
            for (int i = 0; i < lambda->arguments.size(); i++) {
                buf << lambda->arguments[i]->name;
                if (i != lambda->arguments.size() - 1) buf << ", ";
            }
            buf << ")";
        } else if (value.type.is_pointer()) {
            buf << value.type.pool << "[0x" << any_cast<int>(value.data) << "]";
        }
    }

    void exec_from(Memory& _memory) override {
        for (auto& expr : expression) {
            auto value = expr->eval_from(_memory);
            print(std::cout, value);
        }
        std::cout << '\n';
        std::cout.flush();
    }
};

// + UNSTABLE WORKS
struct NodeVariableEqual : public Node { NO_EVAL
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
        start_value_token(start_value_token), end_value_token(end_value_token) {
            this->NODE_TYPE = NodeTypes::NODE_VARIABLE_EQUAL;
        };

    void exec_from(Memory& _memory) override {
        
        auto right_value = expression->eval_from(_memory);
        

        // НЕ перемещаем variable, используем сырой указатель
        Node* variable_ptr = variable.get();
        
        // Получаем целевую память и имя переменной
        pair<Memory, string> target = resolveAssignmentTargetMemory(variable_ptr, _memory);
        

        Memory target_memory = target.first;
        string target_var_name = target.second;

       

        if (!target_memory.check_literal(target_var_name))
            ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, target_var_name);
         
        // Проверяем константность
        if (target_memory.is_const(target_var_name)) {
            ERROR::ConstRedefinition(start_left_value_token, end_value_token, target_var_name);
        }
        
        // Проверяем типизацию для статических переменных
        if (target_memory.is_static(target_var_name)) {
            auto wait_type = target_memory.get_wait_type(target_var_name);
            auto value_type = right_value.type;
            if (!IsTypeCompatible(wait_type, value_type)) {
                ERROR::StaticTypesMisMatch(start_left_value_token, end_value_token, wait_type, value_type);
            }
        }
        
        // Выполняем присваивание
        target_memory.set_object_value(target_var_name, right_value);
    }

    pair<Memory, string> resolveAssignmentTargetMemory(Node* node, Memory& _memory) {
        
        if (node->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            
            if (!_memory.check_literal(((NodeLiteral*)node)->name)) {
                ERROR::UndefinedLeftVariable(start_left_value_token, end_left_value_token, ((NodeLiteral*)node)->name);
            }
            return {_memory, ((NodeLiteral*)node)->name};
        }
        else if (node->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            NodeNameResolution* resolution = (NodeNameResolution*)node;

            // Получаем namespace
            Value ns_value = resolution->namespace_expr->eval_from(_memory);

            if (ns_value.type != STANDART_TYPE::NAMESPACE) {
                cout << "ERROR TYPE" << endl;
                exit(0);
            }

            auto ns = any_cast<Namespace>(ns_value.data);

            // Если есть цепочка, идем по ней
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            if (full_chain.size() > 1) {
                // Ищем конечный namespace через цепочку
                Memory current_memory = ns.memory;

                // Проходим по всем промежуточным namespace кроме последнего
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& ns_name = full_chain[i];

                    if (!current_memory.check_literal(ns_name)) {
                        ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                    }

                    Value next_ns_value = current_memory.get_variable(ns_name)->value;

                    if (next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                        cout << "ERROR TYPE" << endl;
                        exit(0);
                    }

                    current_memory = any_cast<Namespace>(next_ns_value.data).memory;
                }

                return {current_memory, full_chain.back()};
            }

            // Если цепочки нет, возвращаем текущий namespace и имя
            return {ns.memory, resolution->current_name};
        }

        // Ошибка для неизвестного типа ноды
        cout << "ERROR TYPE" << endl;
        exit(0);

    }
};

// + SUCCESS WORKS
struct NodeBlock : public Node { NO_EVAL
    vector<unique_ptr<Node>> nodes_array;

    NodeBlock(vector<unique_ptr<Node>> &nodes_array) : nodes_array(std::move(nodes_array)) {
        this->NODE_TYPE = NodeTypes::NODE_BLOCK_OF_NODES;
    }

    void exec_from(Memory& _memory) override {
        for (int i = 0; i < nodes_array.size(); i++) {
            nodes_array[i]->exec_from(_memory);
        }
    }
};

// + SUCCESS WORKS
struct NodeIf : public Node { NO_EVAL
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> true_statement;
    unique_ptr<Node> else_statement = nullptr;

    NodeIf(unique_ptr<Node> eq_expression, unique_ptr<Node> true_statement, unique_ptr<Node> else_statement = nullptr) :
        eq_expression(std::move(eq_expression)), true_statement(std::move(true_statement)),
        else_statement(std::move(else_statement)) {
        this->NODE_TYPE = NodeTypes::NODE_IF;
    }

    void exec_from(Memory& _memory) override {
        auto value = eq_expression->eval_from(_memory);

        if (value.type == STANDART_TYPE::BOOL) {
            if (any_cast<bool>(value.data)) {
                true_statement->exec_from(_memory);
                return;
            }
            if (else_statement) else_statement->exec_from(_memory);
        } else if (value.type == STANDART_TYPE::NULL_T) {
            else_statement->exec_from(_memory);
        } else {
            true_statement->exec_from(_memory);
        }
    }
};

// + SUCCESS WORKS
struct NodeNamespaceDecl : public Node { NO_EVAL
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
            this->NODE_TYPE = NodeTypes::NODE_NAMESPACE_DECLARATION;
        }

    void exec_from(Memory& _memory) override {
        unique_ptr<Memory> new_namespace_memory = make_unique<Memory>();
        
        _memory.link_objects(*new_namespace_memory);
        if (statement) {
            statement->exec_from(*new_namespace_memory);
        }

        if (_memory.check_literal(name)) {
            if (_memory.is_final(name)) {
                ERROR::VariableAlreadyDefined(decl_token, name);
            }
            _memory.delete_variable(name);
        }

        auto new_namespace = NewNamespace(*new_namespace_memory, name);
        _memory.add_object(name, new_namespace, STANDART_TYPE::NAMESPACE, is_const, is_static, is_final, is_global);
    }
};

struct Break {};

// + SUCCESS WORKS
struct NodeBreak : public Node { NO_EVAL
    NodeBreak() {
        this->NODE_TYPE = NodeTypes::NODE_BREAK;
    }

    void exec_from(Memory& _memory) override {
        throw Break();
    }
};

struct Continue {};

// + SUCCESS WORKS
struct NodeContinue : public Node { NO_EVAL
    NodeContinue() {
        this->NODE_TYPE = NodeTypes::NODE_CONTINUE;
    }

    void exec_from(Memory& _memory) override {
        throw Continue();
    }
};


// + SUCCESS WORKS
struct NodeWhile : public Node { NO_EVAL
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> statement;

    NodeWhile(unique_ptr<Node> eq_expression, unique_ptr<Node> statement) :
        eq_expression(std::move(eq_expression)), statement(std::move(statement)) {
        this->NODE_TYPE = NodeTypes::NODE_WHILE;
    }


    void exec_from(Memory& _memory) override {
        while (true) {
            auto value = eq_expression->eval_from(_memory);
            if (value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }

            try {
                statement->exec_from(_memory);
            }
            catch (Break) { break; }
            catch (Continue) { continue; }
        }
    }
};


// + SUCCESS WORKS
struct NodeDoWhile : public Node { NO_EVAL
    unique_ptr<Node> eq_expression;
    unique_ptr<Node> statement;

    NodeDoWhile(unique_ptr<Node> eq_expression, unique_ptr<Node> statement) :
        eq_expression(std::move(eq_expression)), statement(std::move(statement)) {
        this->NODE_TYPE = NodeTypes::NODE_DO_WHILE;
    }

    void exec_from(Memory& _memory) override {
        while (true) {
            try { statement->exec_from(_memory); }
            catch (Break) { break; }
            catch (Continue) { continue; }

            auto value = eq_expression->eval_from(_memory);
            if (value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }
        }
    }
};


// + SUCCESS WORKS
struct NodeFor : public Node { NO_EVAL
    unique_ptr<Node> start_state;
    unique_ptr<Node> check_expr;
    unique_ptr<Node> update_state;
    unique_ptr<Node> body;

    NodeFor(unique_ptr<Node> start_state, unique_ptr<Node> check_expr, unique_ptr<Node> update_state, unique_ptr<Node> body) :
        start_state(std::move(start_state)), check_expr(std::move(check_expr)),
        update_state(std::move(update_state)), body(std::move(body)) {
        this->NODE_TYPE = NodeTypes::NODE_FOR;
    }

    void exec_from(Memory& _memory) override {
        start_state->exec_from(_memory);

        while (true) {
            auto value = check_expr->eval_from(_memory);
            if (value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data) == false)
                    break;
            } else if (value.type == STANDART_TYPE::INT) {
                if (any_cast<int64_t>(value.data) == 0)
                    break;
            } else if (value.type == STANDART_TYPE::DOUBLE) {
                if (any_cast<long double>(value.data) == 0)
                    break;
            } else {
                break;
            }

            try {
                body->exec_from(_memory);
            }
            catch (Break) { break; }
            catch (Continue) {
                // Пропускаем оставшуюся часть итерации
                // но ВСЕГДА выполняем update_state перед continue
                update_state->exec_from(_memory);
                continue;
            }

            // Нормальное выполнение (без continue)
            
            update_state->exec_from(_memory);
            

        }
    }
};


// + SUCCESS WORKS
struct NodeBlockDecl : public Node { NO_EVAL
    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;

    vector<unique_ptr<Node>> decls;

    NodeBlockDecl(vector<unique_ptr<Node>> decls) : decls(std::move(decls)) {
        this->NODE_TYPE = NodeTypes::NODE_BLOCK_OF_DECLARATIONS;
    }

    void exec_from(Memory& _memory) override {
        for (int i = 0; i < decls.size(); i++) {
            if (decls[i]->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
                ((NodeBaseVariableDecl*)decls[i].get())->is_const = is_const;
                ((NodeBaseVariableDecl*)decls[i].get())->is_static = is_static;
                ((NodeBaseVariableDecl*)decls[i].get())->is_final = is_final;
                ((NodeBaseVariableDecl*)decls[i].get())->is_global = is_global;
            }
            if (decls[i]->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
                ((NodeNamespaceDecl*)decls[i].get())->is_const = is_const;
                ((NodeNamespaceDecl*)decls[i].get())->is_static = is_static;
                ((NodeNamespaceDecl*)decls[i].get())->is_final = is_final;
                ((NodeNamespaceDecl*)decls[i].get())->is_global = is_global;
            }
            decls[i]->exec_from(_memory);
        }
    }
};

struct NodeAddressOf : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeAddressOf(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_ADDRESS_OF;
    }

    pair<Memory, string> resolveAssignmentTarget(Node* node, Memory& _memory) {
        NodeNameResolution* resolution = (NodeNameResolution*)node;

        // Получаем namespace
        Value ns_value = resolution->namespace_expr->eval_from(_memory);

        if (ns_value.type != STANDART_TYPE::NAMESPACE) {
            cout << "ERROR TYPE" << endl;
            exit(0);
        }

        auto ns = any_cast<Namespace>(ns_value.data);

        // Если есть цепочка, идем по ней
        vector<string> full_chain = resolution->remaining_chain;
        full_chain.insert(full_chain.begin(), resolution->current_name);

        if (full_chain.size() > 1) {
            // Ищем конечный namespace через цепочку
            Memory current_memory = ns.memory;

            // Проходим по всем промежуточным namespace кроме последнего
            for (size_t i = 0; i < full_chain.size() - 1; i++) {
                const string& ns_name = full_chain[i];

                if (!current_memory.check_literal(ns_name)) {
                    ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                }

                Value next_ns_value = current_memory.get_variable(ns_name)->value;

                if (next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                    cout << "ERROR TYPE" << endl;
                    exit(0);
                }

                current_memory = any_cast<Namespace>(next_ns_value.data).memory;
            }

            return {current_memory, full_chain.back()};
        }

        // Если цепочки нет, возвращаем текущий namespace и имя
        return {ns.memory, resolution->current_name};
    }

    Value eval_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        if (expr->NODE_TYPE == NodeTypes::NODE_SCOPES) {
            while (expr->NODE_TYPE == NodeTypes::NODE_SCOPES)
                expr = std::move(((NodeScopes*)(expr.get()))->expression);
        }
        if (expr->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            auto var_name = ((NodeLiteral*)(expr.get()))->name;
            auto addr = _memory.get_variable(var_name)->address;
            return NewPointer(addr, value.type);
        }
        if (expr->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            auto [memory, var_name] = resolveAssignmentTarget(expr.get(), _memory);
            auto addr = memory.get_variable(var_name)->address;
            return NewPointer(addr, value.type);
        }
    }
};

struct NodeDereference : public Node { NO_EXEC
    unique_ptr<Node> expr;
    Token start;
    Token end;

    NodeDereference(unique_ptr<Node> expr, Token start, Token end) : expr(std::move(expr)), start(start), end(end) {
        this->NODE_TYPE = NodeTypes::NODE_DEREFERENCE;
    }

    Value eval_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        if (value.type.is_pointer()) {
            auto object = STATIC_MEMORY.get_by_address(any_cast<int>(value.data));
            if (!object)
                return NewNull();
            return object->value;
        }
        if (value.type == STANDART_TYPE::TYPE) {
            //if (any_cast<Type>(value.data).is_union_type()) {
            //    ERROR::InvalidDereferenceType(start, end);
            //}
            return NewType(PointerType(any_cast<Type>(value.data)));
        }
        ERROR::InvalidDereferenceValue(start, end, value.type);
    }
};

struct NodeLeftDereference : public Node { NO_EVAL
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
            this->NODE_TYPE = NodeTypes::NODE_LEFT_DEREFERENCE;
        };

    void exec_from(Memory& _memory) override {
        auto right_value = right_expr->eval_from(_memory);
        

        // НЕ перемещаем variable, используем сырой указатель
        Node* variable_ptr = left_expr.get();

        auto value = left_expr->eval_from(_memory);

        

        if (value.type.is_pointer()) {
            auto address = any_cast<int>(value.data);
            auto object = STATIC_MEMORY.get_by_address(address);
            auto modifiers = object->modifiers;


            // Проверяем константность
            if (modifiers.is_const) {
                ERROR::ConstPointerRedefinition(start_left_value_token, end_left_value_token);
            }

            // Проверяем типизацию для статических переменных
            if (modifiers.is_static) {
                auto wait_type = object->wait_type;
                auto value_type = right_value.type;
                if (!IsTypeCompatible(wait_type, value_type)) {
                    ERROR::StaticTypesMisMatch(start_left_value_token, end_value_token, wait_type, value_type);
                }
            }

            // Выполняем присваивание
            if (STATIC_MEMORY.is_registered(address)) {
                STATIC_MEMORY.set_object_value(address, right_value);
            }


            
            
            
        }
    }

};

struct NodeTypeof : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeTypeof(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_TYPEOF;
    }

    Value eval_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        return NewType(value.type);
    }
};

struct NodeSizeof : public Node { NO_EXEC
    unique_ptr<Node> expr;

    NodeSizeof(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_SIZEOF;
    }

    Value eval_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        if (value.type == STANDART_TYPE::INT)
            return NewInt(sizeof(int64_t));
        if (value.type == STANDART_TYPE::DOUBLE)
            return NewInt(sizeof(long double));
        if (value.type == STANDART_TYPE::CHAR)
            return NewInt(sizeof(char));
        if (value.type == STANDART_TYPE::STRING)
            return NewInt(any_cast<string>(value.data).size() * sizeof(char));
        if (value.type == STANDART_TYPE::BOOL)
            return NewInt(sizeof(bool));
        if (value.type == STANDART_TYPE::TYPE)
            return NewInt(sizeof(Type));
        if (value.type == STANDART_TYPE::NAMESPACE)
            return NewInt(sizeof(Namespace));
        if (value.type == STANDART_TYPE::NULL_T)
            return NewInt(sizeof(Null));

        return NewInt(sizeof(std::any));
    }
};


// TODO AND TESTS
// +- UNSTABLE WORKS
struct NodeDelete : public Node { NO_EVAL
    unique_ptr<Node> target;
    Token start_token;
    Token end_token;

    NodeDelete(unique_ptr<Node> target, Token start_token, Token end_token)
        : target(std::move(target)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_DELETE;
    }

    void exec_from(Memory& _memory) override {
        if (target->NODE_TYPE != NodeTypes::NODE_LITERAL && target->NODE_TYPE != NodeTypes::NODE_NAME_RESOLUTION && target->NODE_TYPE != NodeTypes::NODE_DEREFERENCE) {
            ERROR::InvalidDeleteInstruction(start_token, end_token);
        }

        if (target->NODE_TYPE == NodeTypes::NODE_DEREFERENCE) {
            auto value = ((NodeDereference*)(target.get()))->expr->eval_from(_memory);
            if (value.type.is_pointer()) {
                auto address = any_cast<int>(value.data);
                if (STATIC_MEMORY.is_registered(address)) {
                    STATIC_MEMORY.unregister_object(address);
                    return;
                }
            }
            ERROR::CanNotDeleteUndereferencedValue(start_token, end_token);
        } else {
            pair<Memory*, string> target_info = resolveDeleteTargetMemory(target.get(), _memory);
            Memory* target_memory = target_info.first;
            string target_name = target_info.second;

            if (!target_memory->check_literal(target_name)) {
                ERROR::UndefinedVariable(start_token);
            }

            // Выполняем удаление
            auto object = target_memory->get_variable(target_name);
            target_memory->delete_variable(target_name);
            STATIC_MEMORY.unregister_object(object->address);
        }
    }

    pair<Memory*, string> resolveDeleteTargetMemory(Node* node, Memory& _memory) {
        if (node->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            if (!_memory.check_literal(((NodeLiteral*)node)->name)) {
                ERROR::UndefinedVariable(start_token);
            }
            return {&_memory, ((NodeLiteral*)node)->name};
        }
        else if (node->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            NodeNameResolution* resolution = (NodeNameResolution*)node;

            // Получаем namespace
            Value ns_value = resolution->namespace_expr->eval_from(_memory);

            if (ns_value.type != STANDART_TYPE::NAMESPACE) {
                cout << "ERROR TYPE" << endl;
                exit(0);
            }

            auto& ns = any_cast<Namespace&>(ns_value.data);  // Получаем ссылку на Namespace

            // Если есть цепочка, идем по ней
            vector<string> full_chain = resolution->remaining_chain;
            full_chain.insert(full_chain.begin(), resolution->current_name);

            if (full_chain.size() > 1) {
                // Ищем конечный namespace через цепочку
                Memory* current_memory = &ns.memory;

                // Проходим по всем промежуточным namespace кроме последнего
                for (size_t i = 0; i < full_chain.size() - 1; i++) {
                    const string& ns_name = full_chain[i];

                    if (!current_memory->check_literal(ns_name)) {
                        ERROR::UndefinedVariableInNamespace(ns_name, "current namespace");
                    }

                    Value next_ns_value = current_memory->get_variable(ns_name)->value;

                    if (next_ns_value.type != STANDART_TYPE::NAMESPACE) {
                        cout << "ERROR TYPE" << endl;
                        exit(0);
                    }

                    auto& next_ns = any_cast<Namespace&>(next_ns_value.data);  // Получаем ссылку
                    current_memory = &next_ns.memory;
                }

                return {current_memory, full_chain.back()};
            }

            // Если цепочки нет, возвращаем текущий namespace и имя
            return {&ns.memory, resolution->current_name};
        }

        // Ошибка для неизвестного типа ноды
        cout << "ERROR TYPE" << endl;
        exit(0);
    }
};


struct NodeIfExpr : public Node { NO_EXEC
    unique_ptr<Node> condition;
    unique_ptr<Node> true_expr;
    unique_ptr<Node> else_expr;

    NodeIfExpr(unique_ptr<Node> condition, unique_ptr<Node> true_expr, unique_ptr<Node> else_expr)
        : condition(std::move(condition)), true_expr(std::move(true_expr)), else_expr(std::move(else_expr)) {
            this->NODE_TYPE = NodeTypes::NODE_IF_EXPRESSION;
    }

    Value eval_from(Memory& _memory) override {
        auto condition_value = condition->eval_from(_memory);
        if (condition_value.type == STANDART_TYPE::BOOL) {
            if (any_cast<bool>(condition_value.data)) {
                return true_expr->eval_from(_memory);
            }
            if (else_expr) return else_expr->eval_from(_memory);
        } else if (condition_value.type == STANDART_TYPE::NULL_T) {
            return else_expr->eval_from(_memory);
        } else {
            return true_expr->eval_from(_memory);
        }
    }

};


struct NodeInput : public Node { NO_EXEC
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;

    NodeInput(unique_ptr<Node> expr, Token start_token, Token end_token)
        : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
            this->NODE_TYPE = NodeTypes::NODE_INPUT;
    }

    Value eval_from(Memory& _memory) override {
        if (expr) {
            
            auto value = expr->eval_from(_memory);
            
            if (value.type == STANDART_TYPE::STRING) {
                cout << any_cast<string>(value.data);
            } else if (value.type == STANDART_TYPE::CHAR) {
                cout << any_cast<char>(value.data);
            } else {
                ERROR::IncompartableTypeInput(start_token, end_token, value.type);
            }
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
};


struct NodeNamespace : public Node { NO_EXEC
    unique_ptr<Node> statement;
    Memory& parent_memory;

    NodeNamespace(Memory& parent_memory, unique_ptr<Node> statement) :
        parent_memory(parent_memory), statement(std::move(statement)) {
            this->NODE_TYPE = NodeTypes::NODE_NAMESPACE_EXPRESSION;
        }

    Value eval_from(Memory& _memory) override {
        auto name_space_mem = make_unique<Memory>();
        _memory.link_objects(*name_space_mem);
        
        if (statement) {
            statement->exec_from(*name_space_mem.get());
        }

        auto new_namespace = NewNamespace(*name_space_mem, "anonymous-namespace");
        return new_namespace;
    }
};


struct NodeAssert : public Node { NO_EVAL
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;

    NodeAssert(unique_ptr<Node> expr, Token start_token, Token end_token)
        : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
            this->NODE_TYPE = NodeTypes::NODE_ASSERT;
    }

    void exec_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        if (value.type != STANDART_TYPE::BOOL) {
            ERROR::AssertionIvalidArgument(start_token, end_token);
        }
        if (!any_cast<bool>(value.data)) {
            ERROR::AssertionFailed(start_token, end_token);
        }
    }

};

struct NodeExpressionStatement : public Node { NO_EVAL
    unique_ptr<Node> expr;
    NodeExpressionStatement(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_EXPRESSION_STATEMENT;
    }
    void exec_from(Memory& _memory) override {
        expr->eval_from(_memory);
    }
};

struct NodeLambda : public Node { NO_EXEC
    vector<Arg*> args;
    unique_ptr<Node> return_type;
    unique_ptr<Node> body;

    shared_ptr<Memory> lambda_memory;
    Memory& parent_memory;

    Token start_args_token;
    Token end_args_token;
    Token start_type_token;
    Token end_type_token;

    string name = "";

    NodeLambda(Memory& parent_memory, unique_ptr<Node> body, vector<Arg*> args, unique_ptr<Node> return_type,
               Token start_args_token, Token end_args_token, Token start_type_token, Token end_type_token) :
        parent_memory(parent_memory), body(std::move(body)), args(std::move(args)), return_type(std::move(return_type)),
        start_args_token(start_args_token), end_args_token(end_args_token), start_type_token(start_type_token), end_type_token(end_type_token) {
            this->NODE_TYPE = NodeTypes::NODE_LAMBDA;
        }

    
    Value eval_from(Memory& _memory) override {
        auto new_lambda_memory = make_shared<Memory>();
        // Копируем глобальные переменные из родительской памяти
        _memory.link_objects(*new_lambda_memory);
        
        
        for (auto arg : args) {
            if (arg->type_expr) {
                auto super_type_value = arg->type_expr->eval_from(*new_lambda_memory);
                if (super_type_value.type != STANDART_TYPE::TYPE) {
                    ERROR::WaitedLambdaArgumentTypeSpecifier(start_args_token, end_args_token, arg->name);
                }
            }
        }

        if (return_type) {
            auto super_type_value = return_type->eval_from(*new_lambda_memory);
            if (super_type_value.type != STANDART_TYPE::TYPE) {
                ERROR::WaitedLambdaReturnTypeSpecifier(start_type_token, end_type_token);
            }
        }
        auto lambda = NewLambda(new_lambda_memory, body.get(), vector(args), std::move(return_type), start_args_token, end_args_token, start_type_token, end_type_token);
        if (name != "") {
            (any_cast<Lambda*>(lambda.data))->memory->add_object(name, lambda, lambda.type, true, true, true, true);
        }
        return lambda;
    }
};


struct Return {
    Value value;
    Return(Value value) : value(value) {}
};

struct NodeReturn : public Node { NO_EVAL
    unique_ptr<Node> expr;
    Token start_token;
    Token end_token;
    NodeReturn(unique_ptr<Node> expr, Token start_token, Token end_token) : expr(std::move(expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_RETURN;
    }

    void exec_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);
        
        throw Return(value);
    }
};


struct NodeCall : public Node { NO_EXEC
    unique_ptr<Node> callable;
    vector<unique_ptr<Node>> args;
    Token start_callable;
    Token end_callable;

    NodeCall(unique_ptr<Node> callable, vector<unique_ptr<Node>> args, Token start_callable, Token end_callable) :
        callable(std::move(callable)), args(std::move(args)), start_callable(start_callable), end_callable(end_callable) {
        this->NODE_TYPE = NodeTypes::NODE_CALL;
    }

    Value eval_from(Memory& _memory) override {
        auto value = callable->eval_from(_memory);
        
        if (value.type == STANDART_TYPE::LAMBDA) {
            auto lambda = any_cast<Lambda*>(value.data);
            Memory saved_mem = *lambda->memory;
            if (lambda->arguments.size() != args.size()) {
                ERROR::InvalidLambdaArgumentCount(start_callable, end_callable,
                    lambda->start_args_token, lambda->end_args_token, lambda->arguments.size(), args.size());
            }

            for (size_t i = 0; i < args.size(); i++) {
                auto settable_value = args[i]->eval_from(_memory);

                if (lambda->arguments[i]->type_expr) {
                    
                    auto super_type_value = lambda->arguments[i]->type_expr->eval_from(*lambda->memory);
                    
                    if (!settable_value.type.is_sub_type(any_cast<Type>(super_type_value.data))) {
                        ERROR::InvalidLambdaArgumentType(start_callable, end_callable, lambda->start_args_token, lambda->end_args_token, 
                            any_cast<Type>(super_type_value.data), settable_value.type, lambda->arguments[i]->name);
                    }
                }

                lambda->memory->add_object_in_lambda(lambda->arguments[i]->name, settable_value);
            }
            
            
            auto result = ((Node*)(lambda->expr))->eval_from(*lambda->memory);
            
            if (lambda->return_type) {
                auto super_type_value = lambda->return_type->eval_from(*lambda->memory);
                if (!result.type.is_sub_type(any_cast<Type>(super_type_value.data))) {
                    ERROR::InvalidLambdaReturnType(start_callable, end_callable, lambda->start_type_token, lambda->end_type_token, 
                        any_cast<Type>(super_type_value.data), result.type);
                }
            }
            *lambda->memory = saved_mem;
            //lambda->memory->clear_unglobals();
            
            
            return result;
        }
        if (value.type.is_func()) {
            auto func = any_cast<Function*>(value.data);
            func->memory->add_object_in_func(func->name, value, false, false, false, true);
            Memory saved_mem = *func->memory;

            if (func->arguments.size() != args.size()) {
                ERROR::InvalidFuncArgumentCount(start_callable, end_callable,
                    func->start_args_token, func->end_args_token, func->arguments.size(), args.size());
            }
            
            for (size_t i = 0; i < args.size(); i++) {
                auto settable_value = args[i]->eval_from(_memory);
                if (func->arguments[i]->type_expr) {
                    auto super_type_value = func->arguments[i]->type_expr->eval_from(*func->memory);
                    
                    if (!settable_value.type.is_sub_type(any_cast<Type>(super_type_value.data))) {
                        ERROR::InvalidFuncArgumentType(start_callable, end_callable, func->start_args_token, func->end_args_token, 
                            any_cast<Type>(super_type_value.data), settable_value.type, func->arguments[i]->name);
                    }
                    
                }

                
                func->memory->add_object_in_func(func->arguments[i]->name, settable_value, func->arguments[i]->is_const, func->arguments[i]->is_static, func->arguments[i]->is_final, true);
            }
            

            try {
                ((Node*)(func->body.get()))->exec_from(*func->memory);
                return NewNull();
            } catch (Return _value) {
                if (func->return_type) {
                    auto super_type_value = func->return_type->eval_from(*func->memory);
                    if (!_value.value.type.is_sub_type(any_cast<Type>(super_type_value.data))) {
                        ERROR::InvalidLambdaReturnType(start_callable, end_callable, func->start_return_type_token, func->end_return_type_token, 
                            any_cast<Type>(super_type_value.data), _value.value.type);
                    }
                }
                *func->memory = saved_mem;
                return _value.value;
            }
            
            
        }

        ERROR::IvalidCallableType(start_callable, end_callable);
    }
};


struct NodeNew : public Node { NO_EXEC
    unique_ptr<Node> expr;
    unique_ptr<Node> type_expr;
    bool is_const = false;
    bool is_static = false;

    Token start_type;
    Token end_type;

    NodeNew(unique_ptr<Node> expr, unique_ptr<Node> type_expr, bool is_static, bool is_const,
            Token start_type, Token end_type) : 
    expr(std::move(expr)), type_expr(std::move(type_expr)), is_const(is_const), is_static(is_static),
    start_type(start_type), end_type(end_type) {
        this->NODE_TYPE = NodeTypes::NODE_NEW;
    }

    Value eval_from(Memory& _memory) override {
        
        if (type_expr && expr) {
            auto result = expr->eval_from(_memory);
            auto super_type_value = type_expr->eval_from(_memory);
            if (!result.type.is_sub_type(any_cast<Type>(super_type_value.data))) {
                ERROR::StaticTypesMisMatch(start_type, end_type, any_cast<Type>(super_type_value.data), result.type);
            }

            auto object = CreateMemoryObject(result, any_cast<Type>(super_type_value.data), is_const, is_static, false, false, nullptr );
            auto addres = NewPointer(object->address, any_cast<Type>(super_type_value.data), true);
            STATIC_MEMORY.register_object(object);
            return addres;
        } else if (type_expr && !expr) {
            auto result = NewNull();
            auto super_type_value = type_expr->eval_from(_memory);
            auto object = CreateMemoryObject(result, any_cast<Type>(super_type_value.data), is_const, is_static, false, false, nullptr );
            auto addres = NewPointer(object->address, any_cast<Type>(super_type_value.data), true);
            STATIC_MEMORY.register_object(object);
            return addres;
        } else if (!type_expr && expr) {
            auto result = expr->eval_from(_memory);
            auto object = CreateMemoryObject(result, result.type, is_const, is_static, false, false, nullptr );
            auto addres = NewPointer(object->address, result.type, true);
            STATIC_MEMORY.register_object(object);
            return addres;
        } else if (is_static && expr) {
            auto result = expr->eval_from(_memory);
            auto object = CreateMemoryObject(result, result.type, is_const, is_static, false, false, nullptr );
            auto addres = NewPointer(object->address, result.type, true);
            STATIC_MEMORY.register_object(object);
            return addres;
        
        } else {
            ERROR::InvalidNewInstruction(start_type, end_type);
        }
    }

};


struct NodeNewFuncType : public Node { NO_EXEC
    vector<unique_ptr<Node>> args_types_expr;
    unique_ptr<Node> return_type_expr;

    Token start_token_args;
    Token end_token_args;

    Token start_token_return;
    Token end_token_return;

    NodeNewFuncType(vector<unique_ptr<Node>> args_types, unique_ptr<Node> return_type_expr,
                    Token start_token_args, Token end_token_args, Token start_token_return, Token end_token_return) : 
                    args_types_expr(std::move(args_types)), return_type_expr(std::move(return_type_expr)),
                    start_token_args(start_token_args), end_token_args(end_token_args),
                    start_token_return(start_token_return), end_token_return(end_token_return) {
        this->NODE_TYPE = NodeTypes::NODE_FUNCTION_TYPE;
    }


    Value eval_from(Memory& _memory) override {
        vector<Type> args_types;

        for (int i = 0; i < args_types_expr.size(); i++) {
            auto value = args_types_expr[i]->eval_from(_memory);
            if (value.type != STANDART_TYPE::TYPE) 
                ERROR::WaitedFuncTypeArgumentTypeSpecifier(start_token_args, end_token_args, i);
            args_types.push_back(any_cast<Type>(value.data));
        }


        if (return_type_expr) {
            auto value = return_type_expr->eval_from(_memory);
            if (value.type != STANDART_TYPE::TYPE)
                ERROR::WaitedFuncTypeReturnTypeSpecifier(start_token_return, end_token_return);
            
            auto result = create_function_type(any_cast<Type>(value.data), args_types);
            return NewType(result);
        } else {
            auto result = create_function_type(args_types);
            return NewType(result);
        }
        
    }

};


struct NodeFuncDecl : public Node { NO_EVAL
    string name;
    vector<Arg*> args;
    unique_ptr<Node> return_type;
    unique_ptr<Node> body;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;

    Token start_args_token;
    Token end_args_token;
    Token start_return_token;
    Token end_return_token;
    

    NodeFuncDecl(string name, vector<Arg*> args, unique_ptr<Node> return_type, unique_ptr<Node> body,
                Token start_args_token, Token end_args_token, Token start_return_token, Token end_return_token) : 
        name(name), args(std::move(args)), return_type(std::move(return_type)), body(std::move(body)),
        start_args_token(start_args_token), end_args_token(end_args_token), start_return_token(start_return_token), end_return_token(end_return_token) {
        this->NODE_TYPE = NodeTypes::NODE_FUNCTION_DECLARATION;
    }

    Type construct_type(Memory& _memory) {
        if (return_type) {
            auto value = return_type->eval_from(_memory);

            if (value.type != STANDART_TYPE::TYPE) 
                ERROR::WaitedFuncTypeReturnTypeSpecifier(start_return_token, end_return_token);
                
            Type return_type = any_cast<Type>(value.data);
                
            vector<Type> args_types;
            for (int i = 0; i < args.size(); i++) {
                auto value = args[i]->type_expr->eval_from(_memory);
                if (value.type != STANDART_TYPE::TYPE) 
                    ERROR::WaitedFuncTypeArgumentTypeSpecifier(start_args_token, end_args_token, args[i]->name);
                args_types.push_back(any_cast<Type>(value.data));
            }
            return create_function_type(return_type, args_types);
        } else {
            vector<Type> args_types;
            for (int i = 0; i < args.size(); i++) {
                auto value = args[i]->type_expr->eval_from(_memory);
                if (value.type != STANDART_TYPE::TYPE) 
                    ERROR::WaitedFuncTypeArgumentTypeSpecifier(start_args_token, end_args_token, args[i]->name);
                args_types.push_back(any_cast<Type>(value.data));
            }
            return create_function_type(args_types);
        }
    }

    void exec_from(Memory& _memory) override {
        Type function_type = construct_type(_memory);

        auto new_function_memory = make_shared<Memory>();
        _memory.link_objects(*new_function_memory);

        auto func = NewFunction(name, new_function_memory, std::move(body), args, std::move(return_type), function_type, start_args_token, end_args_token, start_return_token, end_return_token);
        auto object = CreateMemoryObject(func, function_type, is_const, is_static, is_final, is_global, &new_function_memory);
        if (_memory.check_literal(name))
            _memory.delete_variable(name);
        _memory.add_object(name, object);
        STATIC_MEMORY.register_object(object);
    }
};


struct NodeExit : public Node { NO_EVAL
    unique_ptr<Node> expr;

    NodeExit(unique_ptr<Node> expr) : expr(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_EXIT;
    }

    void exec_from(Memory& _memory) override {
        auto value = expr->eval_from(_memory);

        if (value.type != STANDART_TYPE::INT) {
            cout << "'Exit' with invalid type" << endl;
            exit(-1);
        }
        
        exit(any_cast<int64_t>(value.data));
    }
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
    string file_name;

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
    unique_ptr<Node> ParseNamespace(Memory& memory);
    unique_ptr<Node> ParseIf(Memory& memory);
    unique_ptr<Node> ParseIfExpr(Memory& memory);

    unique_ptr<Node> ParseCall(unique_ptr<Node> expr, Memory& memory, Token start, Token end);

    unique_ptr<Node> ParseNumber();
    unique_ptr<Node> ParseString();
    unique_ptr<Node> ParseChar();
    unique_ptr<Node> ParseBool();
    unique_ptr<Node> ParseNull();
    unique_ptr<Node> ParseBreak();
    unique_ptr<Node> ParseContinue();
    unique_ptr<Node> ParseLiteral(Memory& memory);

    unique_ptr<Node> ParseAssert(Memory& memory);


    unique_ptr<Node> ParseAddressOf(Memory& memory);
    unique_ptr<Node> ParseDereference(Memory& memory);
    unique_ptr<Node> ParseLeftDereference(Memory& memory);

    unique_ptr<Node> ParseTypeof(Memory& memory);
    unique_ptr<Node> ParseSizeof(Memory& memory);

    unique_ptr<Node> ParseScopes(Memory& memory);

    unique_ptr<Node> ParseNameResolution(unique_ptr<Node> expression);

    unique_ptr<Node> ParseLambda(Memory& memory);

    unique_ptr<Node> ParseNew(Memory& memory); 

    unique_ptr<Node> ParseNewFunctionType(Memory& memory);
    unique_ptr<Node> ParseFuncDecl(Memory& memory);

    unique_ptr<Node> ParsePostfix(unique_ptr<Node> expr, Memory &memory);

    unique_ptr<Node> ParseExit(Memory& memory);
    unique_ptr<Node> ParseReturn(Memory& memory);

    Memory GLOBAL_MEMORY;


    ASTGenerator(TokenWalker& walker, string file_name) : walker(walker), file_name(file_name) {}

    unique_ptr<Node> parse_primary_expression(Memory &memory) {
        Token current = *walker.get();
        if (walker.CheckType(TokenType::NUMBER))
            return ParseNumber();

        if (walker.CheckType(TokenType::STRING))
            return ParseString();

        if (walker.CheckType(TokenType::CHAR))
            return ParseChar();

        if (walker.CheckValue("(")) {
            Token start = *walker.get();
            auto expr =  ParseScopes(memory);
            // Применяем постфиксные операции к скобочному выражению
            return ParsePostfix(std::move(expr), memory);
        }

        if (walker.CheckType(TokenType::KEYWORD) && (walker.CheckValue("true") || walker.CheckValue("false")))
            return ParseBool();

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("null"))
            return ParseNull();

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("input"))
            return ParseInput(memory);

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("lambda"))
            return ParseLambda(memory);

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("typeof") && walker.CheckValue("(", 1)) {
            return ParseTypeof(memory);
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("new")) {
            return ParseNew(memory);
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("sizeof") && walker.CheckValue("(", 1)) {
            return ParseSizeof(memory);
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("namespace")) {
            return ParseNamespace(memory);
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("Func")) {
            return ParseNewFunctionType(memory);
        }

        if (walker.CheckType(TokenType::LITERAL)) {
            auto expr = ParseLiteral(memory);
            // Применяем постфиксные операции к литералу
            return ParsePostfix(std::move(expr), memory);
        }


        return nullptr;
    }

    unique_ptr<Node> parse_unary_expression(Memory &memory) {
        if (walker.CheckType(TokenType::OPERATOR) && (walker.CheckValue("-") || walker.CheckValue("+")
        || walker.CheckValue("--") || walker.CheckValue("++") ||
            walker.CheckValue("!") || walker.CheckValue("not"))) {
            string op = walker.get()->value;
            Token operator_token = *walker.get();
            walker.next(); // pass operator
            Token start = *walker.get();
            auto operand = parse_unary_expression(memory);
            Token end = *walker.get(-1);
            return make_unique<NodeUnary>(std::move(operand), start, end, operator_token);
        }
        if (walker.CheckType(TokenType::OPERATOR) && walker.CheckValue("&")) {
            return ParseAddressOf(memory);
        }
        if (walker.CheckType(TokenType::DEREFERENCE) && walker.CheckValue("*")) {
            return ParseDereference(memory);
        }
        auto expr = parse_primary_expression(memory);
        return ParsePostfix(std::move(expr), memory);
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
                if ((walker.CheckType(TokenType::OPERATOR) || walker.CheckType(TokenType::DEREFERENCE)) && walker.CheckValue(candidate)) {
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
        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("if")) {
            return ParseIfExpr(memory);
        }

        return parse_binary_expression_or(memory);
    }

    unique_ptr<Node> parse_expression(Memory &memory) {
        return parse_higher_order_expressions(memory);
    }

    unique_ptr<Node> parse_statement(Memory& memory) {
        Token current = *walker.get();
        if (current.type == TokenType::L_CURVE_BRACKET) {
            return ParseBlock(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "out") {
            return ParseOut(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "outln") {
            return ParseOutLn(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "assert") {
            return ParseAssert(memory);
        }

        if (current.type == TokenType::KEYWORD && current.value == "func") {
            return ParseFuncDecl(memory);
        }
        // block declaration
        if ((current.type == TokenType::KEYWORD) &&
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
        if (current.type == TokenType::KEYWORD && current.value == "let") {
            return ParseBaseVariableDecl(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "final") {
            return ParseFinalVariableDecl(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "static") {
            return ParseStaticVariableDecl(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "const") {
            return ParseConstVariableDecl(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "global") {
            return ParseGlobalVariableDecl(memory);
        }

        if (current.type == TokenType::KEYWORD && current.value == "exit") {
            return ParseExit(memory);
        }

        if (current.type == TokenType::KEYWORD && current.value == "ret") {
            return ParseReturn(memory);
        }

        if (current.type == TokenType::KEYWORD && current.value == "namespace") {
            return ParseNameSpaceDecl(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "if") {
            return ParseIf(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "del") {
            return ParseDelete(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "while") {
            return ParseWhile(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "do") {
            return ParseDoWhile(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "for") {
            return ParseFor(memory);
        }
        if (current.type == TokenType::KEYWORD && current.value == "break") {
            return ParseBreak();
        }
        if (current.type == TokenType::KEYWORD && current.value == "continue") {
            return ParseContinue();
        }
        if (current.type == TokenType::LITERAL || current.value == "(") {

            auto start_left_value_token = *walker.get();
            auto left_expr = parse_expression(memory);

            auto end_left_value_token = *walker.get(-1);

            if (walker.CheckValue("=")) {
             
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
            } else {
                if (!walker.CheckValue(";"))
                    ERROR::UnexpectedToken(*walker.get(), "';'");
                walker.next();
                return make_unique<NodeExpressionStatement>(std::move(left_expr));
            }
        }
        if (current.value == "*") {
            return ParseLeftDereference(memory);
        }
        // Expression statement (например, *a + 1;)
        auto expr = parse_expression(memory);
        if (expr) {
            if (!walker.CheckValue(";"))
                ERROR::UnexpectedToken(*walker.get(), "';'");
            walker.next();
            
            // Создаем специальный узел для выражения, которое вычисляется, но ничего не делает
            
            
            return make_unique<NodeExpressionStatement>(std::move(expr));
        }
        ERROR::UnexpectedToken(current, "expression or statement");
    }

    void GenerateStandartTypes(Memory& _memory) {
        auto OBJ_TYPE_INT = CreateMemoryObject(NewType("Int"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Int",OBJ_TYPE_INT);
        STATIC_MEMORY.register_object(OBJ_TYPE_INT);
        
        auto OBJ_TYPE_DOUBLE = CreateMemoryObject(NewType("Double"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Double",OBJ_TYPE_DOUBLE);
        STATIC_MEMORY.register_object(OBJ_TYPE_DOUBLE);

        auto OBJ_TYPE_CHAR = CreateMemoryObject(NewType("Char"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Char",OBJ_TYPE_CHAR);
        STATIC_MEMORY.register_object(OBJ_TYPE_CHAR);

        auto OBJ_TYPE_STRING = CreateMemoryObject(NewType("String"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("String",OBJ_TYPE_STRING);
        STATIC_MEMORY.register_object(OBJ_TYPE_STRING);

        auto OBJ_TYPE_BOOL = CreateMemoryObject(NewType("Bool"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Bool",OBJ_TYPE_BOOL);
        STATIC_MEMORY.register_object(OBJ_TYPE_BOOL);

        auto OBJ_TYPE_TYPE = CreateMemoryObject(NewType("Type"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Type",OBJ_TYPE_TYPE);
        STATIC_MEMORY.register_object(OBJ_TYPE_TYPE);

        auto OBJ_TYPE_NULL = CreateMemoryObject(NewType("Null"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Null",OBJ_TYPE_NULL);
        STATIC_MEMORY.register_object(OBJ_TYPE_NULL);


        auto OBJ_TYPE_NAMESPACE = CreateMemoryObject(NewType("Namespace"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Namespace",OBJ_TYPE_NAMESPACE);
        STATIC_MEMORY.register_object(OBJ_TYPE_NAMESPACE);

        auto OBJ_TYPE_LAMBDA = CreateMemoryObject(NewType("Lambda"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("Lambda",OBJ_TYPE_LAMBDA);
        STATIC_MEMORY.register_object(OBJ_TYPE_LAMBDA);

        auto OBJ_TYPE_AUTO = CreateMemoryObject(NewType("auto"), STANDART_TYPE::TYPE, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("auto",OBJ_TYPE_AUTO);
        STATIC_MEMORY.register_object(OBJ_TYPE_AUTO);

        auto __TWIST_FILE__ = CreateMemoryObject(Value(STANDART_TYPE::STRING, string(file_name)), STANDART_TYPE::STRING, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("__FILE__", __TWIST_FILE__);
        STATIC_MEMORY.register_object(__TWIST_FILE__);

        auto __TWIST_ADDR__ = CreateMemoryObject(NewPointer(AddressManager::get_current_address() + 2, STANDART_TYPE::NULL_T), STANDART_TYPE::STRING, true, true, true, true, &GLOBAL_MEMORY);
        GLOBAL_MEMORY.add_object("__PTR__", __TWIST_ADDR__);
        STATIC_MEMORY.register_object(__TWIST_ADDR__);
        // STATIC_MEMORY.register_object(__TWIST_FILE__);
        
        // STATIC_MEMORY.debug_print();
    }

    inline void parse() {
        GenerateStandartTypes(GLOBAL_MEMORY);

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
            memory_copy.copy_object(name, obj->value, obj->wait_type,
                                    obj->modifiers.is_const, obj->modifiers.is_static,
                                    obj->modifiers.is_final, obj->modifiers.is_global, obj->address);
        }

        return Context(std::move(nodes), std::move(memory_copy));
    }
};


struct ContextExecutor {
    Context context;

    ContextExecutor(Context context) : context(std::move(context)) {}
    void run() {
        for (int i = 0; i < (context.nodes).size(); i++) {
            (context.nodes)[i]->exec_from(context.memory);
        }
    }
};


unique_ptr<Node> ASTGenerator::ParseReturn(Memory& memory) {
    walker.next();
    Token start = *walker.get();
    auto expr = parse_expression(memory);
    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    Token end = *walker.get(-1);
    walker.next();
    return make_unique<NodeReturn>(std::move(expr), start, end);
}


unique_ptr<Node> ASTGenerator::ParseExit(Memory& memory) {
    walker.next();
    auto expr = parse_expression(memory);
    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();
    return make_unique<NodeExit>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParsePostfix(unique_ptr<Node> expr, Memory &memory) {
    while (true) {
        if (walker.CheckValue("(")) {
            Token start = *walker.get();
            Token end;
            expr = ParseCall(std::move(expr), memory, start, end);
        } else if (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
            expr = ParseNameResolution(std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}


unique_ptr<Node> ASTGenerator::ParseFuncDecl(Memory& memory) {
    walker.next();

    if (!walker.CheckType(TokenType::LITERAL)) 
        ERROR::UnexpectedToken(*walker.get(), "function name");
    string name = walker.get()->value;
    walker.next();

    // parse arguments
    Token start_args_token = *walker.get();
    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    
    
    vector<Arg*> arguments;
    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }

        bool arg_is_const = false;
        bool arg_is_static = false;
        bool arg_is_final = false;
        while (true) {
            if (walker.CheckValue("const")) {
                arg_is_const = true;
                walker.next();
                continue;
            }
            if (walker.CheckValue("static")) {
                arg_is_static = true;
                walker.next();
                continue;
            }
            if (walker.CheckValue("final")) {
                arg_is_final = true;
                walker.next();
                continue;
            }
            break;
        }
        
            
        

        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "variable name");
        string arg_name = walker.get()->value;
        walker.next();

        unique_ptr<Node> type_expr = nullptr;
        unique_ptr<Node> default_expr = nullptr;

        

        if (!walker.CheckValue(":")) 
            ERROR::UnexpectedToken(*walker.get(), "':'");

        

        walker.next();
        type_expr = parse_expression(memory);
        if (!type_expr)
            ERROR::UnexpectedToken(*walker.get(), "type expression");
        

        if (walker.CheckValue("=")) {
            walker.next();
            default_expr = parse_expression(memory);
            if (!default_expr)
                ERROR::UnexpectedToken(*walker.get(), "default value expression");
        }
        
        auto arg = new Arg(arg_name);
        arg->type_expr = std::move(type_expr);
        arg->default_parameter = std::move(default_expr);
        arg->is_const = arg_is_const;
        arg->is_static = arg_is_static;
        arg->is_final = arg_is_final;
        
        arguments.push_back(arg);
        
        if (!walker.CheckValue(",") && !walker.CheckValue(")")) {
            ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
        }
        if (walker.CheckValue(",")) walker.next();
    }

    Token end_args_token = *walker.get(-1);
    Token start_return_token = *walker.get();
    unique_ptr<Node> return_type_expr = nullptr;
    if (walker.CheckValue("->")) {
        walker.next();
        start_return_token = *walker.get();
        return_type_expr = parse_expression(memory);
        if (!return_type_expr)
            ERROR::UnexpectedToken(*walker.get(), "return type expression");
    }
    Token end_return_token = *walker.get(-1);
    
    
    auto statement = parse_statement(memory);

    
    
    
    return make_unique<NodeFuncDecl>(name, std::move(arguments), std::move(return_type_expr), std::move(statement), start_args_token, end_args_token, start_return_token, end_return_token);
        
    
}


unique_ptr<Node> ASTGenerator::ParseNewFunctionType(Memory& memory) {
    walker.next(); // pass "func" token

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    
    Token start_token = *walker.get();
    walker.next();

    if (walker.CheckValue(")")) {
        Token end_token = *walker.get();
        walker.next(); // pass ")" token
        return make_unique<NodeNewFuncType>(vector<unique_ptr<Node>>(), nullptr, start_token, start_token, Token(), Token());
    }
    vector<unique_ptr<Node>> type_args;
    while (true) {
        
        auto type_expr = parse_expression(memory);
        
        
        if (!type_expr)
            ERROR::UnexpectedToken(*walker.get(), "expression");

        type_args.push_back(std::move(type_expr));

        
        if (walker.CheckValue(")") || walker.CheckValue(",")) {
            if (walker.CheckValue(",")) {
                walker.next();
                continue;
            }
            else if (walker.CheckValue(")")) break;
        }
        ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
    }

    Token end_token = *walker.get();
    walker.next(); // pass ")" token

    unique_ptr<Node> return_type_expr = nullptr;
    Token start_token_return;
    Token end_token_return;

    if (walker.CheckValue("->")) {
        walker.next(); // pass "->" token
        start_token_return = *walker.get();
        return_type_expr = parse_expression(memory);
        end_token_return = *walker.get(-1);
        if (!return_type_expr)
            ERROR::UnexpectedToken(*walker.get(), "expression");
    }

    return make_unique<NodeNewFuncType>(std::move(type_args), std::move(return_type_expr), start_token, end_token, start_token_return, end_token_return);

}


unique_ptr<Node> ASTGenerator::ParseNew(Memory& memory) {
    walker.next(); // pass 'new' token
    bool is_static = false;
    bool is_const = false;
    unique_ptr<Node> type_expr = nullptr;

    Token start_type = *walker.get();
    Token end_type = *walker.get();

    if (walker.CheckValue("<")) {
        walker.next();
        while (!walker.CheckValue(">")) {
            auto m = walker.get()->value;
            if (m == "const") {is_const = true; walker.next();}
            else if (m == "static") {
                is_static = true;
                walker.next();
                if (walker.CheckValue("(")) {
                    walker.next();
                    start_type = *walker.get();
                    type_expr = parse_expression(memory);
                    if (!type_expr)
                        ERROR::UnexpectedToken(*walker.get(), "type expression");
                    end_type = *walker.get(-1);
                    if (!walker.CheckValue(")"))
                        ERROR::UnexpectedToken(*walker.get(), "')'");
                    walker.next();
                }
            }
            else ERROR::UnexpectedToken(*walker.get(), "modifiers ('const', 'static')");
            
            if (walker.CheckValue(",") || walker.CheckValue(">")) {
                if (walker.CheckValue(",")) {
                    walker.next();
                    continue;
                }
                else if (walker.CheckValue(">")) break;
            }
            ERROR::UnexpectedToken(*walker.get(), "',' or '>'");
        }
        walker.next(); // pass ")" token
    }
    
    auto expr = parse_expression(memory);
    
    return make_unique<NodeNew>(std::move(expr), std::move(type_expr), is_static, is_const, start_type, end_type);
}


unique_ptr<Node> ASTGenerator::ParseCall(unique_ptr<Node> expr, Memory& memory, Token start, Token end) {
    walker.next(); // pass '(' token

    vector<unique_ptr<Node>> arguments;
    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }
        auto arg = parse_expression(memory);
        if (!arg)
            ERROR::UnexpectedToken(*walker.get(), "argument expression");
        arguments.push_back(std::move(arg));

        if (!walker.CheckValue(")") && !walker.CheckValue(",")) {
            ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
        }

        if (walker.CheckValue(",")) walker.next();
    }
    end = *walker.get(-1);
    return make_unique<NodeCall>(std::move(expr), std::move(arguments), start, end);
}


unique_ptr<Node> ASTGenerator::ParseLambda(Memory& memory) {
    walker.next(); // pass 'lambda' token
    Token start_args_token = *walker.get();


    string name = "";
    if (walker.CheckValue("[")) {
        walker.next();
        if (walker.CheckType(TokenType::LITERAL)) {
            name = walker.get()->value;
            walker.next();
        } else {
            ERROR::UnexpectedToken(*walker.get(), "literal (not `String` or `Char`)");
        }
        if (!walker.CheckValue("]")) ERROR::UnexpectedToken(*walker.get(), "']'");
        walker.next();
    }

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();
    
    vector<Arg*> arguments;
    
    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }
        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "variable name");
        string arg_name = walker.get()->value;
        walker.next();

        unique_ptr<Node> type_expr = nullptr;
        unique_ptr<Node> default_expr = nullptr;

        if (walker.CheckValue(":")) {
            walker.next();
            type_expr = parse_expression(memory);
            if (!type_expr)
                ERROR::UnexpectedToken(*walker.get(), "type expression");
        }

        if (walker.CheckValue("=")) {
            walker.next();
            default_expr = parse_expression(memory);
            if (!default_expr)
                ERROR::UnexpectedToken(*walker.get(), "default value expression");
        }
        
        auto arg = new Arg(arg_name);
        arg->type_expr = std::move(type_expr);
        arg->default_parameter = std::move(default_expr);
        arguments.push_back(arg);
        
        if (!walker.CheckValue(",") && !walker.CheckValue(")")) {
            ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
        }
        if (walker.CheckValue(",")) walker.next();
    }

    Token end_args_token = *walker.get(-1);

    Token type_start_token;
    Token type_end_token;
 
    unique_ptr<Node> return_type = nullptr;
    if (walker.CheckValue("->")) {
        walker.next();
        type_start_token = *walker.get();
        return_type = parse_expression(memory);
        type_end_token = *walker.get(-1);
        if (!return_type)
            ERROR::UnexpectedToken(*walker.get(), "return type expression");
    }

    if (!walker.CheckValue("{"))
        ERROR::UnexpectedToken(*walker.get(), "'{'");
    walker.next();

    
    
    auto lambda_node = make_unique<NodeLambda>(
        memory, nullptr, arguments, std::move(return_type), 
        start_args_token, end_args_token, type_start_token, type_end_token);
    lambda_node->name = name;

    auto block = parse_expression(*lambda_node->lambda_memory);

    lambda_node->body = std::move(block);

    if (!walker.CheckValue("}"))
        ERROR::UnexpectedToken(*walker.get(), "'}'");
    walker.next();

    return lambda_node;
}


unique_ptr<Node> ASTGenerator::ParseAssert(Memory& memory) {
    walker.next(); // pass 'assert' token
    Token start_token = *walker.get();
    auto expr = parse_expression(memory);
    Token end_token = *walker.get(-1);
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();
    return make_unique<NodeAssert>(std::move(expr), start_token, end_token);
}


unique_ptr<Node> ASTGenerator::ParseNamespace(Memory& memory) {
    walker.next(); // pass "namespace" token
    auto namespace_node = make_unique<NodeNamespace>(memory, nullptr);
    
    auto block = parse_statement(memory);
    namespace_node->statement = std::move(block);

    return namespace_node;
}


unique_ptr<Node> ASTGenerator::ParseInput(Memory& memory) {
    auto start_token = *walker.get();
    walker.next(); // pass 'input' token
    auto end_token = *walker.get(-1);
    unique_ptr<Node> expr = nullptr;
    if (walker.CheckValue("(")) {
        walker.next();
        auto start_token = *walker.get();
        expr = parse_expression(memory);
        if (!expr)
            ERROR::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckValue(")"))
            ERROR::UnexpectedToken(*walker.get(), "')'");
        auto end_token = *walker.get();
        walker.next();
    }
    return make_unique<NodeInput>(std::move(expr), start_token, end_token);
}


unique_ptr<Node> ASTGenerator::ParseLeftDereference(Memory& memory) {
    walker.next(); // pass '*' token

    auto start_left_value_token = *walker.get(-1);
    auto left_expr = parse_expression(memory);

    if (!left_expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    

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
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')' after typeof");
    walker.next();
    return make_unique<NodeTypeof>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParseSizeof(Memory& memory) {
    walker.next(); // pass 'sizeof' token
    walker.next(); // pass '(' token
    auto expr = parse_expression(memory);
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')' after sizeof");
    walker.next();
    return make_unique<NodeSizeof>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParseAddressOf(Memory& memory) {
    walker.next(); // pass '&' token
    auto expr = parse_primary_expression(memory);
    return make_unique<NodeAddressOf>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParseDereference(Memory& memory) {
    walker.next(); // pass '*' token
    if (walker.CheckValue("(")) {
        Token start_token = *walker.get();
        walker.next();
        auto expr = parse_expression(memory);
        if (!expr) ERROR::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckValue(")")) ERROR::UnexpectedToken(*walker.get(), "')'");
        Token end_token = *walker.get();
        walker.next();
        return make_unique<NodeDereference>(std::move(expr), start_token, end_token);
    } else {
        Token start_token = *walker.get();
        auto expr = parse_expression(memory);
        if (!expr) ERROR::UnexpectedToken(*walker.get(), "primary expression or (expression)");
        Token end_token = *walker.get(-1);
        return make_unique<NodeDereference>(std::move(expr), start_token, end_token);
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
        !walker.CheckValue("namespace") && !walker.CheckValue("func")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }


    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeBaseVariableDecl*)decl.get())->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDecl*)decl.get())->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFuncDecl*)decl.get())->is_final = true;
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
        !walker.CheckValue("namespace") && !walker.CheckValue("func")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeBaseVariableDecl*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDecl*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFuncDecl*)decl.get())->is_static = true;
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
        !walker.CheckValue("namespace") && !walker.CheckValue("func")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeBaseVariableDecl*)decl.get())->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDecl*)decl.get())->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFuncDecl*)decl.get())->is_const = true;
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
        !walker.CheckValue("namespace") && !walker.CheckValue("func")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement(memory);

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeBaseVariableDecl*)decl.get())->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDecl*)decl.get())->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFuncDecl*)decl.get())->is_global = true;
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
    vector<unique_ptr<Node>> args;
    args.push_back(parse_expression(memory));
    while (true) {
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            auto expr = parse_expression(memory);
            if (!expr) {
                ERROR::UnexpectedToken(*walker.get(), "expression");
            }
            args.push_back(std::move(expr));
            continue;
        }
        break;
    }

    if (!walker.CheckValue(";")) {
        ERROR::UnexpectedToken(*walker.get(), "';'");
    }
    walker.next();
    return make_unique<NodeBaseOut>(std::move(args));
}

// Base out parsing
//
// outln <expr>;
unique_ptr<Node> ASTGenerator::ParseOutLn(Memory& memory) {
    walker.next(); // pass 'out'
    vector<unique_ptr<Node>> args;
    args.push_back(parse_expression(memory));
    while (true) {
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            auto expr = parse_expression(memory);
            if (!expr) {
                ERROR::UnexpectedToken(*walker.get(), "expression");
            }
            args.push_back(std::move(expr));
            continue;
        }
        break;
    }

    if (!walker.CheckValue(";")) {
        ERROR::UnexpectedToken(*walker.get(), "';'");
    }
    walker.next();
    return make_unique<NodeBaseOutLn>(std::move(args));
}
