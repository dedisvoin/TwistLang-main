#include "twist-tokenwalker.cpp"
#include "twist-namespace.cpp"
#include "twist-tokens.cpp"
#include "twist-errors.cpp"
#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "twist-array.cpp"
#include "twist-args.cpp"

#include "Nodes/NodeNumber.cpp"
#include "Nodes/NodeString.cpp"
#include "Nodes/NodeChar.cpp"
#include "Nodes/NodeBool.cpp"
#include "Nodes/NodeNull.cpp"
#include "Nodes/NodeLiteral.cpp"

#include "Nodes/NodeNamespaceResolution.cpp"
#include "Nodes/NodeObjectResolution.cpp"
#include "Nodes/NodeScopes.cpp"
#include "Nodes/NodeUnary.cpp"
#include "Nodes/NodeBinary.cpp"
#include "Nodes/NodeVariableDeclaration.cpp"
#include "Nodes/NodeOut.cpp"
#include "Nodes/NodeBlock.cpp"
#include "Nodes/NodeIf.cpp"
#include "Nodes/NodeBreak.cpp"
#include "Nodes/NodeContinue.cpp"
#include "Nodes/NodeWhile.cpp"
#include "Nodes/NodeDoWhile.cpp"
#include "Nodes/NodeFor.cpp"
#include "Nodes/NodeInput.cpp"

#include "Nodes/NodeTypeof.cpp"
#include "Nodes/NodeSizeof.cpp"

#include "Nodes/NodeAssert.cpp"
#include "Nodes/NodeExit.cpp"
#include "Nodes/NodeVariableEqual.cpp"
#include "Nodes/NodeNamespaceDeclaration.cpp"
#include "Nodes/NodeNamespaceAnonim.cpp"
#include "Nodes/NodeNew.cpp"
#include "Nodes/NodeFunctionDeclaration.cpp"
#include "Nodes/NodeBlockOfDeclarations.cpp"
#include "Nodes/NodeReturn.cpp"
#include "Nodes/NodeExpressionState.cpp"
#include "Nodes/NodeLambda.cpp"
#include "Nodes/NodeStructDeclaration.cpp"
#include "Nodes/NodeAddresOf.cpp"
#include "Nodes/NodeDereference.cpp"
#include "Nodes/NodeDelete.cpp"
#include "Nodes/NodeLeftDereference.cpp"
#include "Nodes/NodeCall.cpp"
#include "Nodes/NodeFunctionType.cpp"

#include <vcruntime_startup.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <any>

#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"


using namespace std;



// NodeNewArrayType:
// Подробное описание:
// Нода для описания типа массива: принимает тип элемента и опциональный
// размер. Возвращает объект Type в виде строкового представления типа.
struct NodeNewArrayType : public Node { NO_EXEC
    unique_ptr<Node> type_expr;
    unique_ptr<Node> size_expr;
    Token start_token;
    Token end_token;

    NodeNewArrayType(unique_ptr<Node> type_expr, unique_ptr<Node> size_expr, Token start_token, Token end_token)
        : type_expr(std::move(type_expr)), size_expr(std::move(size_expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_ARRAY_TYPE;
    }

    Value eval_from(Memory& _memory) override {
        if (type_expr) {
            auto type_value = type_expr->eval_from(_memory);

            if (type_value.type != STANDART_TYPE::TYPE) {
                ERROR::InvalidArrayTypeExpression(start_token, end_token);
            }

            if (size_expr) {
                auto size_value = size_expr->eval_from(_memory);
                if (size_value.type != STANDART_TYPE::INT) {
                    ERROR::InvalidArraySize(start_token);
                }

                auto ret_type = "[" + any_cast<Type>(type_value.data).pool + ", " + to_string(any_cast<int64_t>(size_value.data)) + "]";
                return Value(STANDART_TYPE::TYPE, Type(ret_type));
            }

            auto ret_type = "[" + any_cast<Type>(type_value.data).pool + ", ~]";

            return Value(STANDART_TYPE::TYPE, Type(ret_type));
        } else {
            auto ret_type = "[, ~]";

            return Value(STANDART_TYPE::TYPE, Type(ret_type));
        }
    }
};

// NodeArray:
// Подробное описание:
// Нода литерала массива: содержит список выражений-элементов, вычисляет их
// значения, выводит объединённый тип массива и возвращает Value с типом
// массивa и самим массивом.
struct NodeArray : public Node { NO_EXEC

    vector<tuple<unique_ptr<Node>, Token, Token>> elements;
    unique_ptr<Node> static_type;
    bool is_static = false;

