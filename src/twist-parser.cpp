#include "twist-nodetemp.cpp"
#include "twist-tokenwalker.cpp"
#include "twist-tokens.cpp"
#include "twist-errors.cpp"
#include "twist-err.cpp"
#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "twist-args.cpp"

// basic nodes
#include "Nodes/NodeNumber.cpp"
#include "Nodes/NodeString.cpp"
#include "Nodes/NodeChar.cpp"
#include "Nodes/NodeBool.cpp"
#include "Nodes/NodeNull.cpp"
#include "Nodes/NodeLiteral.cpp"
#include "Nodes/NodeScopes.cpp"

// binary and unary operators
#include "Nodes/NodeUnary.cpp"
#include "Nodes/NodeBinary.cpp"

// declarations
#include "Nodes/NodeVariableDeclaration.cpp"
#include "Nodes/NodeNamespaceDeclaration.cpp"
#include "Nodes/NodeFunctionDeclaration.cpp"
#include "Nodes/NodeStructDeclaration.cpp"
#include "Nodes/NodeBlockOfDeclarations.cpp"

// resolutions
#include "Nodes/NodeNamespaceResolution.cpp"
#include "Nodes/NodeObjectResolution.cpp"


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

#include "Nodes/NodeNamespaceAnonim.cpp"
#include "Nodes/NodeNew.cpp"

#include "Nodes/NodeReturn.cpp"
#include "Nodes/NodeExpressionState.cpp"
#include "Nodes/NodeLambda.cpp"
#include "Nodes/NodeAddresOf.cpp"
#include "Nodes/NodeDereference.cpp"
#include "Nodes/NodeDelete.cpp"
#include "Nodes/NodeLeftDereference.cpp"
#include "Nodes/NodeCall.cpp"
#include "Nodes/NodeFunctionType.cpp"
#include "Nodes/NodeArrayType.cpp"
#include "Nodes/NodeArray.cpp"
#include "Nodes/NodeGetIndex.cpp"
#include "Nodes/NodeArrayPush.cpp"

#include <vcruntime_startup.h>
#include <string>
#include <vector>
#include <any>

#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"


using namespace std;


struct ASTGenerator {
    vector<Node*> nodes;
    TokenWalker& walker;
    string file_name;

    // Declarations
    Node* ParseVariableDecl();
    Node* ParseFinalVariableDecl();
    Node* ParseStaticVariableDecl();
    Node* ParseConstVariableDecl();
    Node* ParseGlobalVariableDecl();
    Node* ParsePrivateVariableDecl();
    Node* ParseBlockDecl(string modifier);

    // Input and outputs
    Node* ParseOut();
    Node* ParseOutLn();
    Node* ParseInput();

    // Delete and new
    Node* ParseDelete();
    Node* ParseNew();

    // Loops
    Node* ParseWhile();
    Node* ParseDoWhile();
    Node* ParseFor();

    // Block of instructs
    Node* ParseBlock();
    Node* ParseScopes();

    // Namespace
    Node* ParseNameSpaceDecl();
    Node* ParseNamespace();
    Node* ParseNameResolution(Node* expression);
    Node* ParseObjectResolution(Node* expression);

    // Conditions
    Node* ParseIf();
    Node* ParseIfExpr();

    // Call
    Node* ParseCall(Node* expr, Token start, Token end);

    // Base literals
    Node* ParseNumber();
    Node* ParseString();
    Node* ParseChar();
    Node* ParseBool();
    Node* ParseNull();
    Node* ParseLiteral();

    // Thread instructions
    Node* ParseBreak();
    Node* ParseContinue();
    Node* ParseExit();
    Node* ParseReturn();

    // Assert
    Node* ParseAssert();

    // Pointers
    Node* ParseAddressOf();
    Node* ParseDereference();
    Node* ParseLeftDereference();

    // Metafunctions
    Node* ParseTypeof();
    Node* ParseSizeof();

    // Lambda
    Node* ParseLambda();

    // Functions
    Node* ParseNewFunctionType();
    Node* ParseFuncDecl();

    // Special
    Node* ParsePostfix(Node* expr);

    // Arrays
    Node* ParseNewArrayType();
    Node* ParseArray();
    Node* ParseGetIndex(Node* expr, Token start, Token end);

    // Structs
    Node* ParseStructDecl();

    Memory GLOBAL_MEMORY;


    ASTGenerator(TokenWalker& walker, string file_name) : walker(walker), file_name(file_name) {}

