#include "twist-nodetemp.cpp"
#include "twist-tokens.cpp"
#include "twist-utils.cpp"
#include "twist-values.cpp"
#include <string>
#include "sstream"

#pragma once

namespace ErrorTypes {
    const string PARSE_ERROR = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::RED + "parse" + TERMINAL_COLORS::RESET;
    const string SYNTAX      = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::YELLOW + "syntax" + TERMINAL_COLORS::RESET;
    const string EXECUTION   = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::MAGENTA + "exec" + TERMINAL_COLORS::RESET;
    const string SEMANTIC    = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::CYAN + "semantic" + TERMINAL_COLORS::RESET;
    const string ECHO    = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::BLUE + "echo" + TERMINAL_COLORS::RESET;
    const string PREPROCESS_ERROR = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::RED + "preprocessing" + TERMINAL_COLORS::RESET;
}

typedef string ErrorType;



struct Error {
    static std::string error_buffer;
    string message;
    PosInFile pif;
    ErrorType type;
    string code;
    int message_type = 0;

    Error* sub_error = nullptr;

    Error() {}

    Error(string message, PosInFile pif, ErrorType type, string code) {
        this->message = message;
        this->pif = pif;
        this->type = type;
        this->code = code;
    }

    static std::string escape_message(const std::string& msg) {
        std::string escaped;
        escaped.reserve(msg.size());
        for (char c : msg) {
            switch (c) {
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n";  break;
                case '\r': escaped += "\\r";  break;
                case '\t': escaped += "\\t";  break;
                default:   escaped += c;     break;
            }
        }
        return escaped;
    }

    void write_error_to_file(std::ostream& out, const Error& err) const {
        out << "pif: " << err.pif << ":" << err.pif.lenght << ":" << err.message_type
            << " message: " << escape_message(err.message);
        if (err.sub_error) {
            out << "\n";
            write_error_to_file(out, *err.sub_error);
        }

    }

    std::string ToString() const {
        std::ostringstream oss;
        write_error_to_file(oss, *this);
        return oss.str();
    }

    void Write() const {
        error_buffer += ToString();
        error_buffer += "\n";
    }

    static void ClearBuffer() {
        error_buffer.clear();
    }
    static const std::string& GetBuffer() {
        return error_buffer;
    }


    // Старый Write больше не нужен, используйте Write(buffer)

    Error(string message, PosInFile start_pif, PosInFile end_pif, ErrorType type, string code) {
        PosInFile new_pif;
        new_pif.file_path = start_pif.file_path;
        new_pif.file_name = start_pif.file_name;
        new_pif.global_line = start_pif.global_line;
        new_pif.line = start_pif.line;
        new_pif.index = start_pif.index;
        new_pif.lenght = end_pif.index - start_pif.index + end_pif.lenght;
        this->message = message;
        this->pif = new_pif;
        this->type = type;
        this->code = code;
    }

    void print() {
        if (sub_error)
            sub_error->print();

        auto color = TM::RED;
        auto err = MT::ERROR;
        if (message_type == 1) {
            color = TM::YELLOW;
            err = MT::WARNING;
        }

        cout << color << ".- " << TM::RESET << err << ">> " << this->type << " >> " << pif << endl;
        vector<string> lines = SplitString(this->code, '\n');
        cout << color << "|" << TM::RESET << endl;
        cout << color << "| " << TM::CYAN << pif.line << " | " << TM::RESET << lines[pif.global_line - 1] << endl;
        cout << color << "| " << string(to_string(pif.line).length() + 3, ' ') << string(pif.index, ' ') << color << string(pif.lenght, '^') << " " << this->message << endl;
        cout << color << "`" << string(to_string(pif.line).length() + 4, '-') << string(pif.index, '-') << "'" << TM::RESET << endl;
        cout << endl;
    }
};

std::string Error::error_buffer = "";
namespace ERROR_THROW {
    static string PREPROCESSOR_OUTPUT;

