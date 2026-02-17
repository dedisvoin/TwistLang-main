#include "twist-tokens.cpp"
#include "twist-utils.cpp"
#include "twist-values.cpp"


namespace ERROR_TYPES {
    const string PARSE_ERROR = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::RED + "parse" + TERMINAL_COLORS::RESET;
    const string SYNTAX      = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::YELLOW + "syntax" + TERMINAL_COLORS::RESET;
    const string EXECUTION   = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::MAGENTA + "exec" + TERMINAL_COLORS::RESET;
    const string SEMANTIC    = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::CYAN + "semantic" + TERMINAL_COLORS::RESET;
    const string MSG         = "[ " + TERMINAL_COLORS::BLACK + "msg" + TERMINAL_COLORS::RESET + " ] ";
    const string WRN         = "[ " + TERMINAL_COLORS::YELLOW + " ! " + TERMINAL_COLORS::RESET + " ] ";
    const string FIX         = "[ " + TERMINAL_COLORS::GREEN + " + " + TERMINAL_COLORS::RESET + " ] ";
}

void MSG(string message) {
    cout << ERROR_TYPES::MSG + TERMINAL_COLORS::MAGENTA << message << TERMINAL_COLORS::RESET << endl;
}

void WRN(string message) {
    cout << ERROR_TYPES::WRN + TERMINAL_COLORS::YELLOW << message << TERMINAL_COLORS::RESET << endl;
}

void FIX(string message) {
    cout << ERROR_TYPES::FIX + TERMINAL_COLORS::GREEN << message << TERMINAL_COLORS::RESET << endl;
}

