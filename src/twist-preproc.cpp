#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cctype>
#include <cstring>
#include <stdexcept>

#pragma once

class Preprocessor {
private:
    struct Macro {
        std::vector<std::string> params;
        std::set<std::string> wildcard_params; // Параметры с * для нестрогой замены
        std::string body;
    };
    
    std::map<std::string, std::string> defines;
    std::map<std::string, Macro> macros;
    std::vector<std::string> includePaths;
    
    struct LineInfo {
        std::string content;
        bool isDirective;
        bool isIncludeMarker;
        bool isIncludeDirective;
        bool isMultilineMacro;
        std::string filename;
        int originalLine;
        int lineCount;  // Количество строк, занимаемых директивой
    };
    
    // Структура для хранения контекста ошибки
    struct ErrorContext {
        std::string filename;
        int line;
        std::string lineContent;
        std::string message;
        
        ErrorContext(const std::string& f, int l, const std::string& lc, const std::string& m)
            : filename(f), line(l), lineContent(lc), message(m) {}
    };
    
    // Функция для красивого вывода ошибки
    void printError(const ErrorContext& err) const {
        std::cerr << ".- [ err ] >> syntax >> '" << err.filename << "':" << err.line << std::endl;
        std::cerr << "|" << std::endl;
        
        // Добавляем пробелы для выравнивания номера строки
        std::string lineNumStr = std::to_string(err.line);
        int lineNumPadding = lineNumStr.length();
        
        std::cerr << "| " << err.line << " | " << err.lineContent << std::endl;
        
        // Вычисляем позицию для каретки (^)
        std::cerr << "| " << std::string(lineNumPadding, ' ') << "   " 
                  << std::string(err.lineContent.length(), ' ') << "^ " << err.message << std::endl;
        
        std::cerr << "`---------------------------'" << std::endl << std::endl;
    }
    
    // Функция для вывода ошибки без контекста строки
    void printSimpleError(const std::string& filename, const std::string& message) const {
        std::cerr << ".- [ err ] >> syntax >> '" << filename << "'" << std::endl;
        std::cerr << "|" << std::endl;
        std::cerr << "| " << message << std::endl;
        std::cerr << "`---------------------------'" << std::endl << std::endl;
    }
    
    static std::string trim(const std::string& s) {
        size_t start = 0, end = s.length() - 1;
        while (start <= end && std::isspace(s[start])) start++;
        while (end >= start && std::isspace(s[end])) end--;
        if (start > end) return "";
        return s.substr(start, end - start + 1);
    }
    
    static std::string trimRight(const std::string& s) {
        size_t end = s.length() - 1;
        while (end != (size_t)-1 && std::isspace(s[end])) end--;
        return s.substr(0, end + 1);
    }
    
    static bool startsWith(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
    }
    