    Node* parse_primary_expression() {
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
            return ParsePostfix(expr);
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

    Node* parse_unary_expression() {
        if (walker.CheckType(TokenType::OPERATOR) && (walker.CheckValue("-") || walker.CheckValue("+")
        || walker.CheckValue("--") || walker.CheckValue("++") ||
            walker.CheckValue("!") || walker.CheckValue("not"))) {
            string op = walker.get()->value;
            Token operator_token = *walker.get();
            walker.next(); // pass operator
            Token start = *walker.get();
            auto operand = parse_unary_expression();
            if (!operand)
                throw ERROR_THROW::UnexpectedToken(start, "expression");
            Token end = *walker.get(-1);
            return new NodeUnary(operand, start, end, operator_token);
        }
        if (walker.CheckType(TokenType::OPERATOR) && walker.CheckValue("&")) {
            return ParseAddressOf();
        }
        if (walker.CheckType(TokenType::DEREFERENCE) && walker.CheckValue("*")) {
            return ParseDereference();
        }
        
        auto expr = parse_primary_expression();
        if (!expr) {
            if (walker.CheckType(TokenType::OPERATOR) && walker.CheckValue(":") && walker.CheckType(TokenType::OPERATOR, 1) && walker.CheckValue(":", 1))
                throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");
            else 
                return nullptr;
        }
        return ParsePostfix(std::move(expr));
    }

    // Вспомогательный метод для парсинга бинарных выражений с заданными операторами
    Node* ParseBinaryLevel(Node* (ASTGenerator::*parseHigherLevel)(),
        const vector<string>& operators, string level_name
    ) {
        Token& start_token = *walker.get();
        Token& start_less = *walker.get(-1);
        
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
            if (!right)
                throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression after " + level_name + " operator");
            if (!left)
                throw ERROR_THROW::UnexpectedToken(start_less, "expression before " + level_name + " operator");


            Token& end_token = *(walker.get() - 1);
            left = new NodeBinary(left, op, right, start_token, end_token, op_token);
        }

        return left;
    }

    Node* parse_binary_expression_or() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_and,
                            {"or", "||"}, "logical OR");
    }

    Node* parse_binary_expression_and() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_eq_ne_in_ni,
                            {"and", "&&"}, "logical AND");
    }

    Node* parse_binary_expression_eq_ne_in_ni() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_less_more,
                            {"==", "!=", "<<", ">>"}, "equality/innary");
    }

    Node* parse_binary_expression_less_more() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_modul,
                            {"<", ">", "<=", ">="}, "comparison");
    }

    Node* parse_binary_expression_modul() {
        return ParseBinaryLevel( &ASTGenerator::parse_binary_expression_sum_sub,
                            {"%"}, "modulus");
    }

    Node* parse_binary_expression_sum_sub() {
        return ParseBinaryLevel(&ASTGenerator::parse_binary_expression_mul_div,
                            {"+", "-"}, "addition/subtraction");
    }

    Node* parse_binary_expression_mul_div() {
        return ParseBinaryLevel( &ASTGenerator::parse_binary_expression_exp,
                            {"*", "/"}, "multiplication/division");
    }

    Node* parse_binary_expression_exp() {
        return ParseBinaryLevel(&ASTGenerator::parse_unary_expression,
                            {"**", "|"}, "exponentiation/bitwise OR");
    }

    Node* parse_higher_order_expressions() {
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
            expr = new NodeArrayPush(expr, value_expr, op_token, end);
        }

        return expr;
    }

    Node* parse_expression() {
        return parse_higher_order_expressions();
    }

    Node* parse_statement() {
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
            return ParseVariableDecl();
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
        
        if (left_expr) {
            auto end_left_value_token = *walker.get(-1);

            // Проверяем оператор присваивания
            if (walker.CheckValue("=")) {
                // Обычное присваивание
                walker.next();
                auto start_value_token = *walker.get();
                auto expr = parse_expression();

                if (!expr)
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

                if (!walker.CheckValue(";"))
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
                auto end_value_token = *walker.get();
                walker.next();

                return new NodeVariableEqual(left_expr, expr,
                                                    start_left_value_token, end_left_value_token,
                                                    start_value_token, end_value_token);
            } else if (walker.CheckValue("<-")) {
                // Оператор push в массив
                Token op_token = *walker.get();
                walker.next();
                auto start_value_token = *walker.get();
                auto expr = parse_expression();

                if (!expr)
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

                if (!walker.CheckValue(";"))
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
                auto end_value_token = *walker.get();
                walker.next();

                // Создаем NodeArrayPush вместо NodeVariableEqual
                return new NodeArrayPush(left_expr, expr,
                                                op_token, end_value_token);
            } else {
                // Просто expression statement
                if (!walker.CheckValue(";"))
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
                walker.next();
                return new NodeExpressionStatement(left_expr);
            }
        }
        
        return nullptr;
    }

    inline void parse() {


        while (!walker.isEnd()) {
            auto stmt = parse_statement();
            if (stmt) {
                this->nodes.push_back(std::move(stmt));
            }
        }
    }

};

