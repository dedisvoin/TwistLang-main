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
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.line - 1] << endl;
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
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.line - 1] << endl;
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
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << 
        TM::YELLOW  << string(op_t.pif.index - start.pif.index, '^') << 
        string(op_t.pif.lenght, '~') << 
        string(end.pif.index - (op_t.pif.index + op_t.pif.lenght) + end.pif.lenght, '^') << 
        " `" << value_l.type->name << "` and `" << value_r.type->name <<"` types are not support this binary operator" << endl;
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
        cout << TM::YELLOW << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::YELLOW << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::YELLOW << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " `" << value.type->name << "` type is not support this unary operator" << endl;
        cout << TM::YELLOW << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }



    void InvalidType(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid value, expected <type expression> or 'auto' keyword" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
        exit(0);
    }



    void IncompartableTypeVarDeclaration(const Token& start, const Token& end, const Token& start_expr, const Token& end_expr, Type waited_type, Type found_type) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Incompartable types" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        if (found_type.name != "Null") {
            cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(end_expr.pif.index + end_expr.pif.lenght - 1, ' ') << TM::RED << ".---- This expression type `" << found_type.name << "` but waited `" << waited_type.name << "`" << endl;
            cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start_expr.pif.index, ' ') << TM::RED << string(end_expr.pif.index - start_expr.pif.index + end_expr.pif.lenght, 'v') << endl;
        }
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        string messsage = "Incompartable type in this variable declaration statement";
        if (found_type.name == "Null")
            messsage = "Use 'auto' for this variable declaration statement";
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " " << messsage << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Use '?' symbol after type expression to automatic create nullable type."); cout << endl;
        MSG("After use '?' this variable type been '" + waited_type.name + " | Null'.");
        exit(0);
    }


    void IncompartableTypeInput(const Token& start, const Token& end, Type found_type) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Incompartable type" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " " << "Input instruction wait `String` type but found `" << found_type.name << "`" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }


    void InvalidDereferenceType(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid dereference" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid dereference type" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Syntax: '*'<variable name> to dereference a variable.");
        MSG("        '*'<type name> to create a pointer type.");
        WRN("Union type unsupported to creating a pointer of union types.");
        FIX("if you want to create a pointer of union type, use *<type name> | *<type name> | ...");
        exit(0);
    }

    void InvalidDereferenceValue(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid dereference" << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid dereference value, waited <variable name> or <type name>" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        exit(0);
    }


    // GOOD
    void ConstRedefinition(const Token& start, const Token& end, const string& var_name) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Constant mutation"  << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Variable '" << var_name << "' cannot be mutated, because it is declared as constant value" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Remove the 'const' keyword in variable declaration statement.");
        exit(0);
    }


    void InvalidDeleteInstruction(const Token& start, const Token& end) {
        string file_lines = PREPROCESSOR_OUTPUT;
        vector<string> lines = SplitString(file_lines, '\n');

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << " >> " << start.pif << " >> Invalid delete instruction" << endl;
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
        cout << TM::RED << "| " << string(to_string(start.pif.line).length() + 3, ' ') << string(start.pif.index, ' ') << TM::RED << string(end.pif.index + end.pif.lenght - start.pif.index, '^') << " Invalid argument" << endl;
        cout << TM::RED << "`" << string(to_string(start.pif.line).length() + 4, '-') << string(start.pif.index, '-') << "'" << TM::RESET << endl;
        MSG("Delete instruction waits a variable name or pointer.");
        exit(0);
    }


    /////////////////////////////////////////

    void UnexpectedStatement(const Token& token, const string& expected) {
        string file_lines = PREPROCESSOR_OUTPUT;

        cout << TM::RED << ".- " << TM::RESET << MT::ERROR << ">> " << ERROR_TYPES::EXECUTION << "::" << ERROR_TYPES::SEMANTIC << " >> " << token.pif << endl;
        vector<string> lines = SplitString(file_lines, '\n');
        cout << TM::RED << "|" << TM::RESET << endl;
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.line - 1] << endl;
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
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.line - 1] << endl;
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
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.line - 1] << endl;
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
        cout << TM::RED << "| " << TM::CYAN << start.pif.line << " | " << TM::RESET << lines[start.pif.line - 1] << endl;
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
        cout << TM::RED << "| " << TM::CYAN << token.pif.line << " | " << TM::RESET << lines[token.pif.line - 1] << endl;
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
}
