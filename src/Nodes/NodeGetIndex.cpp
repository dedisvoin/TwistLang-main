#include "../twist-nodetemp.cpp"
#include "../twist-array.cpp"
#include "../twist-err.cpp"


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

        if (index_value.type != STANDART_TYPE::INT) 
            throw ERROR_THROW::ArrayInvalidIndexType(start_token, end_token, index_value.type);

        int64_t idx = any_cast<int64_t>(index_value.data);

        // Обработка для массивов
        if (value.type.is_array_type()) {
            auto arr = any_cast<Array>(value.data);

            // Отрицательный индекс: отсчёт с конца
            if (idx < 0) 
                idx = arr.get_size() + idx;

            // Проверка после пересчёта
            if (idx < 0 || idx >= arr.get_size()) 
                throw ERROR_THROW::ArrayIndexOutOfRange(start_token, end_token, idx, arr.get_size());

            return arr.values[idx];
        }
        // Обработка для строк
        else if (value.type == STANDART_TYPE::STRING) {
            auto& str = any_cast<string&>(value.data);

            if (idx < 0) 
                idx = static_cast<int64_t>(str.size()) + idx;

            if (idx < 0 || idx >= static_cast<int64_t>(str.size())) 
                throw ERROR_THROW::ArrayIndexOutOfRange(start_token, end_token, idx, str.size());

            return NewChar(str[idx]);
        }
        // Неподдерживаемый тип
        else {
            throw ERROR_THROW::ArrayInvalidIndexType(start_token, end_token, value.type);
        }
    }
};