void GenerateStandartTypes(Memory* g_memory, string g_file_name) {
    auto OBJ_TYPE_INT = CreateMemoryObject(NewType("Int"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Int",OBJ_TYPE_INT);
    STATIC_MEMORY.register_object(OBJ_TYPE_INT);

    auto OBJ_TYPE_DOUBLE = CreateMemoryObject(NewType("Double"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Double",OBJ_TYPE_DOUBLE);
    STATIC_MEMORY.register_object(OBJ_TYPE_DOUBLE);

    auto OBJ_TYPE_CHAR = CreateMemoryObject(NewType("Char"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Char",OBJ_TYPE_CHAR);
    STATIC_MEMORY.register_object(OBJ_TYPE_CHAR);

    auto OBJ_TYPE_STRING = CreateMemoryObject(NewType("String"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("String",OBJ_TYPE_STRING);
    STATIC_MEMORY.register_object(OBJ_TYPE_STRING);

    auto OBJ_TYPE_BOOL = CreateMemoryObject(NewType("Bool"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Bool",OBJ_TYPE_BOOL);
    STATIC_MEMORY.register_object(OBJ_TYPE_BOOL);

    auto OBJ_TYPE_TYPE = CreateMemoryObject(NewType("Type"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Type",OBJ_TYPE_TYPE);
    STATIC_MEMORY.register_object(OBJ_TYPE_TYPE);

    auto OBJ_TYPE_NULL = CreateMemoryObject(NewType("Null"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Null",OBJ_TYPE_NULL);
    STATIC_MEMORY.register_object(OBJ_TYPE_NULL);


    auto OBJ_TYPE_NAMESPACE = CreateMemoryObject(NewType("Namespace"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Namespace",OBJ_TYPE_NAMESPACE);
    STATIC_MEMORY.register_object(OBJ_TYPE_NAMESPACE);

    auto OBJ_TYPE_LAMBDA = CreateMemoryObject(NewType("Lambda"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("Lambda",OBJ_TYPE_LAMBDA);
    STATIC_MEMORY.register_object(OBJ_TYPE_LAMBDA);

    auto OBJ_TYPE_AUTO = CreateMemoryObject(NewType("auto"), STANDART_TYPE::TYPE, g_memory,
        true, true, true, true, false);
    g_memory->add_object("auto",OBJ_TYPE_AUTO);
    STATIC_MEMORY.register_object(OBJ_TYPE_AUTO);

    auto __TWIST_FILE__ = CreateMemoryObject(Value(STANDART_TYPE::STRING, string(g_file_name)), STANDART_TYPE::STRING, g_memory,
        true, true, true, true, false);
    g_memory->add_object("__FILE__", __TWIST_FILE__);
    STATIC_MEMORY.register_object(__TWIST_FILE__);

    auto __TWIST_ADDR__ = CreateMemoryObject(NewPointer(AddressManager::get_current_address() + 2, STANDART_TYPE::NULL_T), STANDART_TYPE::STRING, g_memory,
        true, true, true, true, false);
    g_memory->add_object("__PTR__", __TWIST_ADDR__);
    STATIC_MEMORY.register_object(__TWIST_ADDR__);
}



// PASS
Node* ASTGenerator::ParseReturn() {
    walker.next();
    Token start = *walker.get();
    auto expr = parse_expression();

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");

    Token end = *walker.get(-1);
    walker.next();
    return new NodeReturn(expr, start, end);
}

// PASS
Node* ASTGenerator::ParseExit() {
    Token start = *walker.get();
    walker.next();
    auto expr = parse_expression();
    Token end = *(walker.get());

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");

    walker.next();
    return new NodeExit(expr, start, end);
}

// PASS
Node* ASTGenerator::ParsePostfix(Node* expr) {
    
    while (true) {
        if (walker.CheckValue("(")) {
            Token start = *walker.get();
            Token end;
            expr = ParseCall(expr, start, end);
        } else if (walker.CheckValue("[")) {
            Token start = *walker.get();
            Token end;
            expr = ParseGetIndex(expr, start, end);
        } else if (walker.CheckValue(":") && walker.CheckValue(":", 1)) {
            expr = ParseNameResolution(expr);
        } else if (walker.CheckValue(".")) {
            expr = ParseObjectResolution(expr);
        } else {
            break;
        }
    }
    return expr;
}

// PASS
Node* ASTGenerator::ParseFuncDecl() {
    walker.next();

    if (!walker.CheckType(TokenType::LITERAL))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "function name");
    string name = walker.get()->value;
    walker.next();

    // parse arguments
    Token start_args_token = *walker.get();
    if (!walker.CheckValue("("))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'('");
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
        Node* variadic_size_expr = nullptr;

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
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "variable name");
        string arg_name = walker.get()->value;
        walker.next();

        Node* type_expr = nullptr;
        Node* default_expr = nullptr;

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
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "variadic size expression");
                if (!walker.CheckValue("]"))
                    throw ERROR_THROW::UnexpectedToken(*walker.get(), "']'");
            }
            walker.next();
        }


        if (!walker.CheckValue(":"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "':'");



        walker.next();
        type_expr = parse_expression();
        if (!type_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "type expression");


        if (walker.CheckValue("=")) {
            walker.next();
            default_expr = parse_expression();
            if (!default_expr)
                throw ERROR_THROW::UnexpectedToken(*walker.get(), "default value expression");
        }

        auto arg = new Arg(arg_name);
        arg->type_expr = type_expr;
        arg->default_parameter = default_expr;
        arg->is_const = arg_is_const;
        arg->is_static = arg_is_static;
        arg->is_final = arg_is_final;
        arg->is_global = arg_is_global;
        arg->is_variadic = is_variadic;
        arg->variadic_size = variadic_size_expr;

        arguments.push_back(arg);

        if (!walker.CheckValue(",") && !walker.CheckValue(")"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "',' or ')'");

        if (walker.CheckValue(",")) walker.next();
    }

    Token end_args_token = *walker.get(-1);
    Token start_return_token = *walker.get();
    Node* return_type_expr = nullptr;
    if (walker.CheckValue("->")) {
        walker.next();
        start_return_token = *walker.get();
        return_type_expr = parse_expression();
        if (!return_type_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "return type expression");
    }
    Token end_return_token = *walker.get(-1);
    auto statement = parse_statement();

    return new NodeFunctionDeclaration(name, arguments, return_type_expr, statement, start_args_token, end_args_token, start_return_token, end_return_token);
}


Node* ASTGenerator::ParseNewFunctionType() {
    walker.next(); // pass "Func" token

    if (!walker.CheckValue("("))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'('");

    Token start_token = *walker.get();
    walker.next();

    // Обработка пустого списка аргументов
    if (walker.CheckValue(")")) {
        Token end_token = *walker.get();
        walker.next(); // pass ")" token

        // Теперь проверяем, есть ли возвращаемый тип
        Node* return_type_expr = nullptr;
        Token start_token_return;
        Token end_token_return;

        if (walker.CheckValue("->")) {
            walker.next(); // pass "->" token
            start_token_return = *walker.get();
            return_type_expr = parse_expression();
            end_token_return = *walker.get(-1);
            if (!return_type_expr)
                throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

            return new NodeNewFuncType(
                vector<Node*>(),
                return_type_expr,
                start_token, end_token,
                start_token_return, end_token_return
            );
        } else {
            return new NodeNewFuncType(
                vector<Node*>(),
                nullptr,
                start_token, end_token,
                Token(), Token()
            );
        }
    }

    vector<Node*> type_args;
    while (true) {
        auto type_expr = parse_expression();

        if (!type_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

        type_args.push_back(type_expr);

        if (walker.CheckValue(")") || walker.CheckValue(",")) {
            if (walker.CheckValue(",")) {
                walker.next();
                continue;
            }
            else if (walker.CheckValue(")")) break;
        }
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "',' or ')'");
    }

    Token end_token = *walker.get();
    walker.next(); // pass ")" token

    Node* return_type_expr = nullptr;
    Token start_token_return;
    Token end_token_return;

    if (walker.CheckValue("->")) {
        walker.next(); // pass "->" token
        start_token_return = *walker.get();
        return_type_expr = parse_expression();
        end_token_return = *walker.get(-1);
        if (!return_type_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");
    }

    return new NodeNewFuncType(
        type_args,
        return_type_expr,
        start_token, end_token,
        start_token_return, end_token_return
    );
}


Node* ASTGenerator::ParseNew() {
    walker.next(); // pass 'new' token
    bool is_static = false;
    bool is_const = false;
    Node* type_expr = nullptr;

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
                        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");
                    walker.next();
                }
            }
            else throw ERROR_THROW::UnexpectedToken(*walker.get(), "modifiers ('const', 'static')");

            if (walker.CheckValue(",") || walker.CheckValue(">")) {
                if (walker.CheckValue(",")) {
                    walker.next();
                    continue;
                }
                else if (walker.CheckValue(">")) break;
            }
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "',' or '>'");
        }
        walker.next(); // pass ")" token
    }

    auto expr = parse_expression();

    return new NodeNew(expr, type_expr, is_static, is_const, start_type, end_type);
}


Node* ASTGenerator::ParseCall(Node* expr, Token start, Token end) {
    walker.next(); // pass '(' token

    vector<Node*> arguments;
    while (true) {
        if (walker.CheckValue(")") && walker.CheckType(TokenType::R_BRACKET)) {
            walker.next();
            break;
        }
        auto arg = parse_expression();
        if (!arg)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "argument expression");
        arguments.push_back(arg);

        if (!walker.CheckValue(")") && !walker.CheckValue(",") && !walker.CheckType(TokenType::R_BRACKET)) {
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "',' or ')'");
        }

        if (walker.CheckValue(",")) walker.next();
    }
    end = *walker.get(-1);
    return new NodeCall(expr, arguments, start, end);
}


Node* ASTGenerator::ParseLambda() {
    walker.next(); // pass 'lambda' token
    Token start_args_token = *walker.get();

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Блок передачи самой Lambda функции фнутрь себя для рекурсивных вызовов
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    string name = "";
    if (walker.CheckValue("[")) {
        walker.next();
        if (walker.CheckType(TokenType::LITERAL)) {
            name = walker.get()->value;
            walker.next();
        } else 
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "literal (not `String` or `Char`)");
        
        if (!walker.CheckValue("]"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "']'");
        walker.next();
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Парсинг аргументов
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if (!walker.CheckValue("("))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    vector<Arg*> arguments;

    while (true) {
        if (walker.CheckValue(")")) {
            walker.next();
            break;
        }

        // Поддерживем лишь модификатор global
        bool arg_is_global = false;
        if (walker.CheckValue("global")) {
            arg_is_global = true;
            walker.next();
        }
        
        
        if (!walker.CheckType(TokenType::LITERAL))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "variable name");
        string arg_name = walker.get()->value;
        walker.next();

        Node* type_expr = nullptr;

        if (!walker.CheckValue(":"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "':'");
        walker.next();

        type_expr = parse_expression();
        if (!type_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "type expression");

        auto arg = new Arg(arg_name);
        arg->type_expr = type_expr;
        arg->is_global = arg_is_global;

        arguments.push_back(arg);

        if (!walker.CheckValue(",") && !walker.CheckValue(")"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "',' or ')'");

        if (walker.CheckValue(",")) walker.next();
    }
    Token end_args_token = *walker.get(-1);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Парсинг возвращаемого типа
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if (!(walker.CheckValue("->") && walker.CheckType(TokenType::OPERATOR)))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "return type (syntax: -> type expression)");
    walker.next();

    Token return_type_start_token = *walker.get();
    auto return_type = parse_expression();
    Token return_type_end_token = *walker.get(-1);

    if (!return_type)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "return type expression");
    

    if (!walker.CheckValue(":"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "':'");
    walker.next();
    //////////////////////////////////////////////////////////////////////////////////////////////////////



    auto lambda_node = new NodeLambda(nullptr, arguments, return_type,
        start_args_token, end_args_token, return_type_start_token, return_type_end_token);
    lambda_node->name = name;

    // парсим тело
    auto body = parse_expression();
    if (!body)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "lambda body");

    lambda_node->body = body;

    return lambda_node;
}

// PASS
Node* ASTGenerator::ParseAssert() {
    walker.next(); // pass 'assert' token
    Token start_token = *walker.get();
    auto expr = parse_expression();
    Node* message_expr = nullptr;
    Token end_token = *walker.get(-1);
    Token start_message, end_message;
    if (!expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");
    if (walker.CheckValue(",")) {
        walker.next(); // pass ','
        start_message = *walker.get();
        message_expr = parse_expression();
        end_message = *walker.get(-1);
        if (!expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "message expression");
    }
    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
    walker.next();
    return new NodeAssert(expr, message_expr, start_token, end_token, start_message, end_message);
}


Node* ASTGenerator::ParseNamespace() {
    walker.next(); // pass "namespace" token
    auto namespace_node = new NodeNamespace(nullptr);

    auto block = parse_statement();
    namespace_node->statement = block;

    return namespace_node;
}

// PASS
Node* ASTGenerator::ParseInput() {
    auto start_token = *walker.get();
    walker.next(); // pass 'input' token
    auto end_token = *walker.get(-1);
    Node* expr = nullptr;

    if (walker.CheckValue("(")) {
        walker.next();
        start_token = *walker.get(-1);
        expr = parse_expression();
        if (!walker.CheckValue(")"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");
        end_token = *walker.get();
        walker.next();
    }
    return new NodeInput(expr, start_token, end_token);
}


Node* ASTGenerator::ParseLeftDereference() {
    walker.next(); // pass '*' token

    auto start_left_value_token = *walker.get(-1);
    auto left_expr = parse_expression();

    if (!left_expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");


    auto end_left_value_token = *walker.get(-1);

    if (!walker.CheckValue("="))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'='");
    walker.next();
    auto start_value_token = *walker.get();
    auto expr = parse_expression();

    if (!expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");


    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
    auto end_value_token = *walker.get();
    walker.next();

    return new NodeLeftDereference(left_expr, expr, start_left_value_token, end_left_value_token, start_value_token, end_value_token);
}

// PASS
Node* ASTGenerator::ParseTypeof() {
    walker.next(); // pass 'typeof' token
    walker.next(); // pass '(' token

    auto expr_token = *walker.get();
    auto expr = parse_expression();

    if (!walker.CheckValue(")"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')' after typeof");
    walker.next();
    return new NodeTypeof(expr, expr_token);
}

// PASS
Node* ASTGenerator::ParseSizeof() {
    walker.next(); // pass 'sizeof' token
    walker.next(); // pass '(' token

    auto expr_token = *walker.get();
    auto expr = parse_expression();

    if (!walker.CheckValue(")"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')' after sizeof");
    walker.next();
    return new NodeSizeof(expr, expr_token);
}


Node* ASTGenerator::ParseAddressOf() {
    walker.next(); // pass '&' token
    auto expr = parse_primary_expression();
    return new NodeAddressOf(expr);
}


Node* ASTGenerator::ParseDereference() {
    walker.next(); // pass '*' token
    if (walker.CheckValue("(")) {
        Token start_token = *walker.get();
        walker.next();
        auto expr = parse_expression();
        if (!expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");
        if (!walker.CheckValue(")"))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");
        Token end_token = *walker.get();
        walker.next();
        return new NodeDereference(expr, start_token, end_token);
    } else {
        Token start_token = *walker.get();
        auto expr = parse_expression();
        if (!expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "primary expression or (expression)");
        Token end_token = *walker.get(-1);
        return new NodeDereference(expr, start_token, end_token);
    }
}


// PASS
Node* ASTGenerator::ParseBlockDecl(string modifier) {
    walker.next(); // pass '{'

    // Создаем блок для хранения всех объявлений
    vector<Node*> declarations;

    // Парсим все объявления внутри блока
    while (!walker.CheckValue("}")) {
        auto stmt = parse_statement();
        if (!stmt)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "'}'");

        declarations.push_back(stmt);
    }

    if (!walker.CheckValue("}"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'}'");
    walker.next();

    auto node = new NodeBlockDecl(std::move(declarations));
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

    return node;
}

// PASS
Node* ASTGenerator::ParseContinue() {
    walker.next(); // pass 'break'

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    return new NodeContinue();
}

// PASS
Node* ASTGenerator::ParseBreak() {
    walker.next(); // pass 'break'

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    return new NodeBreak();
}

// PASS
Node* ASTGenerator::ParseFor() {
    walker.next(); // pass 'for' token

    if (!walker.CheckValue("("))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto init_state = parse_statement();

    auto check_expr = parse_expression();

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
    walker.next();

    auto update_state = parse_statement();
    if (!update_state)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "update statement");


    if (!walker.CheckValue(")"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    auto body_token = *walker.get();
    auto body = parse_statement();

    return new NodeFor(init_state, check_expr, update_state, body, body_token);
}

// PASS
Node* ASTGenerator::ParseWhile() {
    walker.next(); // pass 'while' token

    if (!walker.CheckValue("("))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto expr = parse_expression();

    if (!walker.CheckValue(")"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");
    walker.next();


    auto body_token = *walker.get();
    auto body = parse_statement();

    if (!body)
        throw ERROR_THROW::UnexpectedToken(body_token, "statement");

    if (body)

    return new NodeWhile(expr, body, body_token);
}

// PASS
Node* ASTGenerator::ParseDoWhile() {
    walker.next(); // pass 'do' token

    auto body_token = *walker.get();
    auto state = parse_statement();

    if (state->NODE_TYPE != NODE_BLOCK_OF_NODES)
        throw ERROR_THROW::UnexpectedToken(body_token, "block of statements");

    if (!walker.CheckValue("while"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'while'");
    walker.next();

    if (!walker.CheckValue("("))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'('");
    walker.next();

    auto expr = parse_expression();

    if (!walker.CheckValue(")"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");
    walker.next();

    return new NodeDoWhile(expr, state);
}


Node* ASTGenerator::ParseDelete() {
    walker.next(); // pass 'del' token

    Token start_token = *walker.get();

    // Парсим выражение, которое нужно удалить
    auto target_expr = parse_expression();

    Token end_token = *walker.get(-1);

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");

    walker.next(); // pass ';'

    return new NodeDelete(target_expr, start_token, end_token);
}

Node* ASTGenerator::ParseObjectResolution(Node* expression) {
    while (walker.CheckValue(".")) {
        Token dot_token = *walker.get(); // токен точки (может пригодиться для ошибок)
        walker.next(); // pass '.'

        if (!walker.CheckType(TokenType::LITERAL))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "literal");

        Token name_token = *walker.get(); // токен имени поля
        string literal_name = name_token.value;
        walker.next(); // pass literal

        // Создаём узел, сохраняя токены имени для точного указания ошибки
        expression = new NodeObjectResolution(
            expression,
            literal_name,
            name_token,   // start = токен имени
            name_token    // end = токен имени
        );
    }
    return expression;
}

// PASS
Node* ASTGenerator::ParseNameResolution(Node* expression) {
    // Ожидаем, что текущий токен — ':' и следующий тоже ':'
    Token start = *walker.get();            // первый ':'
    walker.next();                           // пропускаем первый ':'
    walker.next();                           // пропускаем второй ':'

    if (!walker.CheckType(TokenType::LITERAL))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "literal");

    Token name_token = *walker.get();
    string literal_name = name_token.value;
    walker.next();                           // пропускаем имя

    Token end = *walker.get(-1);              // последний считанный токен (имя)

    // Создаём узел только для одного уровня разрешения
    return new NodeNamespaceResolution(
        expression,
        literal_name,
        start,
        end
    );
}


Node* ASTGenerator::ParseNameSpaceDecl() {
    walker.next(); // pass 'namespace' token

    if (!walker.CheckType(TokenType::LITERAL))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "namespace identifier");
    Token decl_token = *walker.get();
    string namespace_name = walker.get()->value;
    walker.next();

    // Создаем namespace
    auto namespace_node = new NodeNamespaceDeclaration(nullptr, namespace_name, decl_token);

    auto block = parse_statement();
    namespace_node->statement = block;

    return namespace_node;
}


Node* ASTGenerator::ParseIfExpr() {
    walker.next(); // pass 'if' token

    if (!walker.CheckType(TokenType::L_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "(");
    walker.next();

    auto eq_expr = parse_expression();
    if (!eq_expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), ")");
    walker.next();

    if (!walker.CheckType(TokenType::L_CURVE_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "{");
    walker.next();

    auto true_expr = parse_expression();
    if (!true_expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "}");
    walker.next();

    Node* else_expr = nullptr;
    if (walker.CheckValue("else")) {
        walker.next();
        if (!walker.CheckType(TokenType::L_CURVE_BRACKET))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "{");
        walker.next();
        else_expr = parse_expression();

        if (!else_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

        if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "}");
        walker.next();
    }

    return new NodeIfExpr(eq_expr, true_expr, else_expr);
}


Node* ASTGenerator::ParseIf() {
    walker.next(); // pass 'if' token

    if (!walker.CheckType(TokenType::L_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "(");
    walker.next();

    auto eq_expr = parse_expression();
    if (!eq_expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckType(TokenType::R_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), ")");

    walker.next();

    auto true_state = parse_statement();
    Node* else_state = nullptr;

    if (walker.CheckValue("else")) {
        walker.next();
        else_state = parse_statement();
        if (!else_state)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "statement or statement block");
    }

    return new NodeIf(eq_expr, true_state, else_state);
}

// PASS
Node* ASTGenerator::ParseBlock() {
    walker.next(); // pass '{'
    vector<Node*> nodes_array;
    if (walker.CheckType(TokenType::R_CURVE_BRACKET)) {
        walker.next(); // pass '}'
        return new NodeBlock(nodes_array);
    }
    
    while (true) {
        auto statement = parse_statement();
        if (!statement) break;
        nodes_array.push_back(statement);
    }
    if (!walker.CheckType(TokenType::R_CURVE_BRACKET))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "}");
    walker.next();
    return new NodeBlock(nodes_array);
}

// PASS
Node* ASTGenerator::ParseLiteral() {
    auto node = new NodeLiteral(*walker.get());
    walker.next();
    return node;
}

// PASS
Node* ASTGenerator::ParseNumber() {
    auto node = new NodeNumber(*walker.get());
    walker.next();
    return node;
}

// PASS
Node* ASTGenerator::ParseString() {
    auto node = new NodeString(walker.get()->value);
    walker.next();
    return node;
}

// PASS
Node* ASTGenerator::ParseChar() {
    auto node = new NodeChar(walker.get()->value[0]);
    walker.next();
    return node;
}

// PASS
Node* ASTGenerator::ParseBool() {
    auto node = new NodeBool(*walker.get());
    walker.next();
    return node;
}

// PASS
Node* ASTGenerator::ParseNull() {
    auto node = new NodeNull();
    walker.next();
    return node;
}

// PASS
Node* ASTGenerator::ParseScopes() {
    walker.next(); // pass '(' token
    auto expr = parse_expression();
    if (!expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckValue(")"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "')'");

    walker.next(); // pass ')' token
    return new NodeScopes(std::move(expr));
}


Node* ASTGenerator::ParseVariableDecl() {
    
    walker.next(); // pass 'let' token


    if (!walker.CheckType(TokenType::LITERAL))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "variable name");
    Token variable_token = *walker.get();
    string var_name = walker.get()->value;
    walker.next(); // pass variable name token

    Node* type_expr = nullptr; // type expression
    Token type_start_token;
    Token type_end_token;
    

    if (walker.CheckValue(":")) {
        walker.next(); // pass ':' token
        type_start_token = *walker.get();
        
        type_expr = parse_expression();
        
        if (!type_expr) 
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "type expression");
        
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
        auto expr = new NodeNull();
        walker.next();
        return new NodeVariableDeclaration(var_name, expr, variable_token,
                                             type_expr, type_start_token, type_end_token, nullable, start_expr_token, end_expr_token);
    }

    if (!walker.CheckValue("="))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'='");

    walker.next(); // pass '=' token

    start_expr_token = *walker.get();
    auto expr = parse_expression();
    end_expr_token = *walker.get(-1);

    if (!expr)
        throw ERROR_THROW::ExpectedExpression(*walker.get());

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");
    walker.next(); // pass ';' token
    
    return new NodeVariableDeclaration(var_name, expr, variable_token,
                                             type_expr, type_start_token, type_end_token, nullable, start_expr_token, end_expr_token);
}


Node* ASTGenerator::ParseFinalVariableDecl() {
    walker.next(); // pass 'final' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        throw ERROR_THROW::ExpectedDeclarationStatement(*walker.get());
    }


    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl)->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl)->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl)->is_final = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl)->is_final = true;
        } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeFunctionDeclaration*)decl)->is_final = true;
    } else {
        throw ERROR_THROW::ExpectedDeclarationStatement(token);
    }

    return decl;
}


Node* ASTGenerator::ParseStaticVariableDecl() {
    walker.next(); // pass 'static' token

    if (!walker.CheckValue("final") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        throw ERROR_THROW::ExpectedDeclarationStatement(*walker.get());
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl)->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl)->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl)->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl)->is_static = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl)->is_static = true;
    } else {
        throw ERROR_THROW::ExpectedDeclarationStatement(token);
    }
    return decl;
}


Node* ASTGenerator::ParseConstVariableDecl() {
    walker.next(); // pass 'const' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("final") && !walker.CheckValue("global") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        throw ERROR_THROW::ExpectedDeclarationStatement(*walker.get());
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl)->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl)->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl)->is_const = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl)->is_const = true;
        } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl)->is_const = true;
    } else {
        throw ERROR_THROW::ExpectedDeclarationStatement(token);
    }

    return decl;
}


