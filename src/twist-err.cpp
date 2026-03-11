#include "twist-tokens.cpp"
#include "twist-utils.cpp"
#include "twist-values.cpp"
#include <string>

#pragma once

namespace ErrorTypes {
    const string PARSE_ERROR = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::RED + "parse" + TERMINAL_COLORS::RESET;
    const string SYNTAX      = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::YELLOW + "syntax" + TERMINAL_COLORS::RESET;
    const string EXECUTION   = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::MAGENTA + "exec" + TERMINAL_COLORS::RESET;
    const string SEMANTIC    = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::CYAN + "semantic" + TERMINAL_COLORS::RESET;
}

typedef string ErrorType;



struct Error {
    string message;
    PosInFile pif;
    ErrorType type;
    string code;
    bool assertion = false;

    Error* sub_error = nullptr;

    Error() {}

    Error(string message, PosInFile pif, ErrorType type, string code) {
        this->message = message;
        this->pif = pif;
        this->type = type;
        this->code = code;
    }

    Error(string message, PosInFile start_pif, PosInFile end_pif, ErrorType type, string code) {
        PosInFile new_pif;
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
        if (assertion) {
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

namespace ERROR_THROW {
    static string PREPROCESSOR_OUTPUT;

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

    Error CallError(const Token& start, const Token& stop, string name, Error* sub_error) {
        Error err = Error("Call error in function '" + name + "'", start.pif, stop.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.sub_error = sub_error;
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

    Error AssertionInvalidMessage(const Token& start, const Token& end) {
        Error err = Error("Invalid assertion message, waited `String` type, or `Char` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }

    Error AssertionFailed(const Token& start, const Token& end) {
        Error err = Error("Assertion failed", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.assertion = true;
        return err;
    }

    Error AssertionFailed(const Token& start, const Token& end, string message) {
        Error err = Error("Assertion failed : " + message, start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        err.assertion = true;
        return err;
    }

    Error ExitInvalidCode(const Token& start, const Token& end, Type type) {
        Error err = Error("Invalid exit code, waited `Int` type, but found `" + type.pool + "` type", start.pif, end.pif, ErrorTypes::EXECUTION, PREPROCESSOR_OUTPUT);
        return err;
    }
}
