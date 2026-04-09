#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"
#include <any>

#include "../twist-namespace.cpp"
#include "../twist-nodetemp.cpp"
#include "../twist-lambda.cpp"


struct NodeEcho : public Node { NO_EVAL
    vector<Node*> expressions;
    Token start_token;
    Token end_token;

    NodeEcho(vector<Node*> expressions, Token start_token, Token end_token)
        : expressions(expressions), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_ECHO;
    }


    void exec_from(Memory* _memory) override {
        #ifdef SERVER
        string echo_message;
        for (size_t i = 0; i < expressions.size(); i++) {
            auto value = expressions[i]->eval_from(_memory);
        
            if (value.type == STANDART_TYPE::TYPE)
                echo_message = echo_message + any_cast<Type&>(value.data).pool;
            else if (value.type == STANDART_TYPE::STRING)
                echo_message = echo_message + any_cast<string&>(value.data);
            else if (value.type == STANDART_TYPE::CHAR)
                echo_message = echo_message + any_cast<char>(value.data);
            else if (value.type == STANDART_TYPE::BOOL) {
                if (any_cast<bool>(value.data)) {
                    echo_message = echo_message + "true";
                } else {
                    echo_message = echo_message + "false";
                }
            }
            else if (value.type == STANDART_TYPE::INT) {
                echo_message = echo_message + to_string(any_cast<int64_t>(value.data));
            } 
            else if (value.type == STANDART_TYPE::DOUBLE) {
                echo_message = echo_message + to_string(any_cast<NUMBER_ACCURACY>(value.data));
            }
            else if (value.type == STANDART_TYPE::NULL_T) {
                echo_message =  echo_message + "null";
            }
            else if (value.type == STANDART_TYPE::NAMESPACE) {
                echo_message = echo_message + "namespace " + any_cast<Namespace>(value.data).name;
            }
            else if (value.type == STANDART_TYPE::LAMBDA) {
                echo_message = echo_message + "Lambda(";
                auto lambda = any_cast<Lambda*>(value.data);
                for (int i = 0; i < lambda->arguments.size(); i++) {
                    echo_message = echo_message + any_cast<Type>(lambda->arguments[i]->type_expr->eval_from(_memory).data).pool;
                    if (i != lambda->arguments.size() - 1) echo_message = echo_message + ", ";
                }
                echo_message = echo_message + ") -> ";
                echo_message = echo_message + any_cast<Type>(lambda->return_type->eval_from(_memory).data).pool;
            } else if (value.type.is_pointer()) {
                echo_message = echo_message + value.type.pool + "[0x" + to_string(any_cast<int>(value.data)) + "]";
            }
            
        }
        throw ERROR_THROW::Echo(start_token, end_token, echo_message);
        #endif
    }
};