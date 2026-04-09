#include "../twist-functions.cpp"
#include "../twist-nodetemp.cpp"
#include "../twist-structs.cpp"
#include "../twist-errors.cpp"
#include "../twist-err.cpp"
#include "../twist-lambda.cpp"
#include "../twist-array.cpp"
#include "../twist-namespace.cpp"

#include "NodeReturn.cpp"
#include <any>
#include <cstdint>


struct NodeCall : public Node { NO_EXEC
    Node* callable;
    vector<Node*> args;
    Token start_callable;
    Token end_callable;

    static Type extract_type_from_value(const Value& val,
                                     const Token& start_token,
                                     const Token& end_token,
                                     const string& context) {
        if (val.type == STANDART_TYPE::TYPE) {
            return any_cast<Type>(val.data);
        }
        else if (!STANDART_TYPE::TYPES.is_sub_type(val.type)) {
            // Пользовательский тип (структура) – возвращаем его тип
            return val.type;
        }
        ERROR::WaitedTypeExpression(start_token);
        // unreachable
    }

    NodeCall(Node* callable, vector<Node*> args, Token start_callable, Token end_callable) :
        callable(callable), args(args), start_callable(start_callable), end_callable(end_callable) {
        this->NODE_TYPE = NodeTypes::NODE_CALL;
    }

    Value call_lambda(Value &value, Memory* _memory) {
        auto lambda = any_cast<Lambda*>(value.data);

        // Создаём новую память для этого вызова
        auto call_memory = new Memory();

        // Копируем глобальные объекты из памяти лямбды (разделяемое владение)
        lambda->memory->link_objects(call_memory);

        // Добавляем саму лямбду в новую память, если у неё есть имя (для рекурсии)
        if (!lambda->name.empty()) {
            call_memory->add_object(lambda->name, value, value.type,
                                    true, true, true, true, false);
        }

        // Проверка количества аргументов
        if (lambda->arguments.size() != args.size()) {
            throw ERROR_THROW::InvalidLambdaArgumentCount(start_callable, end_callable,
                lambda->start_args_token, lambda->end_args_token,
                lambda->arguments.size(), args.size());
        }

        // Добавляем аргументы в новую память
        for (size_t i = 0; i < args.size(); i++) {
            auto arg_value = args[i]->eval_from(_memory);

            // Проверка типа аргумента (используем исходную память лямбды для вычисления ожидаемого типа)
            if (lambda->arguments[i]->type_expr) {
                auto expected_type_val = lambda->arguments[i]->type_expr->eval_from(lambda->memory);
                Type expected = extract_type_from_value(expected_type_val,
                                    lambda->start_args_token, lambda->end_args_token,
                                    "parameter '" + lambda->arguments[i]->name + "'");
                if (!arg_value.type.is_sub_type(expected)) {
                    throw ERROR_THROW::InvalidLambdaArgumentType(start_callable, end_callable,
                        lambda->start_args_token, lambda->end_args_token,
                        expected, arg_value.type, lambda->arguments[i]->name);
                }
            }

            call_memory->add_object_in_lambda(lambda->arguments[i]->name, arg_value,
                                            lambda->arguments[i]->is_global);
        }

        Value result = NewNull();
        try {
            result = ((Node*)(lambda->expr))->eval_from(call_memory);
        }
        catch (Error err) {
            
            if (err.message_type == 1) {
                string saved_message = err.message;
                err.message = "...";
                throw ERROR_THROW::CallError(start_callable, end_callable, "anonymous-lambda", new Error(err), err.message_type, saved_message);
            } else if (err.message_type == 2) {
                string saved_message = err.message;
                err.message = "...";
                throw ERROR_THROW::CallError(start_callable, end_callable, "anonymous-lambda", err.message_type, saved_message);
                
            }
            throw ERROR_THROW::CallError(start_callable, end_callable, "anonymous-lambda",new Error(err), err.message_type);
        }

        

        // Проверка типа возвращаемого значения (если задан)
        if (lambda->return_type) {
            auto expected_type_val = lambda->return_type->eval_from(lambda->memory);
            Type expected = extract_type_from_value(expected_type_val,
                                lambda->start_type_token, lambda->end_type_token,
                                "return type");
            if (!result.type.is_sub_type(expected)) {
                throw ERROR_THROW::InvalidLambdaReturnType(start_callable, end_callable,
                    lambda->start_type_token, lambda->end_type_token,
                    expected, result.type);
            }
        }

        return result;
    }