    NodeArray(vector<tuple<unique_ptr<Node>, Token, Token>> elements, unique_ptr<Node> static_type = nullptr) : 
        elements(std::move(elements)), static_type(std::move(static_type)) {
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

// NodeGetIndex:
// Подробное описание:
// Нода индексирования массива/коллекции: вычисляет выражение и индекс,
// проверяет тип индекса и диапазон, затем возвращает соответствующий элемент.
struct NodeGetIndex : public Node { NO_EXEC
    unique_ptr<Node> expr;
    unique_ptr<Node> index_expr;
    Token start_token;
    Token end_token;

    NodeGetIndex(unique_ptr<Node> expr, unique_ptr<Node> index_expr, Token start_token, Token end_token)
        : expr(std::move(expr)), index_expr(std::move(index_expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_GET_BY_INDEX;
    }

    Value eval_from(Memory& _memory) override {
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


// NodeArrayPush:
// Подробное описание:
// Нода операции добавления элемента в массив (`<-`). Поддерживает оптимизацию
// для простых переменных и namespace-доступа — модификация выполняется прямо
// в памяти без копирования; для выражений использует копию массива.
struct NodeArrayPush : public Node { NO_EXEC
    unique_ptr<Node> left_expr;
    unique_ptr<Node> right_expr;
    Token start_token;
    Token end_token;

    NodeArrayPush(unique_ptr<Node> left_expr, unique_ptr<Node> right_expr, Token start_token, Token end_token)
        : left_expr(std::move(left_expr)), right_expr(std::move(right_expr)), start_token(start_token), end_token(end_token) {
        this->NODE_TYPE = NodeTypes::NODE_ARRAY_PUSH;
    }

    Value eval_from(Memory& _memory) override {
        if (left_expr->NODE_TYPE == NodeTypes::NODE_SCOPES) {
            left_expr = std::move(((NodeScopes*)(left_expr.get()))->expression);
        }
        // ОПТИМИЗАЦИЯ: Проверяем тип левого выражения
        // Если это простая переменная (NODE_LITERAL), модифицируем её в памяти напрямую
        if (left_expr->NODE_TYPE == NodeTypes::NODE_LITERAL) {
            string var_name = ((NodeLiteral*)left_expr.get())->name;
            MemoryObject* var_obj = _memory.get_variable(var_name);

            if (var_obj && var_obj->value.type.is_array_type()) {
                auto right_value = right_expr->eval_from(_memory);
                // Проверка типа элемента массива
                auto& arr = any_cast<Array&>(var_obj->value.data);
                auto arr_type_pair = arr.type.parse_array_type();
                string elem_type_str = arr_type_pair.first;
                if (elem_type_str != "") {
                    Type expected(elem_type_str);
                    if (!IsTypeCompatible(expected, right_value.type)) {
                        ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                    }
                }
                // Модифицируем массив ПРЯМО в памяти БЕЗ копирования
                arr.values.emplace_back(right_value);
                return var_obj->value;
            }
        }
        // Если это доступ через namespace (NODE_NAME_RESOLUTION), тоже модифицируем напрямую
        else if (left_expr->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
            NodeNamespaceResolution* resolution = (NodeNamespaceResolution*)left_expr.get();
            Value ns_value = resolution->namespace_expr->eval_from(_memory);

            if (ns_value.type == STANDART_TYPE::NAMESPACE) {
                auto& ns = any_cast<Namespace&>(ns_value.data);
                string var_name = resolution->current_name;

                if (ns.memory->check_literal(var_name)) {
                    MemoryObject* var_obj = ns.memory->get_variable(var_name);
                    if (var_obj && var_obj->value.type.is_array_type()) {
                        auto right_value = right_expr->eval_from(_memory);
                        auto& arr = any_cast<Array&>(var_obj->value.data);
                        auto arr_type_pair = arr.type.parse_array_type();
                        string elem_type_str = arr_type_pair.first;
                        if (elem_type_str != "") {
                            Type expected(elem_type_str);
                            if (!IsTypeCompatible(expected, right_value.type)) {
                                ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                            }
                        }
                        arr.values.emplace_back(right_value);
                        return var_obj->value;
                    }
                }
            }
        }
        // ОБРАБОТКА РАЗЫМЕНОВАНИЯ УКАЗАТЕЛЯ
        else if (left_expr->NODE_TYPE == NodeTypes::NODE_DEREFERENCE) {
            NodeDereference* deref = (NodeDereference*)left_expr.get();

            // Вычисляем выражение, которое должно быть указателем
            Value ptr_value = deref->expr->eval_from(_memory);

            if (!ptr_value.type.is_pointer()) {
                ERROR::InvalidArrayPushType(start_token, end_token, ptr_value.type.pool);
            }

            // Получаем адрес из указателя
            int address = any_cast<int>(ptr_value.data);

            // Находим объект в STATIC_MEMORY
            MemoryObject* obj = STATIC_MEMORY.get_by_address(address);
            if (!obj) {
                ERROR::InvalidArrayPushType(start_token, end_token, "null pointer");
            }

            if (!obj->value.type.is_array_type()) {
                ERROR::InvalidArrayPushType(start_token, end_token, obj->value.type.pool);
            }

            auto right_value = right_expr->eval_from(_memory);
            auto& arr = any_cast<Array&>(obj->value.data);
            auto arr_type_pair = arr.type.parse_array_type();
            string elem_type_str = arr_type_pair.first;
            if (elem_type_str != "") {
                Type expected(elem_type_str);
                if (!IsTypeCompatible(expected, right_value.type)) {
                    ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                }
            }
            arr.values.emplace_back(right_value);

            // Возвращаем значение из памяти (не копию)
            return obj->value;
        }
        // ОБРАБОТКА СЛОЖНЫХ ВЫРАЖЕНИЙ С РАЗЫМЕНОВАНИЕМ И ДОСТУПОМ К ПОЛЯМ
        else if (left_expr->NODE_TYPE == NodeTypes::NODE_GET_BY_INDEX) {
            // Если это индексация, модифицируем копию
            auto left_value = left_expr->eval_from(_memory);
            auto right_value = right_expr->eval_from(_memory);

            if (!left_value.type.is_array_type()) {
                ERROR::InvalidArrayPushType(start_token, end_token, left_value.type.pool);
            }
            auto& arr = any_cast<Array&>(left_value.data);
            auto arr_type_pair = arr.type.parse_array_type();
            string elem_type_str = arr_type_pair.first;
            if (elem_type_str != "") {
                Type expected(elem_type_str);
                if (!IsTypeCompatible(expected, right_value.type)) {
                    ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                }
            }
            arr.values.emplace_back(right_value);
            return left_value;
        }
        
        // FALLBACK: Для других случаев (выражений) работаем с копией
        auto left_value = left_expr->eval_from(_memory);
        auto right_value = right_expr->eval_from(_memory);

        if (!left_value.type.is_array_type()) {
            ERROR::InvalidArrayPushType(start_token, end_token, left_value.type.pool);
        }

        // Модифицируем копию и возвращаем её
        {
            auto& arr = any_cast<Array&>(left_value.data);
            auto arr_type_pair = arr.type.parse_array_type();
            string elem_type_str = arr_type_pair.first;
            if (elem_type_str != "") {
                Type expected(elem_type_str);
                if (!IsTypeCompatible(expected, right_value.type)) {
                    ERROR::InvalidArrayElementTypeOnPush(start_token, end_token, expected.pool, right_value.type.pool);
                }
            }
            arr.values.emplace_back(right_value);
        }
        return left_value;
    }

};


struct Context {
    Memory memory;
    vector<unique_ptr<Node>> nodes;

    Context() = default;

    // Конструктор перемещения
    Context(vector<unique_ptr<Node>>&& nodes, Memory&& mem)
        : nodes(std::move(nodes)), memory(std::move(mem)) {}

    Context(vector<unique_ptr<Node>>&& nodes, Memory& mem)
        : nodes(std::move(nodes)), memory(std::move(mem)) {}

    // Запрещаем копирование
    Context(const Context&) = delete;

    Context& operator=(const Context&) = delete;

    // Разрешаем перемещение
    Context(Context&& other) noexcept
        : nodes(std::move(other.nodes)), memory(std::move(other.memory)) {}

    Context& operator=(Context&& other) noexcept {
        if (this != &other) {
            nodes = std::move(other.nodes);
            memory = std::move(other.memory);
        }
        return *this;
    }

    void set_nodes(vector<unique_ptr<Node>>&& new_nodes) {
        nodes = std::move(new_nodes);  // Используем move
    }

    void set_memory(Memory&& new_memory) {
        memory = std::move(new_memory);  // Используем move
    }
};


struct ASTGenerator {
    vector<unique_ptr<Node>> nodes;
    TokenWalker& walker;
    string file_name;

    // Declarations
    unique_ptr<Node> ParseBaseVariableDecl();
    unique_ptr<Node> ParseFinalVariableDecl();
    unique_ptr<Node> ParseStaticVariableDecl();
    unique_ptr<Node> ParseConstVariableDecl();
    unique_ptr<Node> ParseGlobalVariableDecl();
    unique_ptr<Node> ParsePrivateVariableDecl();
    unique_ptr<Node> ParseBlockDecl(string modifier);

    // Input and outputs
    unique_ptr<Node> ParseOut();
    unique_ptr<Node> ParseOutLn();
    unique_ptr<Node> ParseInput();

    // Delete and new
    unique_ptr<Node> ParseDelete();
    unique_ptr<Node> ParseNew();

    // Loops
    unique_ptr<Node> ParseWhile();
    unique_ptr<Node> ParseDoWhile();
    unique_ptr<Node> ParseFor();

    // Block of instructs
    unique_ptr<Node> ParseBlock();
    unique_ptr<Node> ParseScopes();

    // Namespace
    unique_ptr<Node> ParseNameSpaceDecl();
    unique_ptr<Node> ParseNamespace();
    unique_ptr<Node> ParseNameResolution(unique_ptr<Node> expression);
    unique_ptr<Node> ParseObjectResolution(unique_ptr<Node> expression);

    // Conditions
    unique_ptr<Node> ParseIf();
    unique_ptr<Node> ParseIfExpr();

    // Call
    unique_ptr<Node> ParseCall(unique_ptr<Node> expr, Token start, Token end);

    // Base literals
    unique_ptr<Node> ParseNumber();
    unique_ptr<Node> ParseString();
    unique_ptr<Node> ParseChar();
    unique_ptr<Node> ParseBool();
    unique_ptr<Node> ParseNull();
    unique_ptr<Node> ParseLiteral();

    // Thread instructions
    unique_ptr<Node> ParseBreak();
    unique_ptr<Node> ParseContinue();
    unique_ptr<Node> ParseExit();
    unique_ptr<Node> ParseReturn();
    
    // Assert
    unique_ptr<Node> ParseAssert();

    // Pointers
    unique_ptr<Node> ParseAddressOf();
    unique_ptr<Node> ParseDereference();
    unique_ptr<Node> ParseLeftDereference();

    // Metafunctions
    unique_ptr<Node> ParseTypeof();
    unique_ptr<Node> ParseSizeof();

    // Lambda
    unique_ptr<Node> ParseLambda();

    // Functions
    unique_ptr<Node> ParseNewFunctionType();
    unique_ptr<Node> ParseFuncDecl();

    // Special
    unique_ptr<Node> ParsePostfix(unique_ptr<Node> expr);

    // Arrays
    unique_ptr<Node> ParseNewArrayType();
    unique_ptr<Node> ParseArray();
    unique_ptr<Node> ParseGetIndex(unique_ptr<Node> expr, Token start, Token end);

    // Structs
    unique_ptr<Node> ParseStructDecl();

    Memory GLOBAL_MEMORY;


    ASTGenerator(TokenWalker& walker, string file_name) : walker(walker), file_name(file_name) {}

    unique_ptr<Node> parse_primary_expression() {
        Token current = *walker.get();
        if (walker.CheckType(TokenType::NUMBER))
            return ParseNumber();

        if (walker.CheckType(TokenType::STRING))
            return ParseString();

        if (walker.CheckType(TokenType::CHAR))
            return ParseChar();

        if (walker.CheckValue("(")) {
            Token start = *walker.get();
            auto expr = ParseScopes();
            // Применяем постфиксные операции к скобочному выражению
            return ParsePostfix(std::move(expr));
        }

        if (walker.CheckValue("[")) {
            return ParseNewArrayType();
        }

        if (walker.CheckValue("{")) {
            return ParseArray();
        }

        if (walker.CheckType(TokenType::KEYWORD) && (walker.CheckValue("true") || walker.CheckValue("false")))
            return ParseBool();

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("null"))
            return ParseNull();

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("input"))
            return ParseInput();

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("lambda"))
            return ParseLambda();

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("typeof") && walker.CheckValue("(", 1)) {
            return ParseTypeof();
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("new")) {
            return ParseNew();
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("sizeof") && walker.CheckValue("(", 1)) {
            return ParseSizeof();
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("namespace")) {
            return ParseNamespace();
        }

        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("Func")) {
            return ParseNewFunctionType();
        }

        if (walker.CheckType(TokenType::LITERAL)) {
            auto expr = ParseLiteral();
            // Применяем постфиксные операции к литералу
            return ParsePostfix(std::move(expr));
        }


        return nullptr;
    }

