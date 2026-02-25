#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"

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
