#include "../twist-nodetemp.cpp"
#include "../twist-args.cpp"
#include "../twist-structs.cpp"
#include "../twist-functions.cpp"
#include "../twist-errors.cpp"

#pragma once

struct NodeFunctionDeclaration : public Node { NO_EVAL
    string name;
    vector<Arg*> args;
    unique_ptr<Node> return_type;
    unique_ptr<Node> body;

    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;
    bool is_private = false;

    Token start_args_token;
    Token end_args_token;
    Token start_return_token;
    Token end_return_token;


    NodeFunctionDeclaration(string name, vector<Arg*> args, unique_ptr<Node> return_type, unique_ptr<Node> body,
                Token start_args_token, Token end_args_token, Token start_return_token, Token end_return_token) :
        name(name), args(std::move(args)), return_type(std::move(return_type)), body(std::move(body)),
        start_args_token(start_args_token), end_args_token(end_args_token), start_return_token(start_return_token), end_return_token(end_return_token) {
        this->NODE_TYPE = NodeTypes::NODE_FUNCTION_DECLARATION;
    }

    static Type extract_type_from_value(const Value& val, const Token& start_token, const Token& end_token, const string& entity_name) {
        if (val.type == STANDART_TYPE::TYPE) {
            return any_cast<Type>(val.data);
        }
        else if (!STANDART_TYPE::UNTYPED.is_sub_type(val.type)) {
            // Это структура
            return any_cast<Struct>(val.data).type;
        }
        // Иначе – ошибка: значение не является типом
        cout << "ERROR_TYPE" << endl;
        // return Type(); // unreachable
    }

    Type construct_type(Memory& _memory) {
        // Обработка возвращаемого типа (если есть)
        Type ret_type;
        if (return_type) {
            auto ret_val = return_type->eval_from(_memory);
            ret_type = extract_type_from_value(ret_val, start_return_token, end_return_token, "return type");
        }

        // Собираем типы аргументов
        vector<Type> arg_types;
        for (size_t i = 0; i < args.size(); ++i) {
            auto arg = args[i];
            if (!arg->type_expr) {
                // По логике парсера type_expr всегда присутствует, но на всякий случай
                ERROR::WaitedFuncTypeArgumentTypeSpecifier(start_args_token, end_args_token, arg->name);
            }
            auto arg_val = arg->type_expr->eval_from(_memory);
            Type t = extract_type_from_value(arg_val, start_args_token, end_args_token, "argument '" + arg->name + "'");
            arg_types.push_back(t);
        }

        // Создаём тип функции
        if (return_type) {
            return create_function_type(ret_type, arg_types);
        } else {
            return create_function_type(arg_types);
        }
    }

    void exec_from(Memory& _memory) override {
        Type function_type = construct_type(_memory);
        

        auto new_function_memory = make_shared<Memory>();
        //_memory.link_objects(*new_function_memory);
        
        auto func = NewFunction(name, new_function_memory, body.get(), args, std::move(return_type), function_type, start_args_token, end_args_token, start_return_token, end_return_token);
        
        auto object = CreateMemoryObject(func, function_type,&new_function_memory, is_const, is_static, is_final, is_global, is_private);
        if (_memory.check_literal(name))
            _memory.delete_variable(name);
        _memory.add_object(name, object);
        STATIC_MEMORY.register_object(object);
    }
};