    unique_ptr<Node> parse_unary_expression() {
        if (walker.CheckType(TokenType::OPERATOR) && (walker.CheckValue("-") || walker.CheckValue("+")
        || walker.CheckValue("--") || walker.CheckValue("++") ||
            walker.CheckValue("!") || walker.CheckValue("not"))) {
            string op = walker.get()->value;
            Token operator_token = *walker.get();
            walker.next(); // pass operator
            Token start = *walker.get();
            auto operand = parse_unary_expression();
            Token end = *walker.get(-1);
            return make_unique<NodeUnary>(std::move(operand), start, end, operator_token);
        }
        if (walker.CheckType(TokenType::OPERATOR) && walker.CheckValue("&")) {
            return ParseAddressOf();
        }
        if (walker.CheckType(TokenType::DEREFERENCE) && walker.CheckValue("*")) {
            return ParseDereference();
        }
        auto expr = parse_primary_expression();
        return ParsePostfix(std::move(expr));
    }

    // Вспомогательный метод для парсинга бинарных выражений с заданными операторами
    unique_ptr<Node> ParseBinaryLevel(unique_ptr<Node> (ASTGenerator::*parseHigherLevel)(),
        const vector<string>& operators, string level_name
    ) {
        Token& start_token = *walker.get();
        auto left = (this->*parseHigherLevel)();

        while (true) {
            bool found_operator = false;
            string op;

            for (const auto& candidate : operators) {
                if ((walker.CheckType(TokenType::OPERATOR) || walker.CheckType(TokenType::DEREFERENCE)) && walker.CheckValue(candidate)) {
                    found_operator = true;
                    op = candidate;
                    break;
                }
            }

            if (!found_operator) break;

            Token& op_token = *walker.get();
            walker.next(); // pass operator

            auto right = (this->*parseHigherLevel)();
            if (!right) {
                ERROR::UnexpectedToken(*walker.get(), "expression after " + level_name + " operator");
            }

            Token& end_token = *(walker.get() - 1);
            left = make_unique<NodeBinary>(std::move(left), op, std::move(right),
                                        start_token, end_token, op_token);
        }

        return left;
    }

