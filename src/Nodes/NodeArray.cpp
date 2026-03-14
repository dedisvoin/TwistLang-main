#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "../twist-array.cpp"

struct NodeArray : public Node { NO_EXEC

    vector<tuple<Node*, Token, Token>> elements;
    Node* static_type;
    bool is_static = false;

    NodeArray(vector<tuple<Node*, Token, Token>> elements, Node* static_type = nullptr) : 
        elements(elements), static_type(static_type) {
        this->NODE_TYPE = NodeTypes::NODE_ARRAY;
    }

    Array construct_array(Memory& _memory) {
        vector<Value> evaled_elements;
        Type T = Type();
        if (!is_static) {
            if (elements.empty()) {
                T = Type("[, ~]"); // пустой массив
            } else {
                auto first = get<0>(elements[0])->eval_from(_memory);
                T = first.type;
                evaled_elements.push_back(first);
                for (size_t i = 1; i < elements.size(); ++i) {
                    auto value = get<0>(elements[i])->eval_from(_memory);
                    T = T | value.type;
                    evaled_elements.push_back(value);
                }
                T = Type("[" + T.pool + ", ~]");
            }
          
        } else {
            T = any_cast<Type>(static_type->eval_from(_memory).data);
            for (int i = 0; i < elements.size(); i++) {
                auto value = get<0>(elements[i])->eval_from(_memory);
                if (!IsTypeCompatible(T.parse_array_type().first, value.type)) {
                    ERROR::InvalidArrayElementType(get<1>(elements[i]), get<2>(elements[i]), T.pool, value.type.pool, i);
                }
                evaled_elements.push_back(value);
            }
        }
        return Array(T, std::move(evaled_elements));
    }

    Value eval_from(Memory& _memory) override {
        auto arr = construct_array(_memory);

        return Value(arr.type, arr);
    }
};