#include "fstream"
#include "string"
#include <cctype>
#include <chrono>
#include <fstream>
#include <functional>
#include <string>
#include "vector"
#include "iostream"
#include "twist-tokens.cpp"
#include "chrono"

#pragma once

using namespace std;

// Split string by delimiter
vector<string> SplitString(const string& str, char delimiter) {
    vector<string> substrings;
    string current;
    for (char c : str) {
        if (c == delimiter) {
            substrings.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    substrings.push_back(current);
    return substrings;
}

// upper string
string ToUpper(string in) noexcept {
    string _;
    for(auto c : in) {
        _ += toupper(c);
    }
    return _;
}

// In string
bool InString(const char c, const string in) noexcept  {
    for (auto s: in) {
        if (c == s) return true;
    }
    return false;
}

template<class T>
bool InVector(vector<T> arr, T value) {
    for (auto v : arr) {
        if (v == value) return true;
    }
    return false;
}

bool IsSubString(const string str, const string sub_str, int index) {
    for (size_t i = static_cast<size_t>(index); i < static_cast<size_t>(index) + sub_str.size(); i++) {
        if (str[i] != sub_str[i - static_cast<size_t>(index)]) return false;
    }
    return true;
}

// Terminal colors
namespace TERMINAL_COLORS {
    string RESET = "\033[0m";
    string RED = "\033[31m";
    string GREEN = "\033[32m";
    string YELLOW = "\033[33m";
    string BLUE = "\033[34m";
    string MAGENTA = "\033[35m";
    string CYAN = "\033[36m";
    string WHITE = "\033[37m";
    string BOLD = "\033[1m";
    string UNDERLINE = "\033[4m";
    string BLACK = "\033[30m";
}
#define TM TERMINAL_COLORS

// Message types
// INFO - informational message
// WARNING - warning message
// ERROR - error message
// SUCCESS - success message
namespace MESSAGE_TYPES {
    string INFO = "[ " + TM::WHITE + "inf" + TM::RESET + " ] ";
    string WARNING = "[ " + TM::YELLOW + "wrn" + TM::RESET + " ] ";
    string ERROR = "[ " + TM::RED + TM::BOLD + "err" + TM::RESET + " ] ";
    string SUCCESS = "[ " + TM::GREEN + "yes" + TM::RESET + " ] ";
    string NOTE = "[ " + TM::BLACK + "msg" + TM::RESET + " ]";
}

#define MT MESSAGE_TYPES




// Program utility functions

/* Program file path is not foud */
void _FILE_NOT_FOUND(string file_name) {
    cout << MT::ERROR + "File '" + file_name + "' not found." << endl;
}

/* Program file is found */
void _FILE_FOUND(string file_name) {
    cout << MT::SUCCESS + "File '" + file_name + "' is found." << endl;
}

/* Program file path is not setted */
void _FILE_NOT_SETTED() {
    cout << MT::ERROR + "File argument not found." + "\n" +\
           MT::INFO + "Use --file <file_name> to set the file to be processed." + "\n" +\
           MT::INFO + "Or use 'twist.exe <file_name>' to set the file" << endl;
}   

/* Program file has invalid specifier */
void _FILE_INVALID_SPECIFIER(string file_name, string specifier) {
    cout << MT::ERROR + "In file '" + file_name + "' Invalid specifier '" + specifier + "'" + "\n" +\
           MT::INFO + "File specifier must be '.twist' or '.tw'." << endl;
}

/* Preprocessed file saved */
void _PREPROCESSED_FILE_SAVED(string file_name) {
    cout << MT::SUCCESS + "Preprocessed file '" + file_name + "' saved." << endl;
}

/* Tokens file saved */
void _TOKENS_FILE_SAVED(string file_name) {
    cout << MT::SUCCESS + "Tokens file '" + file_name + "' saved." << endl;
}



inline auto GetTimePoint() {
    return chrono::high_resolution_clock::now();
}

void TimeIt(string name, function<void()> func) {
    auto start = GetTimePoint();
    func();
    auto end = GetTimePoint();
    
    auto duration_ns = chrono::duration_cast<chrono::nanoseconds>(end - start);
    long long ns = duration_ns.count();
    
    // Разбиваем на единицы времени
    long long minutes = ns / (1000000000LL * 60);
    long long seconds = (ns / 1000000000LL) % 60;
    long long milliseconds = (ns / 1000000LL) % 1000;
    long long microseconds = (ns / 1000LL) % 1000;
    long long nanoseconds = ns % 1000;
    
    cout << MT::INFO << name << TERMINAL_COLORS::BLACK;
    
    bool printed = false;
    
    if (minutes > 0) {
        cout << minutes << "m ";
        printed = true;
    }
    if (seconds > 0 || minutes > 0) {
        cout << seconds << "s ";
        printed = true;
    }
    if (milliseconds > 0 || printed) {
        cout << milliseconds << "ms ";
    }
    if (microseconds > 0 && ns < 1000000) { // показываем микросекунды только для коротких операций
        cout << microseconds << "mcs ";
    }
    if (nanoseconds > 0 && ns < 1000) { // показываем наносекунды только для очень коротких
        cout << nanoseconds << "ns";
    }
    
    cout << TERMINAL_COLORS::RESET << endl;
}


/*
    Opens a file and returns its content as a single string.
*/
string OpenFile(string file_name) {
    ifstream stream(file_name);
    string result;
    string line;

    while (getline(stream, line)) {
        result += line + "\n";
    }
    return result;
}


/*
    Saves content to a file. Returns true if successful, false otherwise.
*/
bool SaveFile(string file_name, string content) {
    ofstream stream = ofstream(file_name);
    if (!stream.is_open()) {
        return false;
    }
    stream << content;
    stream.close();
    return true;
}

/*
    Saved tokens content to a file.
*/
bool SaveTokensFile(string file_name, vector<Token> tokens) {
    string content;

    for (auto token : tokens) {
        content += TokenTypeToString(token.type) + " : " + token.value + " -> " \
            + token.pif.file_name + ";" + to_string(token.pif.line) +  ";" + to_string(token.pif.index) \
            + " [" + to_string(token.pif.lenght) + "]"  + "\n";
    }
    
    if (SaveFile(file_name, content)) {
        _TOKENS_FILE_SAVED(file_name);
        return true;
    }
    return false;
}

/*
    Saves preprocessed file. Returns true if successful, false otherwise.
*/
bool SavePreprocessedFile(string file_name, string content) {
    if (SaveFile(file_name, content)) {
        _PREPROCESSED_FILE_SAVED(file_name);
        return true;
    }
    return false;
}

/*
    Converts command line arguments into a vector of strings.
*/
vector<string> ConvertArgs(int argc, char** argv) {
    vector<string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(string(argv[i]));
    }
    return args;
}


/*
    Debug method to output command line arguments.
*/
void OutArgs(vector<string> args) {
    for (string arg : args) {
        cout << arg << endl;
    }
}


/*
    Checks if the file has a valid specifier.
*/
bool CheckFileSpecifier(string file_name) {
    size_t dot_pos = file_name.find_last_of(".");
    if (dot_pos == string::npos) {
        return false;
    }
    string specifier = file_name.substr(dot_pos);
    return (specifier == ".twist" || specifier == ".tw");
}

/*
    Argument parser structure.
    Parses command line arguments and stores the file path.
*/
struct ArgsParser {

    vector<string> args;
    string file_path;
    bool save_preprocessed = false;
    bool save_token = false;
    bool interp_time = false;
    bool compile_mod = false;
    bool delete_precompiled = true;

    ArgsParser(vector<string> args) : args(args) {}

    void Parse() {
        if (args.size() == 2) {
            file_path = args[1];
        } else {
            for (size_t i = 0; i < args.size(); i++) {
                if (args[i] == "--file" && i + 1 < args.size()) {
                    file_path = args[i + 1];
                    continue;
                }
                if (args[i] == "-sp") {
                    save_preprocessed = true;
                    continue;
                }
                if (args[i] == "-st") {
                    save_token = true;
                    continue;
                }
                if (args[i] == "-run-time") {
                    interp_time = true;
                    continue;
                }
                if (args[i] == "-c") {
                    compile_mod = true;
                    continue;
                }

                if (args[i] == "-no-del") {
                    delete_precompiled = false;
                    continue;
                }
            }
        }
    }

    void FileIsExist() {
        if (file_path == "") {
            _FILE_NOT_SETTED();
            exit(1);
        }
        ifstream f(file_path.c_str());
        if (!f.good()) {
            _FILE_NOT_FOUND(file_path);
            exit(1);
        } else {
            if (!CheckFileSpecifier(file_path)) {
                size_t dot_pos = file_path.find_last_of(".");
                string specifier = file_path.substr(dot_pos);
                _FILE_INVALID_SPECIFIER(file_path, specifier);
                exit(1);
            }
            _FILE_FOUND(file_path);
        }
    }
};