    unique_ptr<Node> parse_binary_expression_or() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_and,
                            {"or", "||"}, "logical OR");
    }

    unique_ptr<Node> parse_binary_expression_and() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_eq_ne_in_ni,
                            {"and", "&&"}, "logical AND");
    }

    unique_ptr<Node> parse_binary_expression_eq_ne_in_ni() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_less_more,
                            {"==", "!=", "<<", ">>"}, "equality/innary");
    }

    unique_ptr<Node> parse_binary_expression_less_more() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_modul,
                            {"<", ">", "<=", ">="}, "comparison");
    }

    unique_ptr<Node> parse_binary_expression_modul() {
        return ParseBinaryLevel( &ASTGenerator::parse_binary_expression_sum_sub,
                            {"%"}, "modulus");
    }

    unique_ptr<Node> parse_binary_expression_sum_sub() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_mul_div,
                            {"+", "-"}, "addition/subtraction");
    }

    unique_ptr<Node> parse_binary_expression_mul_div() {
        return ParseBinaryLevel( &ASTGenerator::parse_binary_expression_exp,
                            {"*", "/"}, "multiplication/division");
    }

    unique_ptr<Node> parse_binary_expression_exp() {
        return ParseBinaryLevel(&ASTGenerator::parse_unary_expression,
                            {"**", "|"}, "exponentiation/bitwise OR");
    }

    unique_ptr<Node> parse_higher_order_expressions() {
        if (walker.CheckType(TokenType::KEYWORD) && walker.CheckValue("if")) {
            return ParseIfExpr();
        }

        Token start = *walker.get();
        auto expr = parse_binary_expression_or();
        if (walker.CheckValue("<-")) {
            Token op_token = *walker.get();
            walker.next();
            auto value_expr = parse_expression();
            Token end = *(walker.get() - 1);
            expr = make_unique<NodeArrayPush>(std::move(expr), std::move(value_expr), op_token, end);
        }

        return expr;
    }

    unique_ptr<Node> parse_expression() {
        return parse_higher_order_expressions();
    }

    unique_ptr<Node> parse_statement() {
        Token current = *walker.get();
        if (current.type == TokenType::L_CURVE_BRACKET) {
            return ParseBlock();
        }
        if (current.type == TokenType::KEYWORD && current.value == "out") {
            return ParseOut();
        }
        if (current.type == TokenType::KEYWORD && current.value == "outln") {
            return ParseOutLn();
        }
        if (current.type == TokenType::KEYWORD && current.value == "assert") {
            return ParseAssert();
        }

        if (current.type == TokenType::KEYWORD && current.value == "func") {
            return ParseFuncDecl();
        }

        if (current.type == TokenType::KEYWORD && current.value == "struct") {
            return ParseStructDecl();
        } 
        // block declaration
        if ((current.type == TokenType::KEYWORD) &&
            (current.value == "final" || current.value == "const" ||
            current.value == "static" || current.value == "global" || current.value == "private")) {
            string modifier = current.value;
            walker.next();
            if (walker.get()->type == TokenType::L_CURVE_BRACKET) {
                return ParseBlockDecl(modifier);
            }
            walker.before();
            // Если не блочная декларация, продолжаем как обычно
        }
        if (current.type == TokenType::KEYWORD && current.value == "let") {
            return ParseBaseVariableDecl();
        }
        if (current.type == TokenType::KEYWORD && current.value == "final") {
            return ParseFinalVariableDecl();
        }
        if (current.type == TokenType::KEYWORD && current.value == "static") {
            return ParseStaticVariableDecl();
        }
        if (current.type == TokenType::KEYWORD && current.value == "const") {
            return ParseConstVariableDecl();
        }
        if (current.type == TokenType::KEYWORD && current.value == "global") {
            return ParseGlobalVariableDecl();
        }
        if (current.type == TokenType::KEYWORD && current.value == "private") {
            return ParsePrivateVariableDecl();
        }

        if (current.type == TokenType::KEYWORD && current.value == "exit") {
            return ParseExit();
        }

        if (current.type == TokenType::KEYWORD && current.value == "ret") {
            return ParseReturn();
        }

        if (current.type == TokenType::KEYWORD && current.value == "namespace") {
            return ParseNameSpaceDecl();
        }
        if (current.type == TokenType::KEYWORD && current.value == "if") {
            return ParseIf();
        }
        if (current.type == TokenType::KEYWORD && current.value == "del") {
            return ParseDelete();
        }
        if (current.type == TokenType::KEYWORD && current.value == "while") {
            return ParseWhile();
        }
        if (current.type == TokenType::KEYWORD && current.value == "do") {
            return ParseDoWhile();
        }
        if (current.type == TokenType::KEYWORD && current.value == "for") {
            return ParseFor();
        }
        if (current.type == TokenType::KEYWORD && current.value == "break") {
            return ParseBreak();
        }
        if (current.type == TokenType::KEYWORD && current.value == "continue") {
            return ParseContinue();
        }

        // Обработка выражения, которое может начинаться с '*' (разыменование)
        // Проверяем, является ли это присваиванием через разыменование
        if (current.value == "*") {
            TokenWalker snapshot = walker; // Сохраняем состояние для отката
            walker.next(); // Пропускаем '*'

            // Парсим выражение, которое разыменовывается
            auto expr = parse_expression();

            // Проверяем, что идет дальше
            if (walker.CheckValue("=")) {
                // Это присваивание через разыменование (*expr = ...)
                // Возвращаемся и вызываем ParseLeftDereference
                walker = snapshot;
                return ParseLeftDereference();
            } else {
                // Это не присваивание, возможно выражение с <- или просто выражение
                // Возвращаемся и парсим как обычное выражение
                walker = snapshot;
            }
        }

        // Общий случай: выражение, которое может быть присваиванием или expression statement
        auto start_left_value_token = *walker.get();
        auto left_expr = parse_expression();

        if (!left_expr) {
            ERROR::UnexpectedToken(*walker.get(), "expression");
        }

        auto end_left_value_token = *walker.get(-1);

        // Проверяем оператор присваивания
        if (walker.CheckValue("=")) {
            // Обычное присваивание
            walker.next();
            auto start_value_token = *walker.get();
            auto expr = parse_expression();

            if (!expr)
                ERROR::UnexpectedToken(*walker.get(), "expression");

            if (!walker.CheckValue(";"))
                ERROR::UnexpectedToken(*walker.get(), "';'");
            auto end_value_token = *walker.get();
            walker.next();

            return make_unique<NodeVariableEqual>(std::move(left_expr), std::move(expr),
                                                start_left_value_token, end_left_value_token,
                                                start_value_token, end_value_token);
        } else if (walker.CheckValue("<-")) {
            // Оператор push в массив
            Token op_token = *walker.get();
            walker.next();
            auto start_value_token = *walker.get();
            auto expr = parse_expression();

            if (!expr)
                ERROR::UnexpectedToken(*walker.get(), "expression");

            if (!walker.CheckValue(";"))
                ERROR::UnexpectedToken(*walker.get(), "';'");
            auto end_value_token = *walker.get();
            walker.next();

            // Создаем NodeArrayPush вместо NodeVariableEqual
            return make_unique<NodeArrayPush>(std::move(left_expr), std::move(expr),
                                            op_token, end_value_token);
        } else {
            // Просто expression statement
            if (!walker.CheckValue(";"))
                ERROR::UnexpectedToken(*walker.get(), "';'");
            walker.next();
            return make_unique<NodeExpressionStatement>(std::move(left_expr));
        }
    }

    void GenerateStandartTypes(Memory& _memory) {
        auto OBJ_TYPE_INT = CreateMemoryObject(NewType("Int"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Int",OBJ_TYPE_INT);
        STATIC_MEMORY.register_object(OBJ_TYPE_INT);

        auto OBJ_TYPE_DOUBLE = CreateMemoryObject(NewType("Double"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Double",OBJ_TYPE_DOUBLE);
        STATIC_MEMORY.register_object(OBJ_TYPE_DOUBLE);

        auto OBJ_TYPE_CHAR = CreateMemoryObject(NewType("Char"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Char",OBJ_TYPE_CHAR);
        STATIC_MEMORY.register_object(OBJ_TYPE_CHAR);

        auto OBJ_TYPE_STRING = CreateMemoryObject(NewType("String"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("String",OBJ_TYPE_STRING);
        STATIC_MEMORY.register_object(OBJ_TYPE_STRING);

        auto OBJ_TYPE_BOOL = CreateMemoryObject(NewType("Bool"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Bool",OBJ_TYPE_BOOL);
        STATIC_MEMORY.register_object(OBJ_TYPE_BOOL);

        auto OBJ_TYPE_TYPE = CreateMemoryObject(NewType("Type"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Type",OBJ_TYPE_TYPE);
        STATIC_MEMORY.register_object(OBJ_TYPE_TYPE);

        auto OBJ_TYPE_NULL = CreateMemoryObject(NewType("Null"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Null",OBJ_TYPE_NULL);
        STATIC_MEMORY.register_object(OBJ_TYPE_NULL);


        auto OBJ_TYPE_NAMESPACE = CreateMemoryObject(NewType("Namespace"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Namespace",OBJ_TYPE_NAMESPACE);
        STATIC_MEMORY.register_object(OBJ_TYPE_NAMESPACE);

        auto OBJ_TYPE_LAMBDA = CreateMemoryObject(NewType("Lambda"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("Lambda",OBJ_TYPE_LAMBDA);
        STATIC_MEMORY.register_object(OBJ_TYPE_LAMBDA);

        auto OBJ_TYPE_AUTO = CreateMemoryObject(NewType("auto"), STANDART_TYPE::TYPE, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("auto",OBJ_TYPE_AUTO);
        STATIC_MEMORY.register_object(OBJ_TYPE_AUTO);

        auto __TWIST_FILE__ = CreateMemoryObject(Value(STANDART_TYPE::STRING, string(file_name)), STANDART_TYPE::STRING, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("__FILE__", __TWIST_FILE__);
        STATIC_MEMORY.register_object(__TWIST_FILE__);

        auto __TWIST_ADDR__ = CreateMemoryObject(NewPointer(AddressManager::get_current_address() + 2, STANDART_TYPE::NULL_T), STANDART_TYPE::STRING, &GLOBAL_MEMORY,
            true, true, true, true, false);
        GLOBAL_MEMORY.add_object("__PTR__", __TWIST_ADDR__);
        STATIC_MEMORY.register_object(__TWIST_ADDR__);
        // STATIC_MEMORY.register_object(__TWIST_FILE__);

        // STATIC_MEMORY.debug_print();
    }

    inline void parse() {
        GenerateStandartTypes(GLOBAL_MEMORY);

        while (!walker.isEnd()) {
            auto stmt = parse_statement();
            if (stmt) {
                this->nodes.push_back(std::move(stmt));
            }
        }
    }

    // В ASTGenerator
    Context run() {
        parse();

        // Создаем копию GLOBAL_MEMORY
        Memory memory_copy;

        // Копируем все переменные
        for (const auto& [name, obj] : GLOBAL_MEMORY.string_pool) {
            memory_copy.copy_object(name, obj->value, obj->wait_type,
                                    obj->modifiers.is_const, obj->modifiers.is_static,
                                    obj->modifiers.is_final, obj->modifiers.is_global, obj->address);
        }

        return Context(std::move(nodes), std::move(memory_copy));
    }
};


struct ContextExecutor {
    Context context;

    ContextExecutor(Context context) : context(std::move(context)) {}
    void run() {
        Memory dump = context.memory;
        for (int i = 0; i < (context.nodes).size(); i++) {
            (context.nodes)[i]->exec_from(context.memory);
        }
        context.memory = dump;
    }
};

unique_ptr<Node> ASTGenerator::ParseReturn() {
    walker.next();
    Token start = *walker.get();
    auto expr = parse_expression();
    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    Token end = *walker.get(-1);
    walker.next();
    return make_unique<NodeReturn>(std::move(expr), start, end);
}


unique_ptr<Node> ASTGenerator::ParseExit() {
    Token start = *walker.get();
    walker.next();
    auto expr = parse_expression();
    Token end = *(walker.get());
    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();
    return make_unique<NodeExit>(std::move(expr), start, end);
}


unique_ptr<Node> ASTGenerator::ParsePostfix(unique_ptr<Node> expr) {
    while (true) {
        if (walker.CheckValue("(")) {
            Token start = *walker.get();
            Token end;
            expr = ParseCall(std::move(expr), start, end);
        } else if (walker.CheckValue("[")) {
            Token start = *walker.get();
            Token end;
            expr = ParseGetIndex(std::move(expr), start, end);
        } else if (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
            expr = ParseNameResolution(std::move(expr));
        } else if (walker.CheckValue(".")) {
            expr = ParseObjectResolution(std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}


unique_ptr<Node> ASTGenerator::ParseFuncDecl() {
    walker.next();

    if (!walker.CheckType(TokenType::LITERAL))
        ERROR::UnexpectedToken(*walker.get(), "function name");
    string name = walker.get()->value;
    walker.next();

    // parse arguments
    Token start_args_token = *walker.get();
    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();



    vector<Arg*> arguments;
  
    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }

        bool arg_is_const = false;
        bool arg_is_static = false;
        bool arg_is_final = false;
        bool arg_is_global = false;
        bool is_variadic = false;
        unique_ptr<Node> variadic_size_expr = nullptr;

        while (true) {
            if (walker.CheckValue("const")) {
                arg_is_const = true;
                walker.next();
                continue;
            }
            if (walker.CheckValue("static")) {
                arg_is_static = true;
                walker.next();
                continue;
            }
            if (walker.CheckValue("final")) {
                arg_is_final = true;
                walker.next();
                continue;
            }
            if (walker.CheckValue("global")) {
                arg_is_global = true;
                walker.next();
                continue;
            }
            break;
        }



        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "variable name");
        string arg_name = walker.get()->value;
        walker.next();

        unique_ptr<Node> type_expr = nullptr;
        unique_ptr<Node> default_expr = nullptr;

        // Вариадик параметр
        if (walker.CheckValue("[")) {
            walker.next();
            is_variadic = true;
            // Если сразу идёт ']' — динамический размер
            if (walker.CheckValue("]")) {
                variadic_size_expr = nullptr;   // маркер динамического размера
            } else {
                variadic_size_expr = parse_expression();
                if (!variadic_size_expr)
                    ERROR::UnexpectedToken(*walker.get(), "variadic size expression");
                if (!walker.CheckValue("]"))
                    ERROR::UnexpectedToken(*walker.get(), "']'");
            }
            walker.next();
        }


        if (!walker.CheckValue(":"))
            ERROR::UnexpectedToken(*walker.get(), "':'");



        walker.next();
        type_expr = parse_expression();
        if (!type_expr)
            ERROR::UnexpectedToken(*walker.get(), "type expression");


        if (walker.CheckValue("=")) {
            walker.next();
            default_expr = parse_expression();
            if (!default_expr)
                ERROR::UnexpectedToken(*walker.get(), "default value expression");
        }

        auto arg = new Arg(arg_name);
        arg->type_expr = std::move(type_expr);
        arg->default_parameter = std::move(default_expr);
        arg->is_const = arg_is_const;
        arg->is_static = arg_is_static;
        arg->is_final = arg_is_final;
        arg->is_global = arg_is_global;
        arg->is_variadic = is_variadic;
        arg->variadic_size = std::move(variadic_size_expr);

        arguments.push_back(arg);

        if (!walker.CheckValue(",") && !walker.CheckValue(")")) {
            ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
        }
        if (walker.CheckValue(",")) walker.next();
    }

    Token end_args_token = *walker.get(-1);
    Token start_return_token = *walker.get();
    unique_ptr<Node> return_type_expr = nullptr;
    if (walker.CheckValue("->")) {
        walker.next();
        start_return_token = *walker.get();
        return_type_expr = parse_expression();
        if (!return_type_expr)
            ERROR::UnexpectedToken(*walker.get(), "return type expression");
    }
    Token end_return_token = *walker.get(-1);


    auto statement = parse_statement();




    return make_unique<NodeFunctionDeclaration>(name, std::move(arguments), std::move(return_type_expr), std::move(statement), start_args_token, end_args_token, start_return_token, end_return_token);


}


unique_ptr<Node> ASTGenerator::ParseNewFunctionType() {
    walker.next(); // pass "Func" token

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");

    Token start_token = *walker.get();
    walker.next();

    // Обработка пустого списка аргументов
    if (walker.CheckValue(")")) {
        Token end_token = *walker.get();
        walker.next(); // pass ")" token

        // Теперь проверяем, есть ли возвращаемый тип
        unique_ptr<Node> return_type_expr = nullptr;
        Token start_token_return;
        Token end_token_return;

        if (walker.CheckValue("->")) {
            walker.next(); // pass "->" token
            start_token_return = *walker.get();
            return_type_expr = parse_expression();
            end_token_return = *walker.get(-1);
            if (!return_type_expr)
                ERROR::UnexpectedToken(*walker.get(), "expression");

            return make_unique<NodeNewFuncType>(
                vector<unique_ptr<Node>>(),
                std::move(return_type_expr),
                start_token, end_token,
                start_token_return, end_token_return
            );
        } else {
            return make_unique<NodeNewFuncType>(
                vector<unique_ptr<Node>>(),
                nullptr,
                start_token, end_token,
                Token(), Token()
            );
        }
    }

    vector<unique_ptr<Node>> type_args;
    while (true) {
        auto type_expr = parse_expression();

        if (!type_expr)
            ERROR::UnexpectedToken(*walker.get(), "expression");

        type_args.push_back(std::move(type_expr));

        if (walker.CheckValue(")") || walker.CheckValue(",")) {
            if (walker.CheckValue(",")) {
                walker.next();
                continue;
            }
            else if (walker.CheckValue(")")) break;
        }
        ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
    }

    Token end_token = *walker.get();
    walker.next(); // pass ")" token

    unique_ptr<Node> return_type_expr = nullptr;
    Token start_token_return;
    Token end_token_return;

    if (walker.CheckValue("->")) {
        walker.next(); // pass "->" token
        start_token_return = *walker.get();
        return_type_expr = parse_expression();
        end_token_return = *walker.get(-1);
        if (!return_type_expr)
            ERROR::UnexpectedToken(*walker.get(), "expression");
    }

    return make_unique<NodeNewFuncType>(
        std::move(type_args),
        std::move(return_type_expr),
        start_token, end_token,
        start_token_return, end_token_return
    );
}


unique_ptr<Node> ASTGenerator::ParseNew() {
    walker.next(); // pass 'new' token
    bool is_static = false;
    bool is_const = false;
    unique_ptr<Node> type_expr = nullptr;

    Token start_type = *walker.get();
    Token end_type = *walker.get();

    if (walker.CheckValue("<")) {
        walker.next();
        while (!walker.CheckValue(">")) {
            auto m = walker.get()->value;
            if (m == "const") {is_const = true; walker.next();}
            else if (m == "static") {
                is_static = true;
                walker.next();
                if (walker.CheckValue("(")) {
                    walker.next();
                    start_type = *walker.get();
                    type_expr = parse_expression();
                    if (!type_expr)
                        ERROR::UnexpectedToken(*walker.get(), "type expression");
                    end_type = *walker.get(-1);
                    if (!walker.CheckValue(")"))
                        ERROR::UnexpectedToken(*walker.get(), "')'");
                    walker.next();
                }
            }
            else ERROR::UnexpectedToken(*walker.get(), "modifiers ('const', 'static')");

            if (walker.CheckValue(",") || walker.CheckValue(">")) {
                if (walker.CheckValue(",")) {
                    walker.next();
                    continue;
                }
                else if (walker.CheckValue(">")) break;
            }
            ERROR::UnexpectedToken(*walker.get(), "',' or '>'");
        }
        walker.next(); // pass ")" token
    }

    auto expr = parse_expression();

    return make_unique<NodeNew>(std::move(expr), std::move(type_expr), is_static, is_const, start_type, end_type);
}


unique_ptr<Node> ASTGenerator::ParseCall(unique_ptr<Node> expr, Token start, Token end) {
    walker.next(); // pass '(' token

    vector<unique_ptr<Node>> arguments;
    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }
        auto arg = parse_expression();
        if (!arg)
            ERROR::UnexpectedToken(*walker.get(), "argument expression");
        arguments.push_back(std::move(arg));

        if (!walker.CheckValue(")") && !walker.CheckValue(",")) {
            ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
        }

        if (walker.CheckValue(",")) walker.next();
    }
    end = *walker.get(-1);
    return make_unique<NodeCall>(std::move(expr), std::move(arguments), start, end);
}


unique_ptr<Node> ASTGenerator::ParseLambda() {
    walker.next(); // pass 'lambda' token
    Token start_args_token = *walker.get();


    string name = "";
    if (walker.CheckValue("[")) {
        walker.next();
        if (walker.CheckType(TokenType::LITERAL)) {
            name = walker.get()->value;
            walker.next();
        } else {
            ERROR::UnexpectedToken(*walker.get(), "literal (not `String` or `Char`)");
        }
        if (!walker.CheckValue("]")) ERROR::UnexpectedToken(*walker.get(), "']'");
        walker.next();
    }

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    vector<Arg*> arguments;

    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }
        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "variable name");
        string arg_name = walker.get()->value;
        walker.next();

        unique_ptr<Node> type_expr = nullptr;
        unique_ptr<Node> default_expr = nullptr;

        if (walker.CheckValue(":")) {
            walker.next();
            type_expr = parse_expression();
            if (!type_expr)
                ERROR::UnexpectedToken(*walker.get(), "type expression");
        }

        if (walker.CheckValue("=")) {
            walker.next();
            default_expr = parse_expression();
            if (!default_expr)
                ERROR::UnexpectedToken(*walker.get(), "default value expression");
        }

        auto arg = new Arg(arg_name);
        arg->type_expr = std::move(type_expr);
        arg->default_parameter = std::move(default_expr);
        arguments.push_back(arg);

        if (!walker.CheckValue(",") && !walker.CheckValue(")")) {
            ERROR::UnexpectedToken(*walker.get(), "',' or ')'");
        }
        if (walker.CheckValue(",")) walker.next();
    }

    Token end_args_token = *walker.get(-1);

    Token type_start_token;
    Token type_end_token;

    unique_ptr<Node> return_type = nullptr;
    if (walker.CheckValue("->")) {
        walker.next();
        type_start_token = *walker.get();
        return_type = parse_expression();
        type_end_token = *walker.get(-1);
        if (!return_type)
            ERROR::UnexpectedToken(*walker.get(), "return type expression");
    }

    if (!walker.CheckValue(":"))
        ERROR::UnexpectedToken(*walker.get(), "':'");
    walker.next();



    auto lambda_node = make_unique<NodeLambda>(nullptr, arguments, std::move(return_type),
        start_args_token, end_args_token, type_start_token, type_end_token);
    lambda_node->name = name;

    auto block = parse_expression();

    lambda_node->body = std::move(block);

    return lambda_node;
}


unique_ptr<Node> ASTGenerator::ParseAssert() {
    walker.next(); // pass 'assert' token
    Token start_token = *walker.get();
    auto expr = parse_expression();
    Token end_token = *walker.get(-1);
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();
    return make_unique<NodeAssert>(std::move(expr), start_token, end_token);
}


unique_ptr<Node> ASTGenerator::ParseNamespace() {
    walker.next(); // pass "namespace" token
    auto namespace_node = make_unique<NodeNamespace>(nullptr);

    auto block = parse_statement();
    namespace_node->statement = std::move(block);

    return namespace_node;
}


unique_ptr<Node> ASTGenerator::ParseInput() {
    auto start_token = *walker.get();
    walker.next(); // pass 'input' token
    auto end_token = *walker.get(-1);
    unique_ptr<Node> expr = nullptr;
    if (walker.CheckValue("(")) {
        walker.next();
        auto start_token = *walker.get();
        expr = parse_expression();
        if (!walker.CheckValue(")"))
            ERROR::UnexpectedToken(*walker.get(), "')'");
        auto end_token = *walker.get();
        walker.next();
    }
    return make_unique<NodeInput>(std::move(expr), start_token, end_token);
}


unique_ptr<Node> ASTGenerator::ParseLeftDereference() {
    walker.next(); // pass '*' token

    auto start_left_value_token = *walker.get(-1);
    auto left_expr = parse_expression();

    if (!left_expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");


    auto end_left_value_token = *walker.get(-1);

    if (!walker.CheckValue("="))
        ERROR::UnexpectedToken(*walker.get(), "'='");
    walker.next();
    auto start_value_token = *walker.get();
    auto expr = parse_expression();

    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");


    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    auto end_value_token = *walker.get();
    walker.next();

    return make_unique<NodeLeftDereference>(std::move(left_expr), std::move(expr), start_left_value_token, end_left_value_token, start_value_token, end_value_token);
}


unique_ptr<Node> ASTGenerator::ParseTypeof() {
    walker.next(); // pass 'typeof' token
    walker.next(); // pass '(' token
    auto expr = parse_expression();
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')' after typeof");
    walker.next();
    return make_unique<NodeTypeof>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParseSizeof() {
    walker.next(); // pass 'sizeof' token
    walker.next(); // pass '(' token
    auto expr = parse_expression();
    if (!expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");
    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')' after sizeof");
    walker.next();
    return make_unique<NodeSizeof>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParseAddressOf() {
    walker.next(); // pass '&' token
    auto expr = parse_primary_expression();
    return make_unique<NodeAddressOf>(std::move(expr));
}


unique_ptr<Node> ASTGenerator::ParseDereference() {
    walker.next(); // pass '*' token
    if (walker.CheckValue("(")) {
        Token start_token = *walker.get();
        walker.next();
        auto expr = parse_expression();
        if (!expr) ERROR::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckValue(")")) ERROR::UnexpectedToken(*walker.get(), "')'");
        Token end_token = *walker.get();
        walker.next();
        return make_unique<NodeDereference>(std::move(expr), start_token, end_token);
    } else {
        Token start_token = *walker.get();
        auto expr = parse_expression();
        if (!expr) ERROR::UnexpectedToken(*walker.get(), "primary expression or (expression)");
        Token end_token = *walker.get(-1);
        return make_unique<NodeDereference>(std::move(expr), start_token, end_token);
    }
}



unique_ptr<Node> ASTGenerator::ParseBlockDecl(string modifier) {
    if (!walker.CheckValue("{")) {
        ERROR::UnexpectedToken(*walker.get(), "'{' after block declaration");
    }
    walker.next();

    // Создаем блок для хранения всех объявлений
    vector<unique_ptr<Node>> declarations;

    // Парсим все объявления внутри блока
    while (!walker.CheckValue("}")) {
        auto stmt = parse_statement();
        if (!stmt) {
            ERROR::UnexpectedToken(*walker.get(), "statement");
        }

        declarations.push_back(std::move(stmt));
    }

    if (!walker.CheckValue("}"))
        ERROR::UnexpectedToken(*walker.get(), "'}'");
    walker.next();

    auto node = make_unique<NodeBlockDecl>(std::move(declarations));
    if (modifier == "static")
        node->is_static = true;

    if (modifier == "const")
        node->is_const = true;

    if (modifier == "global")
        node->is_global = true;

    if (modifier == "final")
        node->is_final = true;

    if (modifier == "private")
        node->is_private = true;

    // Возвращаем блок с примененными модификаторами
    return node;
}


unique_ptr<Node> ASTGenerator::ParseContinue() {
    walker.next(); // pass 'break'

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    return make_unique<NodeContinue>();
}


unique_ptr<Node> ASTGenerator::ParseBreak() {
    walker.next(); // pass 'break'

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    return make_unique<NodeBreak>();
}


unique_ptr<Node> ASTGenerator::ParseFor() {
    walker.next(); // pass 'for' token

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto init_state = parse_statement();

    auto check_expr = parse_expression();

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    auto update_state = parse_statement();
    if (!update_state)
        ERROR::UnexpectedToken(*walker.get(), "update statement");


    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    auto body = parse_statement();

    return make_unique<NodeFor>(std::move(init_state), std::move(check_expr), std::move(update_state), std::move(body));
}


unique_ptr<Node> ASTGenerator::ParseWhile() {
    walker.next(); // pass 'while' token

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto expr = parse_expression();

    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    auto state = parse_statement();

    return make_unique<NodeWhile>(std::move(expr), std::move(state));
}


unique_ptr<Node> ASTGenerator::ParseDoWhile() {
    walker.next(); // pass 'do' token

    auto state = parse_statement();

    if (!walker.CheckValue("while"))
        ERROR::UnexpectedToken(*walker.get(), "'while'");
    walker.next();

    if (!walker.CheckValue("("))
        ERROR::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto expr = parse_expression();

    if (!walker.CheckValue(")"))
        ERROR::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    return make_unique<NodeDoWhile>(std::move(expr), std::move(state));
}


unique_ptr<Node> ASTGenerator::ParseDelete() {
    walker.next(); // pass 'del' token

    Token start_token = *walker.get();

    // Парсим выражение, которое нужно удалить
    auto target_expr = parse_expression();

    Token end_token = *walker.get(-1);

    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");

    walker.next(); // pass ';'

    return make_unique<NodeDelete>(std::move(target_expr), start_token, end_token);
}

unique_ptr<Node> ASTGenerator::ParseObjectResolution(unique_ptr<Node> expression) {
    vector<string> chain;
    
    // Собираем всю цепочку имен
    while (walker.CheckValue(".")) {
        walker.next(); // pass first '.'

        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "literal");

        string literal_name = walker.get()->value;
        chain.push_back(literal_name);
        walker.next();
    }
    Token start = *walker.get(-1);
    // Если есть цепочка, создаем ноду разрешения
    if (!chain.empty()) {
        // Первый элемент цепочки становится первым уровнем
        string first_name = chain[0];
        vector<string> remaining_chain(chain.begin() + 1, chain.end());
        Token end = *walker.get(-1);
        expression = make_unique<NodeObjectResolution>(std::move(expression), first_name, start, end, remaining_chain);
    }

    return expression;
}


unique_ptr<Node> ASTGenerator::ParseNameResolution(unique_ptr<Node> expression) {
    vector<string> chain;
    Token start = *walker.get(-1);
    // Собираем всю цепочку имен
    while (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
        walker.next(); // pass first ':'
        walker.next(); // pass second ':'

        if (!walker.CheckType(TokenType::LITERAL))
            ERROR::UnexpectedToken(*walker.get(), "literal");

        string literal_name = walker.get()->value;
        chain.push_back(literal_name);
        walker.next();
    }

    // Если есть цепочка, создаем ноду разрешения
    if (!chain.empty()) {
        // Первый элемент цепочки становится первым уровнем
        string first_name = chain[0];
        vector<string> remaining_chain(chain.begin() + 1, chain.end());
        Token end = *walker.get(-1);
        expression = make_unique<NodeNamespaceResolution>(std::move(expression), first_name, start, end, remaining_chain);
    }

    return expression;
}


unique_ptr<Node> ASTGenerator::ParseNameSpaceDecl() {
    walker.next(); // pass 'namespace' token

    if (!walker.CheckType(TokenType::LITERAL))
        ERROR::UnexpectedToken(*walker.get(), "namespace identifier");
    Token decl_token = *walker.get();
    string namespace_name = walker.get()->value;
    walker.next();

    // Создаем namespace
    auto namespace_node = make_unique<NodeNamespaceDeclaration>(nullptr, namespace_name, decl_token);

    auto block = parse_statement();
    namespace_node->statement = std::move(block);

    return namespace_node;
}


unique_ptr<Node> ASTGenerator::ParseIfExpr() {
    walker.next(); // pass 'if' token

    if (!walker.CheckType(TokenType::L_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "(");
    walker.next();

    auto eq_expr = parse_expression();
    if (!eq_expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), ")");
    walker.next();

    if (!walker.CheckType(TokenType::L_CURVE_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "{");
    walker.next();

    auto true_expr = parse_expression();
    if (!true_expr) ERROR::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "}");
    walker.next();

    unique_ptr<Node> else_expr = nullptr;
    if (walker.CheckValue("else")) {
        walker.next();
        if (!walker.CheckType(TokenType::L_CURVE_BRACKET))
            ERROR::UnexpectedToken(*walker.get(), "{");
        walker.next();
        else_expr = parse_expression();
        if (!else_expr) ERROR::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
            ERROR::UnexpectedToken(*walker.get(), "}");
        walker.next();
    }

    return make_unique<NodeIfExpr>(std::move(eq_expr), std::move(true_expr), std::move(else_expr));
}


unique_ptr<Node> ASTGenerator::ParseIf() {
    walker.next(); // pass 'if' token

    if (!walker.CheckType(TokenType::L_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), "(");
    walker.next();

    auto eq_expr = parse_expression();
    if (!eq_expr)
        ERROR::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_BRACKET))
        ERROR::UnexpectedToken(*walker.get(), ")");
    walker.next();

    auto true_state = parse_statement();
    unique_ptr<Node> else_state;

    if (walker.CheckValue("else")) {
        walker.next();
        else_state = parse_statement();
        if (!else_state)
            ERROR::UnexpectedToken(*walker.get(), "statement or statement block");
    }

    return make_unique<NodeIf>(std::move(eq_expr), std::move(true_state), std::move(else_state));
}


unique_ptr<Node> ASTGenerator::ParseBlock() {
    walker.next();
    vector<unique_ptr<Node>> nodes_array;
    while (!walker.CheckType(TokenType::R_CURVE_BRACKET)) {
        nodes_array.push_back(parse_statement());
    }
    walker.next();
    return make_unique<NodeBlock>(nodes_array);
}


unique_ptr<Node> ASTGenerator::ParseLiteral() {
    auto node = make_unique<NodeLiteral>(*walker.get());
    walker.next();
    return node;
}


unique_ptr<Node> ASTGenerator::ParseNumber() {
    auto node = make_unique<NodeNumber>(*walker.get());
    walker.next();
    return node;
}


unique_ptr<Node> ASTGenerator::ParseString() {
    auto node = make_unique<NodeString>(walker.get()->value);
    walker.next();
    return node;
}


unique_ptr<Node> ASTGenerator::ParseChar() {
    auto node = make_unique<NodeChar>(walker.get()->value[0]);
    walker.next();
    return node;
}


unique_ptr<Node> ASTGenerator::ParseBool() {
    auto node = make_unique<NodeBool>(*walker.get());
    walker.next();
    return node;
}


unique_ptr<Node> ASTGenerator::ParseNull() {
    auto node = make_unique<NodeNull>();
    walker.next();
    return node;
}


unique_ptr<Node> ASTGenerator::ParseScopes() {
    walker.next(); // pass '(' token
    auto expr = parse_expression();
    if (!expr) {
        ERROR::UnexpectedToken(*walker.get(), "expression");
    }
    if (!walker.CheckValue(")")) {
        ERROR::UnexpectedToken(*walker.get(), "')'");
    }
    walker.next(); // pass ')' token
    return make_unique<NodeScopes>(std::move(expr));
}



unique_ptr<Node> ASTGenerator::ParseBaseVariableDecl() {
    walker.next(); // pass 'let' token

   
    if (!walker.CheckType(TokenType::LITERAL)) {
        ERROR::UnexpectedToken(*walker.get(), "variable name");
    }

    
    Token variable_token = *walker.get();
    string var_name = walker.get()->value;
    walker.next(); // pass variable name token

    unique_ptr<Node> type_expr = nullptr; // type expression
    Token type_start_token;
    Token type_end_token;



    if (walker.CheckValue(":")) {
        walker.next(); // pass ':' token
        type_start_token = *walker.get();
        type_expr = parse_expression();
        type_end_token = *walker.get(-1);
    } else {
        type_start_token = *walker.get();
        type_end_token = *walker.get();
    }


    bool nullable = false;
    if (walker.CheckValue("?")) {
        nullable = true;
        walker.next(); // pass '?' token
    }

    Token start_expr_token = *walker.get();
    Token end_expr_token = *walker.get();


    if (walker.CheckValue(";")) {
        auto expr = make_unique<NodeNull>();
        walker.next();
        return make_unique<NodeVariableDeclaration>(var_name, std::move(expr), variable_token,
                                             std::move(type_expr), type_start_token, type_end_token, nullable, start_expr_token, end_expr_token);
    }

    if (!walker.CheckValue("="))
        ERROR::UnexpectedToken(*walker.get(), "'='");
    walker.next(); // pass '=' token

    start_expr_token = *walker.get();
    auto expr = parse_expression();
    end_expr_token = *walker.get(-1);


    if (!walker.CheckValue(";"))
        ERROR::UnexpectedToken(*walker.get(), "';'");
    walker.next(); // pass ';' token

    return make_unique<NodeVariableDeclaration>(var_name, std::move(expr), variable_token,
                                             std::move(type_expr), type_start_token, type_end_token, nullable, start_expr_token, end_expr_token);
}


unique_ptr<Node> ASTGenerator::ParseFinalVariableDecl() {
    walker.next(); // pass 'final' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }


    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl.get())->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl.get())->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl.get())->is_final = true;
        } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeFunctionDeclaration*)decl.get())->is_final = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }

    return decl;
}


unique_ptr<Node> ASTGenerator::ParseStaticVariableDecl() {
    walker.next(); // pass 'static' token

    if (!walker.CheckValue("final") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl.get())->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl.get())->is_static = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }

    return decl;
}


unique_ptr<Node> ASTGenerator::ParseConstVariableDecl() {
    walker.next(); // pass 'const' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("final") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl.get())->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl.get())->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl.get())->is_const = true;
        } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl.get())->is_const = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }
    
    return decl;
}


unique_ptr<Node> ASTGenerator::ParseGlobalVariableDecl() {
    walker.next(); // pass 'const' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("final") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl.get())->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl.get())->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl.get())->is_global = true;
        } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl.get())->is_global = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }
    return decl;
}