    static void replaceWholeWord(std::string& str, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            bool isWordBoundary = true;
            
            if (pos > 0) {
                char before = str[pos - 1];
                if (std::isalnum(before) || before == '_') {
                    isWordBoundary = false;
                }
            }
            
            if (pos + from.length() < str.length()) {
                char after = str[pos + from.length()];
                if (std::isalnum(after) || after == '_') {
                    isWordBoundary = false;
                }
            }
            
            if (isWordBoundary) {
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
    
    std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::string findIncludeFile(const std::string& filename) {
        std::ifstream test(filename);
        if (test) {
            test.close();
            return filename;
        }
        
        for (const auto& path : includePaths) {
            std::string fullPath = path + "/" + filename;
            std::ifstream test2(fullPath);
            if (test2) {
                test2.close();
                return fullPath;
            }
        }
        
        printSimpleError(filename, "Include file not found: " + filename);
        throw std::runtime_error("Include file not found: " + filename);
    }
    
    void collectIncludes(const std::string& source, 
                    const std::string& filename,
                    int outerLine,
                    std::vector<LineInfo>& lines) {
    std::stringstream input(source);
    std::string line;
    int lineNum = 0;
    
    while (std::getline(input, line)) {
        lineNum++;
        std::string trimmed = trim(line);
        
        if (startsWith(trimmed, "#macro")) {
            std::string macroContent = line;
            bool hasOpeningBrace = (line.find('{') != std::string::npos);
            
            if (hasOpeningBrace) {
                int braceCount = 0;
                bool inQuotes = false;
                char quoteChar = '\0';
                
                // Подсчитываем скобки в первой строке
                for (char c : macroContent) {
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
                
                // Собираем последующие строки, пока не скомпенсируем все скобки
                while (braceCount > 0 && std::getline(input, line)) {
                    lineNum++;
                    macroContent += "\n" + line;
                    
                    for (char c : line) {
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
                    printError(ErrorContext(filename, lineNum, macroContent, 
                                           "Unbalanced braces in macro definition"));
                    throw std::runtime_error("Unbalanced braces in macro definition");
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
            // Подсчитываем количество строк в макросе
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
                
                std::string includeFile;
                try {
                    includeFile = extractIncludeFilename(line);
                } catch (const std::runtime_error& e) {
                    printError(ErrorContext(filename, lineNum, line, e.what()));
                    throw;
                }
                
                std::string fullPath = findIncludeFile(includeFile);
                
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
                
                std::string includedContent = readFile(fullPath);
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
    }
    
    std::string extractIncludeFilename(const std::string& line) {
        size_t start = line.find('"');
        if (start == std::string::npos) {
            throw std::runtime_error("Invalid #include syntax - missing opening quote");
        }
        size_t end = line.find('"', start + 1);
        if (end == std::string::npos) {
            throw std::runtime_error("Invalid #include syntax - missing closing quote");
        }
        return line.substr(start + 1, end - start - 1);
    }
    
    void processAllDirectives(const std::vector<LineInfo>& lines) {
        defines.clear();
        macros.clear();
        
        for (const auto& info : lines) {
            if (info.isDirective && !info.isIncludeDirective) {
                std::string trimmed = trim(info.content);
                if (startsWith(trimmed, "#define")) {
                    try {
                        processDefineDirective(info);
                    } catch (const std::runtime_error& e) {
                        printError(ErrorContext(info.filename, info.originalLine, 
                                               info.content, e.what()));
                        throw;
                    }
                } else if (startsWith(trimmed, "#macro")) {
                    try {
                        processMacroDirective(info);
                    } catch (const std::runtime_error& e) {
                        printError(ErrorContext(info.filename, info.originalLine, 
                                               info.content, e.what()));
                        throw;
                    }
                }
            }
        }
    }
    
    void processDefineDirective(const LineInfo& info) {
        const std::string& line = info.content;
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            throw std::runtime_error("Invalid #define syntax - missing '='");
        }
        
        std::string left = trim(line.substr(7, eqPos - 7));
        std::string right = trim(line.substr(eqPos + 1));
        
        if (left.empty()) {
            throw std::runtime_error("Invalid #define syntax - empty identifier");
        }
        
        for (char c : left) {
            if (!std::isalnum(c) && c != '_') {
                throw std::runtime_error("Invalid character in define name: '" + std::string(1, c) + "'");
            }
        }
        
        if (!left.empty()) {
            std::string value = right;
            
            bool changed;
            do {
                changed = false;
                std::string oldValue = value;
                
                for (const auto& def : defines) {
                    std::string temp = value;
                    replaceWholeWord(temp, def.first, def.second);
                    if (temp != value) {
                        value = temp;
                        changed = true;
                    }
                }
                
            } while (changed);
            
            defines[left] = value;
        }
    }
    
    void processMacroDirective(const LineInfo& info) {
        const std::string& line = info.content;
        
        size_t macroPos = line.find("#macro");
        if (macroPos == std::string::npos) {
            throw std::runtime_error("Invalid #macro directive");
        }
        
        size_t pos = macroPos + 6;
        while (pos < line.size() && std::isspace(line[pos])) pos++;
        
        size_t nameStart = pos;
        while (pos < line.size() && (std::isalnum(line[pos]) || line[pos] == '_')) pos++;
        
        if (pos == nameStart) {
            throw std::runtime_error("Missing macro name");
        }
        
        std::string name = line.substr(nameStart, pos - nameStart);
        
        for (char c : name) {
            if (!std::isalnum(c) && c != '_') {
                throw std::runtime_error("Invalid character in macro name: '" + std::string(1, c) + "'");
            }
        }
        
        while (pos < line.size() && std::isspace(line[pos])) pos++;
        if (pos >= line.size() || line[pos] != '(') {
            throw std::runtime_error("Missing '(' in macro definition");
        }
        
        size_t paramStart = pos + 1;
        int parenDepth = 1;
        pos++;
        
        while (pos < line.size() && parenDepth > 0) {
            if (line[pos] == '(') parenDepth++;
            else if (line[pos] == ')') parenDepth--;
            pos++;
        }
        
        if (parenDepth != 0) {
            throw std::runtime_error("Unmatched parentheses in macro definition");
        }
        
        std::string paramsStr = line.substr(paramStart, pos - paramStart - 1);
        std::vector<std::string> raw_params; // Параметры как есть (возможно с *)
        std::vector<std::string> processed_params; // Параметры без *
        std::set<std::string> wildcard_params; // Имена параметров с * (без звездочки)
        
        if (!paramsStr.empty()) {
            std::stringstream paramStream(paramsStr);
            std::string param;
            while (std::getline(paramStream, param, ',')) {
                std::string trimmedParam = trim(param);
                if (!trimmedParam.empty()) {
                    // Проверяем, является ли параметр "wildcard" параметром (начинается с *)
                    bool is_wildcard = false;
                    std::string param_name = trimmedParam;
                    
                    if (trimmedParam[0] == '*') {
                        is_wildcard = true;
                        param_name = trim(trimmedParam.substr(1));
                        
                        if (param_name.empty()) {
                            throw std::runtime_error("Invalid wildcard parameter - missing name after '*'");
                        }
                    }
                    
                    // Проверка на корректность имени параметра
                    for (char c : param_name) {
                        if (!std::isalnum(c) && c != '_') {
                            throw std::runtime_error("Invalid character in parameter name: '" + 
                                                   std::string(1, c) + "'");
                        }
                    }
                    
                    raw_params.push_back(trimmedParam);
                    processed_params.push_back(param_name);
                    
                    if (is_wildcard) {
                        wildcard_params.insert(param_name);
                    }
                }
            }
        }
        
        while (pos < line.size() && std::isspace(line[pos])) pos++;
        if (pos >= line.size() || line[pos] != '=') {
            throw std::runtime_error("Missing '=' in macro definition");
        }
        pos++;
        
        while (pos < line.size() && std::isspace(line[pos])) pos++;
        
        if (pos >= line.size()) {
            throw std::runtime_error("Missing macro body");
        }
        
        std::string body = line.substr(pos);
        
        // Обрабатываем тело макроса: подставляем дефайны
        std::string processedBody = body;
        for (const auto& def : defines) {
            replaceWholeWord(processedBody, def.first, def.second);
        }
        
        Macro macro;
        macro.params = processed_params; // Без звездочек
        macro.wildcard_params = wildcard_params; // Какие параметры являются wildcard
        macro.body = processedBody;
        macros[name] = macro;
    }
    
    std::string expandInString(const std::string& str, const LineInfo& context) {
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
                try {
                    std::string expanded = expandMacroCall(old, mac.first, mac.second, context);
                    if (expanded != old) {
                        result = expanded;
                        changed = true;
                        break;
                    }
                } catch (const std::runtime_error& e) {
                    printError(ErrorContext(context.filename, context.originalLine, 
                                           str, e.what()));
                    throw;
                }
            }
        } while (changed);
        
        return result;
    }
    
    std::string removeOuterBraces(const std::string& str) {
        std::string trimmed = trim(str);
        
        if (trimmed.size() >= 2 && trimmed[0] == '{' && trimmed[trimmed.size() - 1] == '}') {
            int braceCount = 0;
            bool inQuotes = false;
            char quoteChar = '\0';
            
            for (size_t i = 0; i < trimmed.size(); i++) {
                char c = trimmed[i];
                
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

    std::string expandMacroCall(const std::string& str, const std::string& name, 
                                const Macro& macro, const LineInfo& context) {
        std::string result = str;
        size_t pos = 0;
        
        while (pos < result.size()) {
            size_t namePos = result.find(name, pos);
            if (namePos == std::string::npos) break;
            
            bool isValid = true;
            
            if (namePos > 0) {
                char before = result[namePos - 1];
                if (std::isalnum(before) || before == '_') {
                    isValid = false;
                }
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
                throw std::runtime_error("Unmatched parentheses in macro call to '" + name + "'");
            }
            
            argEnd--;
            
            std::string argsStr = result.substr(argStart, argEnd - argStart);
            std::vector<std::string> args;
            
            try {
                splitArguments(argsStr, args);
            } catch (const std::runtime_error& e) {
                throw std::runtime_error("Error parsing arguments for macro '" + name + "': " + e.what());
            }
            
            if (args.size() != macro.params.size()) {
                std::string msg = "Wrong number of arguments for macro '" + name + "' - expected " +
                                 std::to_string(macro.params.size()) + ", got " + std::to_string(args.size());
                throw std::runtime_error(msg);
            }
            
            // Разворачиваем аргументы (подставляем дефайны)
            for (size_t i = 0; i < args.size(); i++) {
                LineInfo argContext = context;
                argContext.content = args[i];
                args[i] = expandInString(args[i], argContext);
            }
            
            std::string expanded = macro.body;
            
            // Подставляем аргументы в тело макроса
            for (size_t i = 0; i < args.size(); i++) {
                const std::string& param_name = macro.params[i];
                const std::string& arg_value = args[i];
                
                // Если параметр помечен как wildcard (*), используем replaceAll
                if (macro.wildcard_params.find(param_name) != macro.wildcard_params.end()) {
                    replaceAll(expanded, param_name, arg_value);
                } else {
                    // Иначе используем replaceWholeWord для строгой замены
                    replaceWholeWord(expanded, param_name, arg_value);
                }
            }
            
            // Удаляем внешние фигурные скобки, если они есть
            expanded = removeOuterBraces(expanded);
            
            result.replace(namePos, argEnd - namePos + 1, expanded);
            pos = namePos + expanded.size();
        }
        
        return result;
    }
    
    void splitArguments(const std::string& argsStr, std::vector<std::string>& args) {
        args.clear();
        std::string current;
        int parenDepth = 0;
        int braceDepth = 0;
        bool inQuotes = false;
        char quoteChar = '\0';
        
        for (size_t i = 0; i < argsStr.size(); i++) {
            char c = argsStr[i];
            
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
                        throw std::runtime_error("Unmatched ')' in arguments");
                    }
                    parenDepth--;
                    current += c;
                } else if (c == '{') {
                    braceDepth++;
                    current += c;
                } else if (c == '}') {
                    if (braceDepth == 0) {
                        throw std::runtime_error("Unmatched '}' in arguments");
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
            throw std::runtime_error("Unmatched '(' in arguments");
        }
        
        if (braceDepth != 0) {
            throw std::runtime_error("Unmatched '{' in arguments");
        }
        
        if (inQuotes) {
            throw std::runtime_error("Unclosed string literal in arguments");
        }
        
        if (!current.empty()) {
            args.push_back(trim(current));
        }
    }
    
    std::string processDirectives(std::vector<LineInfo>& lines) {
    std::stringstream output;
    
    try {
        processAllDirectives(lines);
    } catch (const std::runtime_error& e) {
        throw;
    }
    
    for (size_t i = 0; i < lines.size(); i++) {
        const LineInfo& info = lines[i];
        
        if (info.isIncludeMarker) {
            output << info.content << "\n";
        } else if (info.isDirective) {
            if (info.isIncludeDirective) {
                continue;
            } else {
                // Для директив макросов (@macro) заменяем на соответствующее количество пустых строк
                if (startsWith(trim(info.content), "#macro")) {
                    // Выводим пустые строки вместо декларации макроса
                    for (int j = 0; j < info.lineCount; j++) {
                        output << "\n";
                    }
                } else {
                    // Для обычных директив (@define) оставляем одну пустую строку
                    output << "\n";
                }
            }
        } else {
            try {
                std::string processed = expandInString(info.content, info);
                output << processed << "\n";
            } catch (const std::runtime_error& e) {
                throw;
            }
        }
    }
    
    return output.str();
}
    
public:
    Preprocessor() {
        includePaths.push_back(".");
        includePaths.push_back("include");
    }
    
    void addIncludePath(const std::string& path) {
        includePaths.push_back(path);
    }
    
    std::string process(const std::string& source, const std::string& filename) {
        std::vector<LineInfo> lines;
        
        try {
            collectIncludes(source, filename, 1, lines);
        } catch (const std::runtime_error& e) {
            throw;
        }
        
        try {
            return processDirectives(lines);
        } catch (const std::runtime_error& e) {
            throw;
        }
    }
};