Node* ASTGenerator::ParseGlobalVariableDecl() {
    walker.next(); // pass 'const' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("final") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("private") && !walker.CheckValue("struct")) {
        throw ERROR_THROW::ExpectedDeclarationStatement(*walker.get());
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl)->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl)->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl)->is_global = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl)->is_global = true;
        } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl)->is_global = true;
    } else {
        throw ERROR_THROW::ExpectedDeclarationStatement(token);
    }
    return decl;
}


Node* ASTGenerator::ParsePrivateVariableDecl() {
    walker.next(); // pass 'private' token

    if (!walker.CheckValue("static") && !walker.CheckValue("let") &&
        !walker.CheckValue("const") && !walker.CheckValue("final") &&
        !walker.CheckValue("namespace") && !walker.CheckValue("func") &&
        !walker.CheckValue("global") && !walker.CheckValue("struct")) {
        throw ERROR_THROW::ExpectedDeclarationStatement(*walker.get());
    }

    auto token = *walker.get();
    auto decl = parse_statement();

    if (decl->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
        ((NodeVariableDeclaration*)decl)->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
        ((NodeNamespaceDeclaration*)decl)->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_BLOCK_OF_DECLARATIONS) {
        ((NodeBlockDecl*)decl)->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
        ((NodeFunctionDeclaration*)decl)->is_private = true;
    } else if (decl->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
        ((NodeStructDeclaration*)decl)->is_private = true;
    } else {
        throw ERROR_THROW::ExpectedDeclarationStatement(token);
    }
    return decl;
}