    Value call_function(Value &value, Memory* _memory) {
        auto func = any_cast<Function*>(value.data);
        _memory->link_objects(func->memory);
        Memory saved_mem = *func->memory;
        func->memory->add_object_in_func(func->name, value, value.type, false, false, false, true);

        size_t arg_idx = 0;  // текущий индекс в списке переданных аргументов args

        // Проходим по всем объявленным параметрам функции
        for (size_t param_idx = 0; param_idx < func->arguments.size(); ++param_idx) {
            Arg* param = func->arguments[param_idx];

            if (!param->is_variadic) {
                // --- Обычный (не variadic) параметр ---
                Value arg_value = NewNull();

                if (arg_idx < args.size()) {
                    arg_value = args[arg_idx]->eval_from(_memory);
                    ++arg_idx;
                }
                else if (param->default_parameter) {
                    arg_value = param->default_parameter->eval_from(_memory);
                }
                else {
                    ERROR::MissingFuncArgument(start_callable, end_callable,
                        func->start_args_token, func->end_args_token,
                        param->name, param_idx);
                }
                Type expected;
                // Проверка типа
                if (param->type_expr) {

                    auto expected_type_val = param->type_expr->eval_from(func->memory);

                    expected = extract_type_from_value(expected_type_val,
                                         func->start_args_token, func->end_args_token,
                                         "parameter '" + param->name + "'");
                    if (!arg_value.type.is_sub_type(expected)) {
                        ERROR::InvalidFuncArgumentType(start_callable, end_callable,
                            func->start_args_token, func->end_args_token,
                            expected, arg_value.type, param->name);
                    }
                }
                if (func->memory->check_literal(param->name)) {
                    MemoryObject* existing = func->memory->get_variable(param->name);
                    if (existing->modifiers.is_global) {
                        ERROR::ArgumentShadowsGlobal(start_callable, end_callable, func->name, param->name);
                    }
                }
                func->memory->add_object_in_func(param->name, arg_value, expected,
                    param->is_const, param->is_static,
                    param->is_final, param->is_global);
            }
            else {
                // --- Variadic параметр ---
                int64_t variadic_size = 0;

                // Определяем размер variadic-блока
                if (param->variadic_size) {   // фиксированный размер
                    Value size_val = param->variadic_size->eval_from(_memory);
                    if (size_val.type != STANDART_TYPE::INT) {
                        ERROR::InvalidVariadicSizeExpression(start_callable, end_callable,
                            size_val.type.pool);
                    }
                    variadic_size = any_cast<int64_t>(size_val.data);
                    if (variadic_size < 0) {
                        ERROR::InvalidVariadicSizeExpression(start_callable, end_callable,
                            "negative size");
                    }
                }
                else {   // динамический размер – все оставшиеся аргументы
                    variadic_size = args.size() - arg_idx;
                }

                // Проверяем, хватает ли переданных аргументов
                if (arg_idx + variadic_size > args.size()) {
                    ERROR::MissingFuncArgument(start_callable, end_callable,
                        func->start_args_token, func->end_args_token,
                        param->name, param_idx);
                }

                // Собираем значения в массив
                vector<Value> elements;
                Type element_type;   // будет определён ниже, если есть type_expr
                bool has_explicit_type = (param->type_expr != nullptr);

                if (has_explicit_type) {
                    auto expected_type_val = param->type_expr->eval_from(func->memory);
                    element_type = any_cast<Type>(expected_type_val.data);
                }

                for (int64_t i = 0; i < variadic_size; ++i) {
                    Value elem = args[arg_idx + i]->eval_from(_memory);

                    // Проверка типа элемента
                    if (has_explicit_type && !elem.type.is_sub_type(element_type)) {
                        ERROR::InvalidVariadicArgumentType(start_callable, end_callable,
                            element_type.pool, elem.type.pool, i);
                    }

                    elements.push_back(elem);
                }
                arg_idx += variadic_size;

                // Создаём тип массива
                string elem_type_str = has_explicit_type ? element_type.pool : "auto";
                string array_type_str;
                if (param->variadic_size) {
                    array_type_str = "[" + elem_type_str + ", " + to_string(variadic_size) + "]";
                } else {
                    array_type_str = "[" + elem_type_str + ", ~]";
                }
                Type array_type(array_type_str);

                // Формируем значение-массив
                Array arr(array_type, std::move(elements));
                Value array_value(array_type, std::move(arr));

                if (func->memory->check_literal(param->name)) {
                    MemoryObject* existing = func->memory->get_variable(param->name);
                    if (existing->modifiers.is_global) {
                        ERROR::ArgumentShadowsGlobal(start_callable, end_callable, func->name, param->name);
                    }
                }

                // Добавляем в память функции под именем параметра
                func->memory->add_object_in_func(param->name, array_value, array_value.type,
                    param->is_const, param->is_static,
                    param->is_final, param->is_global);
            }
        }

        // Проверка, что не осталось лишних аргументов
        if (arg_idx < args.size()) {
            ERROR::InvalidFuncArgumentCount(start_callable, end_callable,
                func->start_args_token, func->end_args_token,
                arg_idx, args.size());
        }
        
        try {
            ((Node*)(func->body))->exec_from(func->memory);
            return NewNull();
        }
        catch (Error err) {
            if (err.message_type == 1) {
                string saved_message = err.message;
                err.message = "...";
                throw ERROR_THROW::CallError(start_callable, end_callable, func->name, new Error(err), err.message_type, saved_message);
            } else if (err.message_type == 2) {
                string saved_message = err.message;
                err.message = "...";
                throw ERROR_THROW::CallError(start_callable, end_callable, func->name, new Error(err), err.message_type, saved_message);
                
            }
            throw ERROR_THROW::CallError(start_callable, end_callable, func->name, new Error(err), err.message_type);
        }
        catch (Return _value) {
            if (func->return_type) {
                auto expected_ret = func->return_type->eval_from(func->memory);
                Type expected = extract_type_from_value(expected_ret,
                                         func->start_return_type_token, func->end_return_type_token,
                                         "return type");
                if (!_value.value.type.is_sub_type(expected)) {
                    ERROR::InvalidLambdaReturnType(start_callable, end_callable,
                        func->start_return_type_token, func->end_return_type_token,
                        expected, _value.value.type);
                }
            }

            *func->memory = saved_mem;
            return _value.value;
        }
    }