namespace ERROR {
    static string PREPROCESSOR_OUTPUT;
    // GOOD
    void UnexpectedToken(const Token& token, const string& expected) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::SYNTAX << " >> " << token.pif << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(token.pif.line).length() + 3, ' ') << string(token.pif.index, ' ') << TERMINAL_COLORS::RED << string(token.pif.lenght, '^') << " Expected " << expected << ", but found '" << token.value << "'" << endl;
        cout << TM::RED << "`" << string(to_string(token.pif.line).length() + 4, '-') << string(token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }



    // GOOD
    void InvalidNumber(const Token& token, const string& value) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::PARSE_ERROR << " >> " << token.pif << " >> Invalid number: '" << value << "'" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(token.pif.line).length() + 3, ' ') << string(token.pif.index, ' ') << TERMINAL_COLORS::RED << string(token.pif.lenght, '^') << " Invalid number format: '" << value << "'" << endl;
        cout << TM::RED << "`" << string(to_string(token.pif.line).length() + 4, '-') << string(token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }



    // GOOD
    void UnsupportedBinaryOperator(const Token& start, const Token& end, const Token& op_t,
                            const Value& value_l, const Value& value_r) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << op_t.pif << " >> Unsupported operator: '" << op_t.value << "'" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(op_t.pif.index + op_t.pif.lenght - 1, ' ') << TM::RED << ".---- Ivalid operator: '" << op_t.value << "'" << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(op_t.pif.index, ' ') << TM::RED << string(op_t.pif.lenght, 'v') << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << 
        TM::YELLOW  << string(op_t.pif.index - start.pif.index, '^') << 
        string(op_t.pif.lenght, '~') << 
        string(end.pif.index - (op_t.pif.index + op_t.pif.lenght) + end.pif.lenght, '^') << 
        " `" << value_l.type.pool << "` and `" << value_r.type.pool <<"` types are not support this binary operator" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }



    // GOOD
    void UnsupportedUnaryOperator(const Token& op_t, const Token& start, const Token& end, const Value& value) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << op_t.pif << " >> Unsupported operator: '" << op_t.value << "'" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(op_t.pif.index + op_t.pif.lenght - 1, ' ') << TM::RED << ".---- Ivalid operator: '" << op_t.value << "'" << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(op_t.pif.index, ' ') << TM::RED << string(op_t.pif.lenght, 'v') << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::YELLOW << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " `" << value.type.pool << "` type is not support this unary operator" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }


    // GOOD
    void InvalidType(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid value, expected <type expression> or 'auto' keyword" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }


    // GOOD
    void StaticTypesMisMatch(const Token& start, const Token& end, Type waited_type, Type found_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> invalid instruction" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Variable waited `" << waited_type.pool << "` type, but found `" << found_type.pool << "` type" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        FIX("Remove the 'static' keyword in the variable declaration or change the type of the variable to `" + found_type.pool + "`");
        exit(0);
    }

    void CanNotDeleteUndereferencedValue(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Types mismatch" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::YELLOW << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Can't delete an undereferencable typed value" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
    }


    // GOOD
    void IncompartableTypeVarDeclaration(const Token& start, const Token& end, const Token& start_expr, const Token& end_expr, Type waited_type, Type found_type) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Incompartable types" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        if (found_type.pool != "Null") {
            cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(end_expr.pif.index + end_expr.pif.lenght - 1, ' ') << TM::RED << ".---- This expression type `" << found_type.pool << "` but waited `" << waited_type.pool << "`" << endl;
            cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start_expr.pif.index, ' ') << TM::RED << string(end_expr.pif.index - start_expr.pif.index + end_expr.pif.lenght, 'v') << endl;
        }
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        string messsage = "Incompartable type in this variable declaration statement";
        if (found_type.pool == "Null")
            messsage = "Use 'auto' for this variable declaration statement";
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " " << messsage << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Use '?' symbol after type expression to automatic create nullable type."); cout << endl;
        MSG("After use '?' this variable type been '" + waited_type.pool + " | Null'.");
        exit(0);
    }


    // GOOD
    void IncompartableTypeInput(const Token& start, const Token& end, Type found_type) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Incompartable type" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " " << "Input instruction wait `String` or `Char` type but found `" << found_type.pool << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }

    // GOOD
    void InvalidDereferenceType(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid dereference" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid dereference type" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Syntax: '*'<variable name> to dereference a variable.");
        MSG("        '*'<type name> to create a pointer type.");
        WRN("Union type unsupported to creating a pointer of union types.");
        FIX("if you want to create a pointer of union type, use *<type name> | *<type name> | ...");
        exit(0);
    }


    // GOOD
    void IvalidCallableType(const Token& start, const Token& end, Type& type) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid call" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid callable type `" << type.pool << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("You must call a this object types (lambda, function, method)");
        exit(0);
    }

        // Ошибка: неверное выражение размера для variadic-параметра (не Int или отрицательное)
    void InvalidVariadicSizeExpression(const Token& start, const Token& end, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid variadic size expression" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Variadic size must be of type `Int`, got `" << actual_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Variadic parameter syntax: `name[size_expr]: Type` or `name[]: Type` for dynamic size.");
        exit(0);
    }

    // Ошибка: неверный тип элемента в variadic-аргументе
    void InvalidVariadicArgumentType(const Token& start, const Token& end, const string& expected, const string& got, int index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid variadic argument type" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Variadic argument " << index << " expected type `" << expected << "`, but got `" << got << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }

    // Ошибка: несоответствие количества элементов в variadic-аргументе (для фиксированного размера)
    void VariadicSizeMismatch(const Token& start, const Token& end, int expected, int actual) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Variadic size mismatch" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Expected " << expected << " arguments for variadic parameter, but got " << actual << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }

    // GOOD
    void PrivateVariableAccess(const Token& start, const Token& end, string name) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Private variable access" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::YELLOW << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Variable '" << name << "' is private" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        
    }


    // GOOD
    void InvalidLambdaArgumentCount(const Token& start, const Token& end, const Token& start_args, const Token& end_args, int wait_count, int found_count) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid call" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line-1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Arguments count mismatch" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << TM::YELLOW << ". " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line-1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Waited " << wait_count << " arguments but found " << found_count << " arguments" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }


    // GOOD
    void InvalidLambdaArgumentType(const Token& start, const Token& end, const Token& start_args, const Token& end_args, Type wait_type, Type found_type, string index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << "+ " << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid call" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid argument type" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << TM::YELLOW << ". " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Argument '" << index << "' waited `" << wait_type.pool << "` but found `" << found_type.pool << "` type" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }

    void InvalidFuncArgumentCount(const Token& start, const Token& end, const Token& start_args, const Token& end_args, int wait_count, int found_count) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid call" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line-1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Arguments count mismatch" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << TM::YELLOW << ". " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line-1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Waited " << wait_count << " arguments but found " << found_count << " arguments" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }

    void InvalidFuncArgumentType(const Token& start, const Token& end, const Token& start_args, const Token& end_args, Type wait_type, Type found_type, string index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

    
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << "+ " << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid call" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid argument type" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << TM::YELLOW << ". " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Argument '" << index << "' waited `" << wait_type.pool << "` but found `" << found_type.pool << "` type" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }


    // GOOD
    void InvalidLambdaReturnType(const Token& start, const Token& end, const Token start_args, const Token end_args, Type wait_type, Type found_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << "+ " << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid call" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid return type `" << found_type.pool << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << TM::YELLOW << ". " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Return waited `" << wait_type.pool << "` but found `" << found_type.pool << "` type" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }


    // GOOD
    void WaitedLambdaArgumentTypeSpecifier(const Token& start_args, const Token& end_args, string index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::SEMANTIC << " >> " << start_args.pif << " >> Invalid type specifier" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Invalid type specifier for argument '" << index << "'" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }

    
    void WaitedFuncTypeArgumentTypeSpecifier(const Token& start_args, const Token& end_args, string index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::SEMANTIC << " >> " << start_args.pif << " >> Invalid type specifier" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Invalid type specifier for argument '" << index << "'" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }

    void WaitedFuncTypeArgumentTypeSpecifier(const Token& start_args, const Token& end_args, int index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::SEMANTIC << " >> " << start_args.pif << " >> Invalid type specifier" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Invalid type specifier for argument '" << index << "'" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }

    static void MissingFuncArgument(const Token& start_callable, const Token& end_callable, 
                                const Token& arg_start, const Token& arg_end, 
                                const string& arg_name, int arg_index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::SEMANTIC << " >> " << start_callable.pif << " >> Missing argument" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start_callable.pif.line << " | " << TM::RESET << lines[start_callable.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_callable.pif.line).length() + 3, ' ') 
            << string(start_callable.pif.index, ' ') << TM::YELLOW 
            << string(end_callable.pif.index + end_callable.pif.lenght - start_callable.pif.index, '^') 
            << " Missing argument at position " << arg_index + 1 << " with no default value" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << arg_start.pif.line << " | " << TM::RESET << lines[arg_start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(arg_start.pif.line).length() + 3, ' ') 
            << string(arg_start.pif.index, ' ') << TM::YELLOW 
            << string(arg_end.pif.index + arg_end.pif.lenght - arg_start.pif.index, '^') 
            << " Argument '" << arg_name << "' declared here" << endl;
        cout << TM::YELLOW << "`" << string(to_string(arg_start.pif.line).length() + 4, '-') 
            << string(arg_start.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }


    void WaitedFuncTypeReturnTypeSpecifier(const Token& start_args, const Token& end_args) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::SEMANTIC << " >> " << start_args.pif << " >> Invalid type specifier" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Invalid reurn type specifier" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        
        exit(0);
    }

    void WaitedLambdaReturnTypeSpecifier(const Token& start_args, const Token& end_args) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::SEMANTIC << " >> " << start_args.pif << " >> Invalid type specifier" << endl;
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start_args.pif.line << " | " << TM::RESET << lines[start_args.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start_args.pif.line).length() + 3, ' ') << string(start_args.pif.index, ' ') << TM::YELLOW << string(end_args.pif.index + end_args.pif.lenght - start_args.pif.index, '^') << " Invalid return type specifier" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start_args.pif.line).length() + 4, '-') << string(start_args.pif.index, '-') << "'" << TM::RESET << endl;
        FIX("Waited type specifier for return type 'Int', 'Float | Double', ... ");
        exit(0);
    }

    void InvalidDereferenceValue(const Token& start, const Token& end, Type type) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid dereference" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid dereference value, waited <variable name> or <type name>, but found `" << type.pool << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }


    void AssertionIvalidArgument(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid assertion argument" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid assertion argument, waited `Bool` type" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }

    void AssertionFailed(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Assertion failed" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::YELLOW << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Assertion failed" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;

    }


    // GOOD
    void ConstRedefinition(const Token& start, const Token& end, const string& var_name) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Constant mutation"  << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Variable '" << var_name << "' cannot be mutated, because it is declared as constant value" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        FIX("Remove the 'const' keyword in variable declaration statement.");
        exit(0);
    }


    void ConstPointerRedefinition(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Constant mutation"  << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Pointer value cannot be mutated, because it is declared as pointer to constant value" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        FIX("Remove the 'const' keyword in declaration expression.");
        exit(0);
    }


    void InvalidDeleteInstruction(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid delete instruction" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid argument" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Delete instruction waits a variable name or pointer.");
        exit(0);
    }

    void InvalidNewInstruction(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid new instruction" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid 'new' syntax" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("'new' instruction syntax:");
        MSG("   new _value_;");
        MSG("   new <const> _value_;");
        MSG("   new <static(_type_)>;");
        MSG("   new <static(_type_), const> _value_;");
        MSG("   ...");
        exit(0);
    }


    /////////////////////////////////////////

    void UnexpectedStatement(const Token& token, const string& expected) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << "::" << ERROR_TYPES::SEMANTIC << " >> " << token.pif << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(token.pif.line).length() + 3, ' ') << string(token.pif.index, ' ') << TERMINAL_COLORS::RED << string(token.pif.lenght, '^') << " Expected " << expected << " statement, but found " << token.value << " statement" << endl;
        cout << TM::RED << "`" << string(to_string(token.pif.line).length() + 4, '-') << string(token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    void WaitedTypeExpression(const Token& token) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::SEMANTIC<< " >> " << token.pif << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(token.pif.line).length() + 3, ' ') << string(token.pif.index, ' ') << TERMINAL_COLORS::RED << string(token.pif.lenght, '^') << " Expected sytnax :<type expression> or 'auto'" << endl;
        cout << TM::RED << "`" << string(to_string(token.pif.line).length() + 4, '-') << string(token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    void UndefinedVariable(const Token& token) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION  << " >> " << token.pif << " >> Undefined variable: '" << token.value << "'" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(token.pif.line).length() + 3, ' ') << string(token.pif.index, ' ') << TM::RED << string(token.pif.lenght, '^') << " Undefined variable: '" << token.value << "'" << endl;
        cout << TM::RED << "`" << string(to_string(token.pif.line).length() + 4, '-') << string(token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    void UndefinedLeftVariable(const Token& start, const Token& end, string name) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Undefine variable" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Undefined variable '" << name << "'" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    void VariableAlreadyDefined(const Token& token, const string& var_name) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION  << " >> " << token.pif << " >> Final variable redefinition: '" << var_name << "'" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(token.pif.line).length() + 3, ' ') << string(token.pif.index, ' ') << TM::RED << string(token.pif.lenght, '^') << " Variable '" << var_name << "' already defined" << endl;
        cout << TM::RED << "`" << string(to_string(token.pif.line).length() + 4, '-') << string(token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    

    static void UndefinedVariableInNamespace(const string& var_name, const string& ns_name) {
        cout << MT::ERROR + "Variable '" + var_name + "' not found in namespace '" + ns_name + "'" << endl;
        exit(1);
    }

    static void InvalidType(const string& expected, const string& actual) {
        cout << MT::ERROR + "Invalid type. Expected " + expected + ", got " + actual << endl;
        exit(1);
    }

    // Ошибка: неверный тип массива для операции push
    void InvalidArrayPushType(const Token& start, const Token& end, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid array push" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Cannot use '<-' operator on non-array type `" << actual_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // Ошибка: неверный тип элемента при добавлении в массив
    void InvalidArrayElementTypeOnPush(const Token& start, const Token& end, const string& expected_type, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << " >> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid element type" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Cannot push value of type `" << actual_type << "` into array of element type `" << expected_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    void InvalidArrayElementType(const Token& start, const Token& end, const string& expected_type, const string& actual_type, size_t index) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << " >> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid array element type" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Array waited element of type `" << expected_type << "`, but found element of type `" << actual_type << "` at index " << index << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // Ошибка: неверный индекс (не целое число)
    void InvalidArrayIndex(const Token& index_token, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << index_token.pif << " >> Invalid array index" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << index_token.pif.line << " | " << TM::RESET << lines[index_token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(index_token.pif.line).length() + 3, ' ') << string(index_token.pif.index, ' ') << TM::RED << string(index_token.pif.lenght, '^') << " Array index must be of type `Int`, got `" << actual_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(index_token.pif.line).length() + 4, '-') << string(index_token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // Ошибка: индекс выходит за границы массива
    void ArrayIndexOutOfRange(const Token& index_token, int64_t index, int64_t size) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << index_token.pif << " >> Array index out of range" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << index_token.pif.line << " | " << TM::RESET << lines[index_token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(index_token.pif.line).length() + 3, ' ') << string(index_token.pif.index, ' ') << TM::RED << string(index_token.pif.lenght, '^') << " Index " << index << " is out of bounds for array of size " << size << endl;
        cout << TM::RED << "`" << string(to_string(index_token.pif.line).length() + 4, '-') << string(index_token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // Ошибка: неверный тип для exit
    void InvalidExitType(const Token& start, const Token& end, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid exit type" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " 'exit' expects `Int` type, got `" << actual_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // Ошибка: неверный тип для array type
    void InvalidArrayTypeExpression(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid array type" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Expected type expression in array type declaration" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // Ошибка: неверный размер массива
    void InvalidArraySize(const Token& size_token) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << size_token.pif << " >> Invalid array size" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << size_token.pif.line << " | " << TM::RESET << lines[size_token.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(size_token.pif.line).length() + 3, ' ') << string(size_token.pif.index, ' ') << TM::RED << string(size_token.pif.lenght, '^') << " Array size must be of type `Int`" << endl;
        cout << TM::RED << "`" << string(to_string(size_token.pif.line).length() + 4, '-') << string(size_token.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // GOOD
    // Ошибка: попытка использовать оператор :: на не-namespace типе
    void InvalidAccessorType(const Token& start, const Token& end, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid accessor operator" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Cannot use '::' accessor on type `" << actual_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("The '::' operator can only be used to access members of a namespace.");
        MSG("Valid syntax: namespace::member");
        exit(0);
    }

    // GOOD
    // Ошибка: попытка доступа к несуществующему свойству в namespace
    void UndefinedProperty(const Token& start, const Token& end, const string& property_name, const string& namespace_name) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Undefined property" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Property '" << property_name << "' not found in namespace '" << namespace_name << "'" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }

    // GOOD
    // Ошибка: попытка доступа к приватному свойству через оператор ::
    void PrivatePropertyAccess(const Token& start, const Token& end, const string& property_name) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::YELLOW << ".- " << TM::RESET << MT::WARNING << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Private property access" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::YELLOW << "|" << TM::RESET << endl;
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::YELLOW << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Property '" << property_name << "' is private and cannot be accessed" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        WRN("Private members can only be accessed from within the same namespace scope.");
    }

    // GOOD
    // Ошибка: неверный тип в цепочке доступа к namespace
    void InvalidNamespaceChainType(const Token& start, const Token& end, const string& chain_element, const string& actual_type) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid namespace chain" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Error in namespace chain at '" << chain_element << "': expected namespace, but found `" << actual_type << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Each element in namespace chain (A::B::C) must be a namespace.");
        exit(0);
    }

    // GOOD
    // Ошибка: неверный тип выражения для операции delete
    void InvalidDeleteTarget(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;
        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid delete target" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.global_line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid target for delete operation" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Delete operation expects a variable name, namespace property (var::prop), or dereferenced pointer (*ptr).");
        exit(0);
    }
}