Node* ASTGenerator::ParseOut() {
    walker.next(); // pass 'out'
    vector<Node*> args;
    args.push_back(parse_expression());
    while (true) {
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            auto expr = parse_expression();
            if (!expr)
                throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

            args.push_back(expr);
            continue;
        }
        break;
    }

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");

    walker.next();
    return new NodeBaseOut(args);
}


Node* ASTGenerator::ParseOutLn() {
    walker.next(); // pass 'out'
    vector<Node*> args;
    args.push_back(parse_expression());
    while (true) {
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            auto expr = parse_expression();
            if (!expr)
                throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

            args.push_back(expr);
            continue;
        }
        break;
    }

    if (!walker.CheckValue(";"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "';'");

    walker.next();
    return new NodeBaseOutLn(std::move(args));
}


Node* ASTGenerator::ParseNewArrayType() {
    Token start = *walker.get();
    walker.next(); // pass "["
    auto type_expr = parse_expression();
    Node* size_expr = nullptr;

    if (walker.CheckValue(",")) {
        walker.next(); // pass ',' token
        size_expr = parse_expression();
        if (!size_expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "vector size expression");

    }
    if (!walker.CheckValue("]"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "']'");
    Token end = *walker.get();
    walker.next();

    if (walker.CheckValue("{")) {
        auto type_node = new NodeNewArrayType(type_expr, size_expr, start, end);
        auto array = ParseArray();
        if (!array)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "array");

        ((NodeArray*)array)->static_type = type_node;
        ((NodeArray*)array)->is_static = true;
        return array;
    }

    return new NodeNewArrayType(type_expr, size_expr, start, end);
}


