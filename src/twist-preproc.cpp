#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <cctype>

#pragma once

// Подключаем существующую систему ошибок
#include "twist-tokens.cpp"
#include "twist-utils.cpp"
#include "twist-err.cpp"

// Расширяем ErrorTypes для препроцессора
namespace ErrorTypes {
    const string PREPROCESSOR = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::BLUE + "preprocessor" + TERMINAL_COLORS::RESET;
    const string MACRO = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::YELLOW + "macro" + TERMINAL_COLORS::RESET;
    const string INCLUDE = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::GREEN + "include" + TERMINAL_COLORS::RESET;
}

// Класс для ошибок препроцессора
class PreprocessorError : public Error {
public:
    PreprocessorError(const string& message, const PosInFile& pif, const ErrorType& type, const string& code)
        : Error(message, pif, type, code) {}
    
    PreprocessorError(const string& message, const PosInFile& start, const PosInFile& end, 
                     const ErrorType& type, const string& code)
        : Error(message, start, end, type, code) {}
};

// Класс для представления позиции в исходном коде препроцессора
struct PreprocessorPos {
    string filename;
    int line;
    int column;
    string line_content;
    
    PreprocessorPos() : line(0), column(0) {}
    PreprocessorPos(const string& f, int l, int c, const string& content) 
        : filename(f), line(l), column(c), line_content(content) {}
    
    PosInFile toPosInFile() const {
        PosInFile pif;
        pif.file_path = filename;
        pif.file_name = filename;
        pif.global_line = line;
        pif.line = line;
        pif.index = column;
        pif.lenght = 1;
        return pif;
    }
};

// Структура для макроса с расширенной информацией
struct Macro {
    std::string name;
    std::vector<std::string> params;
    std::set<std::string> wildcard_params;
    std::string body;
    PreprocessorPos definition_pos;
    bool is_processed = false;
    int use_count = 0;
};

// Класс для кэширования включенных файлов
class IncludeCache {
private:
    std::unordered_map<std::string, std::string> cache;
    std::set<std::string> included_files;
    
public:
    bool isIncluded(const std::string& filename) const {
        return included_files.find(filename) != included_files.end();
    }
    
    void markIncluded(const std::string& filename) {
        included_files.insert(filename);
    }
    
    void addToCache(const std::string& filename, const std::string& content) {
        cache[filename] = content;
    }
    
    std::string getFromCache(const std::string& filename) const {
        auto it = cache.find(filename);
        return it != cache.end() ? it->second : "";
    }
    
    void clear() {
        cache.clear();
        included_files.clear();
    }
};

// Основной класс препроцессора
class Preprocessor {
private:
    // Конфигурация
    struct Config {
        int max_recursion_depth = 100;
        int max_macro_expansion = 10000;
        bool enable_caching = true;
        std::vector<std::string> include_paths;
        
        Config() {
            include_paths.push_back(".");
            include_paths.push_back("include");
        }
    };
    
    Config config;
    IncludeCache include_cache;
    
    // Хранилище макросов и определений
    std::unordered_map<std::string, std::string> defines;
    std::unordered_map<std::string, Macro> macros;
    
    // Для отслеживания рекурсии
    std::set<std::string> expanding_macros;
    std::map<std::string, int> macro_expansion_count;
    
    // Контекст обработки
    struct ProcessingContext {
        std::string current_filename;
        std::vector<PreprocessorPos> include_stack;
        int current_line = 0;
        
        void pushFile(const std::string& filename, int line) {
            include_stack.push_back({filename, line, 0, ""});
            current_filename = filename;
        }
        
        void popFile() {
            if (!include_stack.empty()) {
                include_stack.pop_back();
                current_filename = include_stack.empty() ? "" : include_stack.back().filename;
            }
        }
        
        PreprocessorPos getCurrentPos(int line, int column, const std::string& content) const {
            return PreprocessorPos(current_filename, line, column, content);
        }
    } context;
    
