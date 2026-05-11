#include "../twist-nodetemp.cpp"
#include "../twist-err.cpp"

struct NodeInput : public Node { NO_EXEC
    Node* expr;
    Token start_token;
    Token end_token;

    NodeInput(Node* expr, Token start_token, Token end_token)
        : expr(expr), start_token(start_token), end_token(end_token) {
            this->NODE_TYPE = NodeTypes::NODE_INPUT;
    }

    Value eval_from(Memory* _memory) override {
        #ifndef SERVER
            if (expr) {

                auto value = expr->eval_from(_memory);

                if (value.type == STANDART_TYPE::STRING) {
                    cout << any_cast<string>(value.data);
                } else if (value.type == STANDART_TYPE::CHAR) {
                    cout << any_cast<char>(value.data);
                } else {
                    throw ERROR_THROW::IncompartableInputType(start_token, end_token, value.type);
                }
            }

            string _input;
            getline(cin, _input);

            if (_input.empty()) 
                return NewNull();
            
            auto value = NewString(_input);
            return NewString(_input);
            
        #else
            ERROR_THROW::InputWarning(start_token, end_token).Write();
            return NewNull();
        #endif

    }
};