    Value call_struct(Value &value, Memory* _memory) {
        // Убедимся, что вызываемый объект действительно является структурой


        Struct* struct_builder = any_cast<Struct*>(value.data);
        
        auto new_memory = new Memory();
        _memory->link_objects(new_memory);
        struct_builder->memory->copy_objects(*new_memory);
        
        
        if (struct_builder->body)
            struct_builder->body->exec_from(new_memory);
        
        
        
        
        // Создаём новую структуру с той же памятью и именем
        auto new_struct = NewStruct(new_memory, struct_builder->name);
        
        return new_struct;
    }

    Value call_string(Value &value_, Memory* _memory) {
        if (args.size() != 1)
            throw ERROR_THROW::InvalidStringArgumentCount(start_callable, end_callable, args.size());

        string new_string;
        auto value = args[0]->eval_from(_memory);
        if (value.type == STANDART_TYPE::TYPE)
            new_string = any_cast<Type&>(value.data).pool;
        else if (value.type == STANDART_TYPE::STRING)
            new_string = any_cast<string&>(value.data);
        else if (value.type == STANDART_TYPE::CHAR)
            new_string = any_cast<char>(value.data);
        else if (value.type == STANDART_TYPE::BOOL) {
            if (any_cast<bool>(value.data)) {
                new_string = "true";
            } else {
                new_string = "false";
            }
        }
        else if (value.type == STANDART_TYPE::INT) {
            new_string = to_string(any_cast<int64_t>(value.data));
        } 
        else if (value.type == STANDART_TYPE::DOUBLE) {
            new_string = to_string(any_cast<NUMBER_ACCURACY>(value.data));
        }
        else if (value.type == STANDART_TYPE::NULL_T) {
            new_string = "null";
        }
        else if (value.type == STANDART_TYPE::NAMESPACE) {
            new_string = "namespace " + any_cast<Namespace>(value.data).name;
        }
        else if (value.type == STANDART_TYPE::LAMBDA) {
            new_string = "Lambda(";
            auto lambda = any_cast<Lambda*>(value.data);
            for (int i = 0; i < lambda->arguments.size(); i++) {
                new_string = new_string + any_cast<Type>(lambda->arguments[i]->type_expr->eval_from(_memory).data).pool;
                if (i != lambda->arguments.size() - 1) new_string = new_string + ", ";
            }
            new_string = new_string + ") -> ";
            new_string = new_string + any_cast<Type>(lambda->return_type->eval_from(_memory).data).pool;
        } else if (value.type.is_pointer()) {
            new_string = value.type.pool + "[0x" + to_string(any_cast<int>(value.data)) + "]";
        }

        return NewString(new_string);
    }

    Value eval_from(Memory* _memory) override {

        auto value = callable->eval_from(_memory);



        // if (value.type == STANDART_TYPE::METHOD) {
        //     auto method = any_cast<Method>(value.data);
        //     auto saved_memory = method.func->memory;
        //     method.func->memory = method.instance_memory;
        //     Value func_val(method.func->type, method.func);
        //     Value result = call_function(func_val, _memory);
        //     method.func->memory = saved_memory;
        //     return result;
        // }

        
        if (value.type == STANDART_TYPE::LAMBDA) {
            return call_lambda(value, _memory);
        }
        if (value.type == STANDART_TYPE::TYPE && any_cast<Type>(value.data).pool == "String") {
            return call_string(value, _memory);
        }
        else if (value.type.is_func()) {
            return call_function(value, _memory);
        }
        else if (!value.type.is_sub_type(STANDART_TYPE::TYPES)) {
            return call_struct(value, _memory);
        }

        throw ERROR_THROW::UncallableType(start_callable, end_callable, value.type);
    }
};