unique_ptr<Node> ASTGenerator::ParsePrivateVariableDecl() {
    walker.next(); // pass 'private' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("final") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("global") && !walker.CheckValue("struct")) {
        ERROR::UnexpectedStatement(*walker.get(), "variable declaration");
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl.get())->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl.get())->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl.get())->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl.get())->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl.get())->is_private = true;
    } else {
        ERROR::UnexpectedStatement(token, "variable declaration");
    }
    return decl;
}


unique_ptr<Node> ASTGenerator::ParseOut() {
    walker.next(); // pass 'out'
    vector<unique_ptr<Node>> args;
    args.push_back(parse_expression());
    while (true) {
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            auto expr = parse_expression();
            if (!expr) {
                ERROR::UnexpectedToken(*walker.get(), "expression");
            }
            args.push_back(std::move(expr));
            continue;
        }
        break;
    }

    if (!walker.CheckValue(";")) {
        ERROR::UnexpectedToken(*walker.get(), "';'");
    }
    walker.next();
    return make_unique<NodeBaseOut>(std::move(args));
}


unique_ptr<Node> ASTGenerator::ParseOutLn() {
    walker.next(); // pass 'out'
    vector<unique_ptr<Node>> args;
    args.push_back(parse_expression());
    while (true) {
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            auto expr = parse_expression();
            if (!expr) {
                ERROR::UnexpectedToken(*walker.get(), "expression");
            }
            args.push_back(std::move(expr));
            continue;
        }
        break;
    }

    if (!walker.CheckValue(";")) {
        ERROR::UnexpectedToken(*walker.get(), "';'");
    }
    walker.next();
    return make_unique<NodeBaseOutLn>(std::move(args));
}