    // PREPROCESSOR ERRORS
    Error PreprocessorWaitedEqual(const Token& pos) {
        Error err = Error("Waited '='", pos.pif, ErrorTypes::PREPROCESS_ERROR, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error PreprocessorMaxIterations(const Token& pos) {
        Error err = Error("Waited ';' but you use max iterations", pos.pif, ErrorTypes::PREPROCESS_ERROR, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error PreprocessorWaitFilePath(const Token& pos) {
        Error err = Error("Waited \"file path\"", pos.pif, ErrorTypes::PREPROCESS_ERROR, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error PreprocessorWaitLiteral(const Token& pos) {
        Error err = Error("Waited literal", pos.pif, ErrorTypes::PREPROCESS_ERROR, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error UnexpectedToken(const Token& token, const string& expected) {
        Error err;
        err.pif = token.pif;
        err.message = "Expected " + expected + ", but found '" + token.value + "'";
        err.type = ErrorTypes::SYNTAX;
        err.code = PREPROCESSOR_OUTPUT;
        return err;
    }

    Error ExpectedDeclarationStatement(const Token& token) {
        Error err;
        err.pif = token.pif;
        err.message = "Expected declaration statement [let, func, struct, namespace], but found '" + token.value + "'";
        err.type = ErrorTypes::SEMANTIC;
        err.code = PREPROCESSOR_OUTPUT;
        return err;
    }

    Error ExpectedExpression(const Token& token) {
        Error err;
        err.pif = token.pif;
        err.message = "Expected expression, but found '" + token.value + "'";
        err.type = ErrorTypes::SEMANTIC;
        err.code = PREPROCESSOR_OUTPUT;
        return err;
    }

    Error CallError(const Token& start, const Token& stop, string name, Error* sub_error, int is_warning = 0, string message = "") {
        Error err;
        if (is_warning) {
            err = Error(message, start.pif, stop.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        } else {
            err = Error("Call error in function '" + name + "'", start.pif, stop.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        }

        err.sub_error = sub_error;
        err.message_type = is_warning;
        return err;
    }

    Error CallError(const Token& start, const Token& stop, string name, int is_warning = 0, string message = "") {
        Error err;
        if (is_warning) {
            err = Error(message, start.pif, stop.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        } else {
            err = Error("Call error in function '" + name + "'", start.pif, stop.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        }

        err.message_type = is_warning;
        return err;
    }

    Error UncallableType(const Token& start, const Token& stop, Type type) {
        Error err = Error("Uncallable type `" + type.pool + "`", start.pif, stop.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error IncompartableInputType(const Token& start, const Token& end, Type found_type) {
        Error err = Error("Input instruction wait `String` or `Char` type but found `" + found_type.pool + "`", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error AssertionInvalidArgument(const Token& start, const Token& end) {
        Error err = Error("Invalid assertion argument, waited `Bool` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidNodeType(const Token& start, string wait_node, string node) {
        Error err = Error("Waited " + wait_node + ", but found " + node, start.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error AssertionInvalidMessage(const Token& start, const Token& end) {
        Error err = Error("Invalid assertion message, waited `String` type, or `Char` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error AssertionFailed(const Token& start, const Token& end) {
        Error err = Error("Assertion failed", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.message_type = 1;
        return err;
    }

    Error InputWarning(const Token& start, const Token& end) {
        Error err = Error("Input is run time instruction. Default return - ''", start.pif, end.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        err.message_type = 1;
        return err;
    }

    Error InfinityLoopWarning(const Token& start, const Token& end) {
        Error err = Error("Infinity loop", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.message_type = 1;
        return err;
    }

    Error UnusedLoopWarning(const Token& start, const Token& end) {
        Error err = Error("Unused loop", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.message_type = 1;
        return err;
    }

    Error ExitWarning(const Token& start, const Token& end, int code) {
        Error err = Error("Program exited with code " + to_string(code), start.pif, end.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        err.message_type = 1;
        return err;
    }

    Error Echo(const Token& start, const Token& end, string value) {
        Error err = Error(value, start.pif, end.pif, ErrorTypes::ECHO, PREPROCESSOR_OUTPUT);
        err.message_type = 2;
        return err;
    }

    Error AssertionFailed(const Token& start, const Token& end, string message) {
        Error err = Error("Assertion failed: " + message, start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.message_type = 1;
        return err;
    }

    Error ExitInvalidCode(const Token& start, const Token& end, Type type) {
        Error err = Error("Invalid exit code, waited `Int` type, but found `" + type.pool + "` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableAlreadyDefined(const Token& token) {
        Error err = Error("Variable '" + token.value + "' already defined (as final)", token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableUndefined(const Token& token) {
        Error err = Error("Undefined variable '" + token.value + "'", token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableUndefined(const Token& start, const Token& end, string name) {
        Error err = Error("Undefined variable '" + name + "'", start.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }


    Error VariableConstRedefinition(const Token& start, const Token& end, string name) {
        Error err = Error("Cannot assign to const variable '" + name + "'", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error PointerToConstRedefinition(const Token& start, const Token& end) {
        Error err = Error("The pointer points to a constant object", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableStaticTypesMisMatch(const Token& start, const Token& end, Type wait_type, Type found_type) {
        Error err = Error("Incompatible type `" + found_type.pool + "` (expected `" + wait_type.pool + "`)", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error NamespaceInvalidAccessorType(const Token& start, const Token& end, Type type) {
        Error err = Error("Cannot use '::' accessor on type `" + type.pool + "`", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableDeclarationInvalidType(const Token& start, const Token& end, Type type) {
        Error err = Error("Invalid variable static declaration type `" + type.pool + "`", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableStaticIncompatibleType(const Token& start, const Token& end, Type wait_type, Type found_type) {
        Error err = Error("Incompatible type `" + found_type.pool + "` (expected `" + wait_type.pool + "`)", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error NamespaceUndefinedVariable(const Token& start, const Token& end, string name) {
        Error err = Error("Undefined variable '" + name + "'", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error PrivateVariableAccess(const Token& start, const Token& end, string name) {
        Error err = Error("Variable '" + name + "' is private", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error UnsupportedUnaryOperator(const Token& operator_token, const Token& start, const Token& end, const Type& type) {
        Error err = Error("Unsupported unary operator '" + operator_token.value + "' for `" + type.pool + "` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error UnsupportedBinaryOperator(const Token& start_token, const Token& end_token, const Token& op_token, const Type& left_type, const Type& right_type) {
        Error err = Error("Unsupported binary operator '" + op_token.value + "' for `" + left_type.pool + "` and `" + right_type.pool + "` types", start_token.pif, end_token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error WaitedLambdaArgumentTypeSpecifier(const Token& start_token, const Token& end_token, string name) {
        Error err = Error("Invalid type specifier for argument '" + name + "'", start_token.pif, end_token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error WaitedLambdaReturnTypeSpecifier(const Token& start_token, const Token& end_token, Type type) {
        Error err = Error("Invalid type specifier `" + type.pool + "`, but waited valid type", start_token.pif, end_token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error WaitedLambdaReturnType(const Token& start_token, const Token& end_token) {
        Error err = Error("Waited return type", start_token.pif, end_token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidLambdaArgumentCount(const Token& start_callable, const Token& end_callable, const Token& start_args, const Token& end_args, size_t expected, size_t found) {
        Error err = Error("Invalid argument count for lambda, expected " + to_string(expected) + " but found " + to_string(found), start_callable.pif, end_callable.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Expected " + to_string(expected) + " arguments but found " + to_string(found), start_args.pif, end_args.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidLambdaArgumentType(const Token& start_callable, const Token& end_callable, const Token& start_args, const Token& end_args, Type expected, Type found, string arg_name) {
        Error err = Error("Invalid type for argument '" + arg_name + "' in lambda, expected `" + expected.pool + "` but found `" + found.pool + "`", start_callable.pif, end_callable.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Expected type `" + expected.pool + "` but found `" + found.pool + "` for argument '" + arg_name + "'", start_args.pif, end_args.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidLambdaReturnType(const Token& start_callable, const Token& end_callable, const Token& start_return_type, const Token& end_return_type, Type expected, Type found) {
        Error err = Error("Invalid return type for lambda, expected `" + expected.pool + "` but found `" + found.pool + "`", start_callable.pif, end_callable.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Expected return type `" + expected.pool + "`", start_return_type.pif, end_return_type.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error WaitedFuncArgumentTypeSpecifier(const Token& start_token, const Token& end_token, string name) {
        Error err = Error("Invalid type specifier for argument '" + name + "'", start_token.pif, end_token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error WaitedFuncReturnTypeSpecifier(const Token& start_token, const Token& end_token, Type type) {
        Error err = Error("Invalid type specifier `" + type.pool + "`, but waited valid type", start_token.pif, end_token.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidFuncReturnType(const Token& start_callable, const Token& end_callable, const Token& start_return_type, const Token& end_return_type, Type expected, Type found) {
        Error err = Error("Invalid return type for lambda, expected `" + expected.pool + "` but found `" + found.pool + "`", start_callable.pif, end_callable.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Expected return type `" + expected.pool + "`", start_return_type.pif, end_return_type.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidFuncArgumentCount(const Token& start_callable, const Token& end_callable, const Token& start_args, const Token& end_args, string func_name, size_t expected, size_t found) {
        Error err = Error("Invalid argument count for '" + func_name + "', expected " + to_string(expected) + " but found " + to_string(found), start_callable.pif, end_callable.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Expected " + to_string(expected) + " arguments but found " + to_string(found), start_args.pif, end_args.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidFuncArgumentType(const Token& start_callable, const Token& end_callable, const Token& start_args, const Token& end_args, Type expected, Type found, string arg_name, string func_name) {
        Error err = Error("Invalid type for argument '" + arg_name + "' in function '" + func_name +"', expected `" + expected.pool + "` but found `" + found.pool + "`", start_callable.pif, end_callable.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Expected type `" + expected.pool + "` but found `" + found.pool + "` for argument '" + arg_name + "'", start_args.pif, end_args.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidFuncVariadicSizeExpression(const Token& start, const Token& end, const Type actual_type) {
        return Error("Variadic size must be of type `Int`, got `" + actual_type.pool + "`", start.pif, end.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
    }

    Error InvalidFuncVariadicSize(const Token& start, const Token& end, const int n) {
        return Error("Variadic size must be positive number, got " + to_string(n), start.pif, end.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
    }

    Error InvalidFuncVariadicArgType(const Token& start, const Token& end, const Type expected, const Type got, string name){
        return Error("Variadic argument '" + name + "' expected type `" + expected.pool + "`, but got `" + got.pool + "`", start.pif, end.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
    }

    Error FuncArgumentMissing(const Token& start_callable, const Token& end_callable, const Token& start_args, const Token& end_args, const string& arg_name, int arg_index) {
        Error err = Error("Missing argument at position " + to_string(arg_index + 1) + " with no default value", start_callable.pif, end_callable.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        err.sub_error = new Error("Argument '" + arg_name + "' declared here", start_args.pif, end_args.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error FuncArgumentShadowsGlobal(const Token& call_start, const Token& call_end, const string& func_name, const string& arg_name) {
        Error err = Error("Argument '" + arg_name + "' in call to function '" + func_name + "' shadows a global variable with the same name", call_start.pif, call_end.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error VariableShadowsGlobal(const Token& call_start, const string& arg_name) {
        Error err = Error("Variable '" + arg_name + "' shadows a global variable with the same name", call_start.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error MaxRecursionDepthExceeded(const Token& start, const Token& end) {
        Error err = Error("Maximum recursion depth exceeded", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidObjectAccessorType(const Token& start, const Token& end, Type type) {
        Error err = Error("Cannot access members of type `" + type.pool + "`", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidNumber(const Token& token) {
        Error err = Error(" Invalid number format: '" + token.value + "'", token.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error WaitedAddresGettebleExpr(const Token& token) {
        Error err = Error(" Exprected addres (&) getteble expression", token.pif, ErrorTypes::SEMANTIC, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error CanNotGetAddress(const Token& start, const Token& end, NodeTypes node) {
        Error err = Error(" It is not possible to get an address from this node type: " + string(get_node_type_name(node)), start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error UndereferencableValue(const Token& start, const Token& end, Type type) {
        Error err = Error(" Invalid dereference value, waited pointer type but found `" + type.pool + "` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidNewModifier(const Token& start) {
        Error err = Error(" Unsupport modifier", start.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidNewInstruction(const Token& start, const Token& end) {
        Error err = Error(" Invalid new instruction", start.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error InvalidStringArgumentCount(const Token& start_args, const Token& end_args, size_t found) {
        Error err = Error("'String' expected 1 argument, but found " + to_string(found), start_args.pif, end_args.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error ArrayIndexOutOfRange(const Token& index_start, const Token& index_end, int64_t index, int64_t size) {
        return Error(" Index " + to_string(index) + " is out of bounds for array of size " + to_string(size), index_start.pif, index_end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
    }

    Error ArrayInvalidIndexType(const Token& index_start, const Token& index_end, const Type& actual_type) {
        return Error("Array index must be of type `Int`, got `" + actual_type.pool + "`", index_start.pif, index_end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
    }

    Error ArrayInvalidElementType(const Token& start, const Token& end, const Type expected_type, const Type actual_type, size_t index) {
        return Error("Array waited element of type `" + expected_type.pool + "`, but found element of type `" + actual_type.pool + "` at index " + to_string(index), start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
    }

    Error InvalidDereferenceAddres(const Token& start, const Token& end) {
        return Error("Ivalid dereference addres", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
    }
}
