#include "../twist-nodetemp.cpp"
#include "../twist-namespace.cpp"
#include "../twist-lambda.cpp"
#include "../twist-functions.cpp"

#include <iomanip>   
#include <limits>    

/*
 * NodeBaseOut / NodeBaseOutLn – узлы вывода в консоль (out и outln).
 *
 * Принимают список выражений, вычисляют их и выводят результат в std::cout.
 * NodeBaseOutLn дополнительно выводит перевод строки и сбрасывает буфер.
 *
 * Поля:
 *   expression – вектор выражений для вывода.
 *
 * print() обрабатывает различные типы значений:
 *   - INT, DOUBLE, BOOL, TYPE, NULL_T, NAMESPACE, STRING, CHAR – прямое строковое представление.
 *   - LAMBDA – "Lambda(arg1, arg2, ...)".
 *   - POINTER – "<тип>[0x<адрес>]".
 *   - FUNCTION – "Func'имя'(arg1:тип, ...) -> возврат".
 *
 * Для DOUBLE используется максимальная точность через std::setprecision.
 */

struct NodeBaseOut : public Node { NO_EVAL
    vector<unique_ptr<Node>> expression;
    NodeBaseOut(vector<unique_ptr<Node>> expr) : expression(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_OUT;
    }

    void print(std::ostream& buf, Value value, Memory& _memory) {
        if (value.type == STANDART_TYPE::INT) {
            buf << any_cast<int64_t&>(value.data);
        } else if (value.type == STANDART_TYPE::DOUBLE) {
            buf << std::setprecision(std::numeric_limits<NUMBER_ACCURACY>::max_digits10) << any_cast<NUMBER_ACCURACY>(value.data);
        } else if (value.type == STANDART_TYPE::BOOL) {
            buf << (any_cast<bool&>(value.data) ? "true" : "false");
        } else if (value.type == STANDART_TYPE::TYPE) {
            buf << any_cast<Type&>(value.data).pool;
        } else if (value.type == STANDART_TYPE::NULL_T) {
            buf << "null";
        } else if (value.type == STANDART_TYPE::NAMESPACE) {
            buf << any_cast<Namespace&>(value.data).name;
        } else if (value.type == STANDART_TYPE::STRING) {
            buf << any_cast<string&>(value.data);
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
        } else if (value.type.is_func()) {
            auto f = any_cast<Function*>(value.data);
            buf << "Func'" + f->name + "'(";
            // ОПТИМИЗАЦИЯ: Кэшируем eval_from результаты вне цикла
            for (int i = 0; i < f->arguments.size(); i++) {
                auto arg_type_val = f->arguments[i]->type_expr->eval_from(_memory);
                buf << f->arguments[i]->name << ":" << any_cast<Type>(arg_type_val.data).pool;

                if (i != f->arguments.size() - 1) buf << ", ";
            }
            auto ret_type_val = f->return_type->eval_from(_memory);
            buf << ") -> " << any_cast<Type>(ret_type_val.data).pool;
        }
    }

    void exec_from(Memory& _memory) override {
        for (auto& expr : expression) {
            auto value = expr->eval_from(_memory);
            print(std::cout, value, _memory);
        }
    }
};

struct NodeBaseOutLn : public Node { NO_EVAL
    vector<unique_ptr<Node>> expression;
    NodeBaseOutLn(vector<unique_ptr<Node>> expr) : expression(std::move(expr)) {
        this->NODE_TYPE = NodeTypes::NODE_OUTLN;
    }

    void print(std::ostream& buf, Value value, Memory& _memory) {
        if (value.type == STANDART_TYPE::INT) {
            buf << any_cast<int64_t>(value.data);
        } else if (value.type == STANDART_TYPE::DOUBLE) {
            buf << std::setprecision(std::numeric_limits<NUMBER_ACCURACY>::max_digits10) << any_cast<NUMBER_ACCURACY>(value.data);
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
        } else if (value.type.is_func()) {
            auto f = any_cast<Function*>(value.data);
            buf << "Func'" + f->name + "'(";
            // ОПТИМИЗАЦИЯ: Кэшируем eval_from результаты вне цикла
            for (int i = 0; i < f->arguments.size(); i++) {
                auto arg_type_val = f->arguments[i]->type_expr->eval_from(_memory);
                buf << f->arguments[i]->name << ":" << any_cast<Type>(arg_type_val.data).pool;

                if (i != f->arguments.size() - 1) buf << ", ";
            }
            auto ret_type_val = f->return_type->eval_from(_memory);
            buf << ") -> " << any_cast<Type>(ret_type_val.data).pool;
        }
    }

    void exec_from(Memory& _memory) override {
        for (auto& expr : expression) {
            auto value = expr->eval_from(_memory);
            print(std::cout, value, _memory);
        }
        std::cout << '\n';
        std::cout.flush();
    }
};