Node* ASTGenerator::ParseArray() {
    walker.next(); // pass '{'
    vector<tuple<Node*, Token, Token>> values;
    if (walker.CheckValue("}")) {
        walker.next(); // pass '}' token
        return new NodeArray(std::move(values));
    }
    while (true) {
        Token start_value = *walker.get();
        auto expr = parse_expression();
        Token end_value = *walker.get(-1);
        if (!expr)
            throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

        values.push_back(std::make_tuple(expr, start_value, end_value));
        if (walker.CheckValue(",")) {
            walker.next(); // pass ',' token
            continue;
        }
        break;
    }
    if (!walker.CheckValue("}"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "'}'");
    walker.next();
    return new NodeArray(values);
}


Node* ASTGenerator::ParseGetIndex(Node* expr, Token start, Token end) {
    walker.next(); // pass '[' token
    auto index_expr = parse_expression();
    if (!index_expr)
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "expression");

    if (!walker.CheckValue("]"))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "']'");

    Token end_bracket = *walker.get();
    walker.next();
    return new NodeGetIndex(expr, index_expr, start, end_bracket);
}


Node* ASTGenerator::ParseStructDecl() {
    auto token = *walker.get();
    walker.next(); // pass 'struct'

    if (!walker.CheckType(TokenType::LITERAL))
        throw ERROR_THROW::UnexpectedToken(*walker.get(), "structure name");

    string name = walker.get()->value;
    walker.next();

    auto body = parse_statement();

    return new NodeStructDeclaration(body, name, token);
}
