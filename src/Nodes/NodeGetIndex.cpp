#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-array.cpp"


struct NodeGetIndex : public Node { NO_EXEC
    Node* expr;
    Node* index_expr;
    Token start_token;
    Token end_token;

    NodeGetIndex(Node* expr, Node* index_expr, Token start_token, Token end_token)
        : expr(expr), index_expr(index_expr), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_GET_BY_INDEX;
    }

    Value eval_from(Memory* _memory) override {
        auto value = expr->eval_from(_memory);

        auto index_value = index_expr->eval_from(_memory);

        if (index_value.type != STANDART_TYPE::INT) {
            ERROR::InvalidArrayIndex(start_token, index_value.type.pool);
        }

        if (value.type.is_array_type()) {
            auto arr = any_cast<Array>(value.data);
            int64_t idx = any_cast<int64_t>(index_value.data);

            if (arr.get_size() <= idx) 
                ERROR::ArrayIndexOutOfRange(start_token, idx, arr.get_size());
            
            return arr.values[idx];

        } else if (value.type == STANDART_TYPE::STRING) {
            auto arr = any_cast<string>(value.data);
            int64_t idx = any_cast<int64_t>(index_value.data);

            if (arr.size() <= idx) 
                ERROR::ArrayIndexOutOfRange(start_token, idx, arr.size());
            

            return NewChar(arr[idx]);
        }
        
    }
};