unique_ptr<Node> ASTGenerator::ParseNewArrayType() {
    Token start = *walker.get();
    walker.next(); // pass "["
    auto type_expr = parse_expression();
    unique_ptr<Node> size_expr = nullptr;

    if (walker.CheckValue(",")) {
        walker.next(); // pass ',' token
        size_expr = parse_expression();
        if (!size_expr) {
            ERROR::UnexpectedToken(*walker.get(), "vector size expression");
        }
    }
    if (!walker.CheckValue("]"))
        ERROR::UnexpectedToken(*walker.get(), "']'");
    Token end = *walker.get();
    walker.next();

    if (walker.CheckValue("{")) {
        auto type_node = make_unique<NodeNewArrayType>(std::move(type_expr), std::move(size_expr), start, end);
        auto array = ParseArray();
        if (!array) {
            ERROR::UnexpectedToken(*walker.get(), "array");
        }
        ((NodeArray*)array.get())->static_type = std::move(type_node);
        ((NodeArray*)array.get())->is_static = true;
        return array;
    }

    return make_unique<NodeNewArrayType>(std::move(type_expr), std::move(size_expr), start, end);
}

unique_ptr<Node> ASTGenerator::ParseArray() {
    walker.next(); // pass '{'
    vector<tuple<unique_ptr<Node>, Token, Token>> values;
    if (walker.CheckValue("}")) {
        walker.next(); // pass '}' token
        return make_unique<NodeArray>(std::move(values));
    }
    while (true) {
        Token start_value = *walker.get();
        auto expr = parse_expression();
        Token end_value = *walker.get(-1);
        if (!expr) {
            ERROR::UnexpectedToken(*walker.get(), "expression");
        }
        values.push_back(std::make_tuple(std::move(expr), start_value, end_value));
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            continue;
        }
        break;
    }
    if (!walker.CheckValue("}"))
        ERROR::UnexpectedToken(*walker.get(), "'}'");
    walker.next();
    return make_unique<NodeArray>(std::move(values));
}

unique_ptr<Node> ASTGenerator::ParseGetIndex(unique_ptr<Node> expr, Token start, Token end) {
    walker.next(); // pass '[' token
    auto index = parse_expression();
    if (!index) {
        ERROR::UnexpectedToken(*walker.get(), "expression");
    }
    if (!walker.CheckValue("]"))
        ERROR::UnexpectedToken(*walker.get(), "']'");
    Token end_bracket = *walker.get();
    walker.next();
    return make_unique<NodeGetIndex>(std::move(expr), std::move(index), start, end_bracket);
}

unique_ptr<Node> ASTGenerator::ParseStructDecl() {
    auto token = *walker.get();
    walker.next(); // pass 'struct'

    if (!walker.CheckType(TokenType::LITERAL)) 
        ERROR::UnexpectedToken(*walker.get(), "structure name");

    string name = walker.get()->value;
    walker.next();
    
    auto body = parse_statement();

    return make_unique<NodeStructDeclaration>(std::move(body), name, token);
}
