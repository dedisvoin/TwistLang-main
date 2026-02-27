#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

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


            if (STATIC_MEMORY.is_registered(address)) {}
            
            else {
                cout << "ERROR ADDR " << address << endl;
                exit(-1);
            }
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