    // Вспомогательные методы
    static std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(s[start])) start++;
        size_t end = s.size();
        while (end > start && std::isspace(s[end - 1])) end--;
        return s.substr(start, end - start);
    }
    
    static std::string trimRight(const std::string& s) {
        size_t end = s.size();
        while (end > 0 && std::isspace(s[end - 1])) end--;
        return s.substr(0, end);
    }
    
    static bool startsWith(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
    }
    
    // Проверка на границы слова
    static bool isWordBoundary(char c) {
        return !std::isalnum(c) && c != '_';
    }
    
    // Безопасная замена с учетом границ слова
    static void replaceWholeWord(std::string& str, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            bool isBoundary = true;
            
            if (pos > 0 && !isWordBoundary(str[pos - 1])) {
                isBoundary = false;
            }
            
            if (pos + from.length() < str.length() && !isWordBoundary(str[pos + from.length()])) {
                isBoundary = false;
            }
            
            if (isBoundary) {
                str.replace(pos, from.length(), to);
                pos += to.length();
            } else {
                pos += from.length();
            }
        }
    }
    
    static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.size(), to);
            pos += to.size();
        }
    }
    
    // Чтение файла с обработкой ошибок
    std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw PreprocessorError(
                "Cannot open file: " + filename,
                PreprocessorPos(filename, 0, 0, "").toPosInFile(),
                ErrorTypes::INCLUDE,
                ""
            );
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    // Поиск файла include
    std::string findIncludeFile(const std::string& filename) {
        // Проверяем абсолютный путь
        std::ifstream test(filename);
        if (test) {
            test.close();
            return filename;
        }
        
        // Проверяем в путях поиска
        for (const auto& path : config.include_paths) {
            std::string fullPath = path + "/" + filename;
            std::ifstream test2(fullPath);
            if (test2) {
                test2.close();
                return fullPath;
            }
        }
        
        throw PreprocessorError(
            "Include file not found: " + filename,
            PreprocessorPos(context.current_filename, context.current_line, 0, "").toPosInFile(),
            ErrorTypes::INCLUDE,
            ""
        );
    }
    
    // Извлечение имени файла из #include
    std::string extractIncludeFilename(const std::string& line, int line_num) {
        size_t start = line.find('"');
        if (start == std::string::npos) {
            throw PreprocessorError(
                "Invalid #include syntax - missing opening quote",
                PreprocessorPos(context.current_filename, line_num, 0, line).toPosInFile(),
                ErrorTypes::SYNTAX,
                line
            );
        }
        size_t end = line.find('"', start + 1);
        if (end == std::string::npos) {
            throw PreprocessorError(
                "Invalid #include syntax - missing closing quote",
                PreprocessorPos(context.current_filename, line_num, 0, line).toPosInFile(),
                ErrorTypes::SYNTAX,
                line
            );
        }
        return line.substr(start + 1, end - start - 1);
    }
    
    // Парсинг определения макроса
    void processMacroDefinition(const std::string& macro_content, int line_num) {
        size_t macroPos = macro_content.find("#macro");
        if (macroPos == std::string::npos) {
            throw PreprocessorError(
                "Invalid #macro directive",
                PreprocessorPos(context.current_filename, line_num, 0, macro_content).toPosInFile(),
                ErrorTypes::MACRO,
                macro_content
            );
        }
        
        size_t pos = macroPos + 6;
        while (pos < macro_content.size() && std::isspace(macro_content[pos])) pos++;
        
        size_t nameStart = pos;
        while (pos < macro_content.size() && (std::isalnum(macro_content[pos]) || macro_content[pos] == '_')) pos++;
        
        if (pos == nameStart) {
            throw PreprocessorError(
                "Missing macro name",
                PreprocessorPos(context.current_filename, line_num, nameStart, macro_content).toPosInFile(),
                ErrorTypes::MACRO,
                macro_content
            );
        }
        
        std::string name = macro_content.substr(nameStart, pos - nameStart);
        
        // Проверка имени
        for (char c : name) {
            if (!std::isalnum(c) && c != '_') {
                throw PreprocessorError(
                    "Invalid character in macro name: '" + std::string(1, c) + "'",
                    PreprocessorPos(context.current_filename, line_num, nameStart, macro_content).toPosInFile(),
                    ErrorTypes::MACRO,
                    macro_content
                );
            }
        }
        
        // Парсинг параметров
        while (pos < macro_content.size() && std::isspace(macro_content[pos])) pos++;
        if (pos >= macro_content.size() || macro_content[pos] != '(') {
            throw PreprocessorError(
                "Missing '(' in macro definition",
                PreprocessorPos(context.current_filename, line_num, pos, macro_content).toPosInFile(),
                ErrorTypes::MACRO,
                macro_content
            );
        }
        
        size_t paramStart = pos + 1;
        int parenDepth = 1;
        pos++;
        
        while (pos < macro_content.size() && parenDepth > 0) {
            if (macro_content[pos] == '(') parenDepth++;
            else if (macro_content[pos] == ')') parenDepth--;
            pos++;
        }
        
        if (parenDepth != 0) {
            throw PreprocessorError(
                "Unmatched parentheses in macro definition",
                PreprocessorPos(context.current_filename, line_num, paramStart - 1, macro_content).toPosInFile(),
                ErrorTypes::MACRO,
                macro_content
            );
        }
        
        std::string paramsStr = macro_content.substr(paramStart, pos - paramStart - 1);
        std::vector<std::string> processed_params;
        std::set<std::string> wildcard_params;
        
        // Парсинг параметров
        if (!paramsStr.empty()) {
            std::stringstream paramStream(paramsStr);
            std::string param;
            while (std::getline(paramStream, param, ',')) {
                std::string trimmedParam = trim(param);
                if (!trimmedParam.empty()) {
                    bool is_wildcard = false;
                    std::string param_name = trimmedParam;
                    
                    if (trimmedParam[0] == '*') {
                        is_wildcard = true;
                        param_name = trim(trimmedParam.substr(1));
                        
                        if (param_name.empty()) {
                            throw PreprocessorError(
                                "Invalid wildcard parameter - missing name after '*'",
                                PreprocessorPos(context.current_filename, line_num, 0, macro_content).toPosInFile(),
                                ErrorTypes::MACRO,
                                macro_content
                            );
                        }
                    }
                    
                    // Проверка имени параметра
                    for (char c : param_name) {
                        if (!std::isalnum(c) && c != '_') {
                            throw PreprocessorError(
                                "Invalid character in parameter name: '" + std::string(1, c) + "'",
                                PreprocessorPos(context.current_filename, line_num, 0, macro_content).toPosInFile(),
                                ErrorTypes::MACRO,
                                macro_content
                            );
                        }
                    }
                    
                    processed_params.push_back(param_name);
                    if (is_wildcard) {
                        wildcard_params.insert(param_name);
                    }
                }
            }
        }
        
        // Поиск тела макроса
        while (pos < macro_content.size() && std::isspace(macro_content[pos])) pos++;
        if (pos >= macro_content.size() || macro_content[pos] != '=') {
            throw PreprocessorError(
                "Missing '=' in macro definition",
                PreprocessorPos(context.current_filename, line_num, pos, macro_content).toPosInFile(),
                ErrorTypes::MACRO,
                macro_content
            );
        }
        pos++;
        
        while (pos < macro_content.size() && std::isspace(macro_content[pos])) pos++;
        
        if (pos >= macro_content.size()) {
            throw PreprocessorError(
                "Missing macro body",
                PreprocessorPos(context.current_filename, line_num, pos, macro_content).toPosInFile(),
                ErrorTypes::MACRO,
                macro_content
            );
        }
        
        std::string body = macro_content.substr(pos);
        
        // Предварительная обработка тела макроса (подстановка define)
        std::string processedBody = body;
        for (const auto& def : defines) {
            replaceWholeWord(processedBody, def.first, def.second);
        }
        
        Macro macro;
        macro.name = name;
        macro.params = processed_params;
        macro.wildcard_params = wildcard_params;
        macro.body = processedBody;
        macro.definition_pos = PreprocessorPos(context.current_filename, line_num, nameStart, macro_content);
        
        macros[name] = macro;
    }
    
    // Разбор аргументов макроса
    void splitArguments(const std::string& argsStr, std::vector<std::string>& args, int line_num) {
        args.clear();
        std::string current;
        int parenDepth = 0;
        int braceDepth = 0;
        bool inQuotes = false;
        char quoteChar = '\0';
        bool escape = false;
        
        for (size_t i = 0; i < argsStr.size(); i++) {
            char c = argsStr[i];
            
            if (escape) {
                current += c;
                escape = false;
                continue;
            }
            
            if (c == '\\' && inQuotes) {
                current += c;
                escape = true;
                continue;
            }
            
            if (!inQuotes && (c == '\'' || c == '"')) {
                inQuotes = true;
                quoteChar = c;
                current += c;
            } else if (inQuotes && c == quoteChar) {
                inQuotes = false;
                current += c;
            } else if (!inQuotes) {
                if (c == '(') {
                    parenDepth++;
                    current += c;
                } else if (c == ')') {
                    if (parenDepth == 0) {
                        throw PreprocessorError(
                            "Unmatched ')' in arguments",
                            PreprocessorPos(context.current_filename, line_num, i, argsStr).toPosInFile(),
                            ErrorTypes::MACRO,
                            argsStr
                        );
                    }
                    parenDepth--;
                    current += c;
                } else if (c == '{') {
                    braceDepth++;
                    current += c;
                } else if (c == '}') {
                    if (braceDepth == 0) {
                        throw PreprocessorError(
                            "Unmatched '}' in arguments",
                            PreprocessorPos(context.current_filename, line_num, i, argsStr).toPosInFile(),
                            ErrorTypes::MACRO,
                            argsStr
                        );
                    }
                    braceDepth--;
                    current += c;
                } else if (c == ',' && parenDepth == 0 && braceDepth == 0) {
                    args.push_back(trim(current));
                    current.clear();
                } else {
                    current += c;
                }
            } else {
                current += c;
            }
        }
        
        if (parenDepth != 0) {
            throw PreprocessorError(
                "Unmatched '(' in arguments",
                PreprocessorPos(context.current_filename, line_num, 0, argsStr).toPosInFile(),
                ErrorTypes::MACRO,
                argsStr
            );
        }
        
        if (braceDepth != 0) {
            throw PreprocessorError(
                "Unmatched '{' in arguments",
                PreprocessorPos(context.current_filename, line_num, 0, argsStr).toPosInFile(),
                ErrorTypes::MACRO,
                argsStr
            );
        }
        
        if (inQuotes) {
            throw PreprocessorError(
                "Unclosed string literal in arguments",
                PreprocessorPos(context.current_filename, line_num, 0, argsStr).toPosInFile(),
                ErrorTypes::MACRO,
                argsStr
            );
        }
        
        if (!current.empty()) {
            args.push_back(trim(current));
        }
    }
    
    // Удаление внешних скобок
    std::string removeOuterBraces(const std::string& str) {
        std::string trimmed = trim(str);
        
        if (trimmed.size() >= 2 && trimmed[0] == '{' && trimmed[trimmed.size() - 1] == '}') {
            int braceCount = 0;
            bool inQuotes = false;
            char quoteChar = '\0';
            bool escape = false;
            
            for (size_t i = 0; i < trimmed.size(); i++) {
                char c = trimmed[i];
                
                if (escape) {
                    escape = false;
                    continue;
                }
                
                if (c == '\\' && inQuotes) {
                    escape = true;
                    continue;
                }
                
                if (!inQuotes && (c == '\'' || c == '"')) {
                    inQuotes = !inQuotes;
                    quoteChar = c;
                } else if (inQuotes && c == quoteChar) {
                    inQuotes = false;
                } else if (!inQuotes) {
                    if (c == '{') {
                        braceCount++;
                        if (braceCount == 1 && i == 0) {
                            continue;
                        }
                    } else if (c == '}') {
                        braceCount--;
                        if (braceCount == 0 && i == trimmed.size() - 1) {
                            std::string inner = trimmed.substr(1, trimmed.size() - 2);
                            return trim(inner);
                        }
                    }
                }
            }
        }
        
        return str;
    }
    
    // Расширение макроса
    std::string expandMacro(const std::string& str, const std::string& name, 
                           const Macro& macro, int line_num, int recursion_depth) {
        if (recursion_depth > config.max_recursion_depth) {
            throw PreprocessorError(
                "Maximum macro expansion recursion depth exceeded: " + name,
                macro.definition_pos.toPosInFile(),
                ErrorTypes::MACRO,
                ""
            );
        }
        
        // Проверка на циклическое расширение
        if (expanding_macros.find(name) != expanding_macros.end()) {
            throw PreprocessorError(
                "Circular macro expansion detected: " + name,
                macro.definition_pos.toPosInFile(),
                ErrorTypes::MACRO,
                ""
            );
        }
        
        // Проверка на количество расширений
        macro_expansion_count[name]++;
        if (macro_expansion_count[name] > config.max_macro_expansion) {
            throw PreprocessorError(
                "Maximum macro expansions exceeded for: " + name,
                macro.definition_pos.toPosInFile(),
                ErrorTypes::MACRO,
                ""
            );
        }
        
        expanding_macros.insert(name);
        
        std::string result = str;
        size_t pos = 0;
        
        while (pos < result.size()) {
            size_t namePos = result.find(name, pos);
            if (namePos == std::string::npos) break;
            
            bool isValid = true;
            
            if (namePos > 0 && !isWordBoundary(result[namePos - 1])) {
                isValid = false;
            }
            
            size_t afterName = namePos + name.size();
            if (afterName >= result.size() || result[afterName] != '(') {
                isValid = false;
            }
            
            if (!isValid) {
                pos = namePos + 1;
                continue;
            }
            
            int parenDepth = 1;
            size_t argStart = afterName + 1;
            size_t argEnd = argStart;
            
            while (argEnd < result.size() && parenDepth > 0) {
                if (result[argEnd] == '(') parenDepth++;
                else if (result[argEnd] == ')') parenDepth--;
                argEnd++;
            }
            
            if (parenDepth != 0) {
                throw PreprocessorError(
                    "Unmatched parentheses in macro call to '" + name + "'",
                    PreprocessorPos(context.current_filename, line_num, namePos, result).toPosInFile(),
                    ErrorTypes::MACRO,
                    result
                );
            }
            
            argEnd--;
            
            std::string argsStr = result.substr(argStart, argEnd - argStart);
            std::vector<std::string> args;
            
            splitArguments(argsStr, args, line_num);
            
            if (args.size() != macro.params.size()) {
                throw PreprocessorError(
                    "Wrong number of arguments for macro '" + name + "' - expected " +
                    std::to_string(macro.params.size()) + ", got " + std::to_string(args.size()),
                    PreprocessorPos(context.current_filename, line_num, namePos, result).toPosInFile(),
                    ErrorTypes::MACRO,
                    result
                );
            }
            
            // Расширение аргументов
            for (size_t i = 0; i < args.size(); i++) {
                args[i] = expandInString(args[i], line_num, recursion_depth + 1);
            }
            
            std::string expanded = macro.body;
            
            // Подстановка аргументов
            for (size_t i = 0; i < args.size(); i++) {
                const std::string& param_name = macro.params[i];
                const std::string& arg_value = args[i];
                
                if (macro.wildcard_params.find(param_name) != macro.wildcard_params.end()) {
                    replaceAll(expanded, param_name, arg_value);
                } else {
                    replaceWholeWord(expanded, param_name, arg_value);
                }
            }
            
            expanded = removeOuterBraces(expanded);
            
            result.replace(namePos, argEnd - namePos + 1, expanded);
            pos = namePos + expanded.size();
        }
        
        expanding_macros.erase(name);
        return result;
    }
    
    // Расширение в строке
    std::string expandInString(const std::string& str, int line_num, int recursion_depth = 0) {
        if (recursion_depth > config.max_recursion_depth) {
            throw PreprocessorError(
                "Maximum expansion recursion depth exceeded",
                PreprocessorPos(context.current_filename, line_num, 0, str).toPosInFile(),
                ErrorTypes::MACRO,
                str
            );
        }
        
        std::string result = str;
        
        // Сначала подставляем дефайны
        for (const auto& def : defines) {
            replaceWholeWord(result, def.first, def.second);
        }
        
        // Затем подставляем макросы
        bool changed;
        do {
            changed = false;
            std::string old = result;
            
            for (const auto& mac : macros) {
                std::string expanded = expandMacro(old, mac.first, mac.second, line_num, recursion_depth);
                if (expanded != old) {
                    result = expanded;
                    changed = true;
                    break;
                }
            }
        } while (changed);
        
        return result;
    }
    
    // Обработка define директивы
    void processDefine(const std::string& line, int line_num) {
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            throw PreprocessorError(
                "Invalid #define syntax - missing '='",
                PreprocessorPos(context.current_filename, line_num, 0, line).toPosInFile(),
                ErrorTypes::SYNTAX,
                line
            );
        }
        
        std::string left = trim(line.substr(7, eqPos - 7));
        std::string right = trim(line.substr(eqPos + 1));
        
        if (left.empty()) {
            throw PreprocessorError(
                "Invalid #define syntax - empty identifier",
                PreprocessorPos(context.current_filename, line_num, 0, line).toPosInFile(),
                ErrorTypes::SYNTAX,
                line
            );
        }
        
        for (char c : left) {
            if (!std::isalnum(c) && c != '_') {
                throw PreprocessorError(
                    "Invalid character in define name: '" + std::string(1, c) + "'",
                    PreprocessorPos(context.current_filename, line_num, 0, line).toPosInFile(),
                    ErrorTypes::SYNTAX,
                    line
                );
            }
        }
        
        // Расширение значения
        std::string value = expandInString(right, line_num, 0);
        
        defines[left] = value;
    }
    
    // Сбор информации о строках и директивах
    struct LineInfo {
        std::string content;
        bool isDirective;
        bool isIncludeMarker;
        bool isIncludeDirective;
        bool isMultilineMacro;
        std::string filename;
        int originalLine;
        int lineCount;
    };
    
    void collectIncludes(const std::string& source, const std::string& filename,
                        int outerLine, std::vector<LineInfo>& lines) {
        std::stringstream input(source);
        std::string line;
        int lineNum = 0;
        
        context.pushFile(filename, outerLine);
        
        while (std::getline(input, line)) {
            lineNum++;
            context.current_line = lineNum;
            std::string trimmed = trim(line);
            
            if (startsWith(trimmed, "#macro")) {
                std::string macroContent = line;
                bool hasOpeningBrace = (line.find('{') != std::string::npos);
                
                if (hasOpeningBrace) {
                    int braceCount = 0;
                    bool inQuotes = false;
                    char quoteChar = '\0';
                    bool escape = false;
                    
                    for (char c : macroContent) {
                        if (escape) {
                            escape = false;
                            continue;
                        }
                        if (c == '\\' && inQuotes) {
                            escape = true;
                            continue;
                        }
                        if (!inQuotes && (c == '\'' || c == '"')) {
                            inQuotes = !inQuotes;
                            quoteChar = c;
                        } else if (inQuotes && c == quoteChar) {
                            inQuotes = false;
                        } else if (!inQuotes) {
                            if (c == '{') braceCount++;
                            else if (c == '}') braceCount--;
                        }
                    }
                    
                    while (braceCount > 0 && std::getline(input, line)) {
                        lineNum++;
                        macroContent += "\n" + line;
                        
                        for (char c : line) {
                            if (escape) {
                                escape = false;
                                continue;
                            }
                            if (c == '\\' && inQuotes) {
                                escape = true;
                                continue;
                            }
                            if (!inQuotes && (c == '\'' || c == '"')) {
                                inQuotes = !inQuotes;
                                quoteChar = c;
                            } else if (inQuotes && c == quoteChar) {
                                inQuotes = false;
                            } else if (!inQuotes) {
                                if (c == '{') braceCount++;
                                else if (c == '}') braceCount--;
                            }
                        }
                    }
                    
                    if (braceCount > 0) {
                        throw PreprocessorError(
                            "Unbalanced braces in macro definition",
                            PreprocessorPos(filename, lineNum, 0, macroContent).toPosInFile(),
                            ErrorTypes::MACRO,
                            macroContent
                        );
                    }
                }
                
                LineInfo info;
                info.content = macroContent;
                info.isDirective = true;
                info.isIncludeMarker = false;
                info.isIncludeDirective = false;
                info.isMultilineMacro = true;
                info.filename = filename;
                info.originalLine = lineNum;
                info.lineCount = std::count(macroContent.begin(), macroContent.end(), '\n') + 1;
                lines.push_back(info);
                
            } else if (startsWith(trimmed, "#include")) {
                int includeLine = outerLine + lineNum - 1;
                
                LineInfo info;
                info.content = line;
                info.isDirective = true;
                info.isIncludeMarker = false;
                info.isIncludeDirective = true;
                info.isMultilineMacro = false;
                info.filename = filename;
                info.originalLine = lineNum;
                info.lineCount = 1;
                lines.push_back(info);
                
                std::string includeFile = extractIncludeFilename(line, lineNum);
                std::string fullPath = findIncludeFile(includeFile);
                
                // Проверка на циклическое включение
                if (include_cache.isIncluded(fullPath)) {
                    throw PreprocessorError(
                        "Circular include detected: " + fullPath,
                        PreprocessorPos(filename, lineNum, 0, line).toPosInFile(),
                        ErrorTypes::INCLUDE,
                        line
                    );
                }
                
                LineInfo startMarker;
                startMarker.content = "<start file=\"" + fullPath + "\" outer=" + 
                                     std::to_string(includeLine) + ">";
                startMarker.isDirective = false;
                startMarker.isIncludeMarker = true;
                startMarker.isIncludeDirective = false;
                startMarker.isMultilineMacro = false;
                startMarker.filename = filename;
                startMarker.originalLine = lineNum;
                startMarker.lineCount = 1;
                lines.push_back(startMarker);
                
                std::string includedContent;
                if (config.enable_caching) {
                    includedContent = include_cache.getFromCache(fullPath);
                    if (includedContent.empty()) {
                        includedContent = readFile(fullPath);
                        include_cache.addToCache(fullPath, includedContent);
                    }
                } else {
                    includedContent = readFile(fullPath);
                }
                
                include_cache.markIncluded(fullPath);
                collectIncludes(includedContent, fullPath, outerLine + lineNum - 1, lines);
                
                LineInfo endMarker;
                endMarker.content = "<end file=\"" + filename + "\" outer=" + 
                                   std::to_string(includeLine) + ">";
                endMarker.isDirective = false;
                endMarker.isIncludeMarker = true;
                endMarker.isIncludeDirective = false;
                endMarker.isMultilineMacro = false;
                endMarker.filename = filename;
                endMarker.originalLine = lineNum;
                endMarker.lineCount = 1;
                lines.push_back(endMarker);
                
            } else if (startsWith(trimmed, "#define")) {
                LineInfo info;
                info.content = line;
                info.isDirective = true;
                info.isIncludeMarker = false;
                info.isIncludeDirective = false;
                info.isMultilineMacro = false;
                info.filename = filename;
                info.originalLine = lineNum;
                info.lineCount = 1;
                lines.push_back(info);
                
            } else {
                LineInfo info;
                info.content = line;
                info.isDirective = false;
                info.isIncludeMarker = false;
                info.isIncludeDirective = false;
                info.isMultilineMacro = false;
                info.filename = filename;
                info.originalLine = lineNum;
                info.lineCount = 1;
                lines.push_back(info);
            }
        }
        
        context.popFile();
    }
    
    void processAllDirectives(const std::vector<LineInfo>& lines) {
        defines.clear();
        macros.clear();
        macro_expansion_count.clear();
        
        for (const auto& info : lines) {
            if (info.isDirective && !info.isIncludeDirective) {
                std::string trimmed = trim(info.content);
                if (startsWith(trimmed, "#define")) {
                    processDefine(info.content, info.originalLine);
                } else if (startsWith(trimmed, "#macro")) {
                    processMacroDefinition(info.content, info.originalLine);
                }
            }
        }
    }
    
    std::string processDirectives(std::vector<LineInfo>& lines) {
        std::stringstream output;
        
        processAllDirectives(lines);
        
        for (size_t i = 0; i < lines.size(); i++) {
            const LineInfo& info = lines[i];
            
            if (info.isIncludeMarker) {
                output << info.content << "\n";
            } else if (info.isDirective) {
                if (info.isIncludeDirective) {
                    continue;
                } else {
                    if (startsWith(trim(info.content), "#macro")) {
                        for (int j = 0; j < info.lineCount; j++) {
                            output << "\n";
                        }
                    } else {
                        output << "\n";
                    }
                }
            } else {
                std::string processed = expandInString(info.content, info.originalLine);
                output << processed << "\n";
            }
        }
        
        return output.str();
    }
    
public:
    Preprocessor() {
        config.include_paths.push_back(".");
        config.include_paths.push_back("include");
        config.enable_caching = true;
        config.max_recursion_depth = 100;
        config.max_macro_expansion = 10000;
    }
    
    void addIncludePath(const std::string& path) {
        config.include_paths.push_back(path);
    }
    
    void setMaxRecursionDepth(int depth) {
        config.max_recursion_depth = depth;
    }
    
    void setMaxMacroExpansion(int max) {
        config.max_macro_expansion = max;
    }
    
    void enableCaching(bool enable) {
        config.enable_caching = enable;
    }
    
    void clearCache() {
        include_cache.clear();
    }
    
    std::string process(const std::string& source, const std::string& filename) {
        std::vector<LineInfo> lines;
        include_cache.clear();
        
        collectIncludes(source, filename, 1, lines);
        
        return processDirectives(lines);
    }
};