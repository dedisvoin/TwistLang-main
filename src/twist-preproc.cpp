#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstring>
#include <stdexcept>

#pragma once

class Preprocessor {
private:
    struct Macro {
        std::vector<std::string> params;
        std::string body;
    };
    
    std::map<std::string, std::string> defines;
    std::map<std::string, Macro> macros;
    std::vector<std::string> includePaths;
    
    struct LineInfo {
        std::string content;
        bool isDirective;      // @define или @macro
        bool isIncludeMarker;  // Маркер include (<start ...> или <end ...>)
        bool isIncludeDirective; // Строка с @include (должна быть удалена)
        std::string filename;
        int originalLine;
    };
    
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
    
    // НОВАЯ ФУНКЦИЯ: Заменяет только целые слова
    static void replaceWholeWord(std::string& str, const std::string& from, const std::string& to) {
        if (from.empty()) return;
        
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            // Проверяем границы слова
            bool isWordBoundary = true;
            
            // Проверяем символ перед словом
            if (pos > 0) {
                char before = str[pos - 1];
                if (std::isalnum(before) || before == '_') {
                    isWordBoundary = false;
                }
            }
            
            // Проверяем символ после слова
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
    
    // Старая функция для полной замены (используется в других местах)
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
        if (!file) throw std::runtime_error("Cannot open file: " + filename);
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
            
            LineInfo info;
            info.content = trimRight(line);
            info.filename = filename;
            info.originalLine = lineNum;
            
            if (startsWith(trimmed, "@include")) {
                int includeLine = outerLine + lineNum - 1;
                
                info.isDirective = true;
                info.isIncludeMarker = false;
                info.isIncludeDirective = true;
                lines.push_back(info);
                
                std::string includeFile = extractIncludeFilename(line);
                std::string fullPath = findIncludeFile(includeFile);
                
                LineInfo startMarker;
                startMarker.content = "<start file=\"" + fullPath + "\" outer=" + 
                                     std::to_string(includeLine) + ">";
                startMarker.isDirective = false;
                startMarker.isIncludeMarker = true;
                startMarker.isIncludeDirective = false;
                startMarker.filename = filename;
                startMarker.originalLine = lineNum;
                lines.push_back(startMarker);
                
                std::string includedContent = readFile(fullPath);
                collectIncludes(includedContent, fullPath, outerLine + lineNum - 1, lines);
                
                LineInfo endMarker;
                endMarker.content = "<end file=\"" + filename + "\" outer=" + 
                                   std::to_string(includeLine) + ">";
                endMarker.isDirective = false;
                endMarker.isIncludeMarker = true;
                endMarker.isIncludeDirective = false;
                endMarker.filename = filename;
                endMarker.originalLine = lineNum;
                lines.push_back(endMarker);
                
            } else if (startsWith(trimmed, "@define") || startsWith(trimmed, "@macro")) {
                info.isDirective = true;
                info.isIncludeMarker = false;
                info.isIncludeDirective = false;
                lines.push_back(info);
            } else {
                info.isDirective = false;
                info.isIncludeMarker = false;
                info.isIncludeDirective = false;
                lines.push_back(info);
            }
        }
    }
    
    std::string extractIncludeFilename(const std::string& line) {
        size_t start = line.find('"');
        if (start == std::string::npos) 
            throw std::runtime_error("Invalid include syntax");
        size_t end = line.find('"', start + 1);
        if (end == std::string::npos)
            throw std::runtime_error("Invalid include syntax");
        return line.substr(start + 1, end - start - 1);
    }
    
    std::string extractFilenameFromMarker(const std::string& marker) {
        size_t fileStart = marker.find("file=\"");
        if (fileStart == std::string::npos) return "";
        fileStart += 6;
        size_t fileEnd = marker.find('"', fileStart);
        if (fileEnd == std::string::npos) return "";
        return marker.substr(fileStart, fileEnd - fileStart);
    }
    
    void processAllDirectives(const std::vector<LineInfo>& lines) {
        defines.clear();
        macros.clear();
        
        for (const auto& info : lines) {
            if (info.isDirective && !info.isIncludeDirective) {
                std::string trimmed = trim(info.content);
                if (startsWith(trimmed, "@define")) {
                    processDefineDirective(trimmed);
                } else if (startsWith(trimmed, "@macro")) {
                    processMacroDirective(trimmed);
                }
            }
        }
    }
    
    void processDefineDirective(const std::string& line) {
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) return;
        
        std::string left = trim(line.substr(7, eqPos - 7));
        std::string right = trim(line.substr(eqPos + 1));
        
        if (!left.empty()) {
            std::string value = right;
            
            // Циклически подставляем дефайны в значение другого дефайна
            bool changed;
            do {
                changed = false;
                std::string oldValue = value;
                
                for (const auto& def : defines) {
                    // Используем replaceWholeWord для дефайнов
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
    
    void processMacroDirective(const std::string& line) {
        size_t parenStart = line.find('(');
        if (parenStart == std::string::npos) return;
        
        std::string name = trim(line.substr(6, parenStart - 6));
        
        size_t parenEnd = line.find(')', parenStart);
        if (parenEnd == std::string::npos) return;
        
        std::string paramsStr = line.substr(parenStart + 1, parenEnd - parenStart - 1);
        std::vector<std::string> params;
        std::stringstream paramStream(paramsStr);
        std::string param;
        while (std::getline(paramStream, param, ',')) {
            params.push_back(trim(param));
        }
        
        size_t eqPos = line.find('=', parenEnd);
        if (eqPos == std::string::npos) return;
        
        std::string body = trim(line.substr(eqPos + 1));
        
        std::string processedBody = body;
        
        // Подставляем дефайны в тело макроса (только целые слова)
        for (const auto& def : defines) {
            replaceWholeWord(processedBody, def.first, def.second);
        }
        
        Macro macro;
        macro.params = params;
        macro.body = processedBody;
        macros[name] = macro;
    }
    
    std::string expandInString(const std::string& str) {
        std::string result = str;
        
        // Сначала подставляем дефайны (только целые слова)
        for (const auto& def : defines) {
            replaceWholeWord(result, def.first, def.second);
        }
        
        // Затем подставляем макросы
        bool changed;
        do {
            changed = false;
            std::string old = result;
            
            for (const auto& mac : macros) {
                std::string expanded = expandMacroCall(old, mac.first, mac.second);
                if (expanded != old) {
                    result = expanded;
                    changed = true;
                    break;
                }
            }
        } while (changed);
        
        return result;
    }
    
    std::string expandMacroCall(const std::string& str, const std::string& name, const Macro& macro) {
        std::string result = str;
        size_t pos = 0;
        
        while (pos < result.size()) {
            size_t namePos = result.find(name, pos);
            if (namePos == std::string::npos) break;
            
            bool isValid = true;
            
            // Проверяем границы слова для имени макроса
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
                throw std::runtime_error("Unmatched parentheses in macro call");
            }
            
            argEnd--;
            
            std::string argsStr = result.substr(argStart, argEnd - argStart);
            std::vector<std::string> args;
            splitArguments(argsStr, args);
            
            if (args.size() != macro.params.size()) {
                throw std::runtime_error("Wrong number of arguments for macro " + name);
            }
            
            std::string expanded = macro.body;
            for (size_t i = 0; i < args.size(); i++) {
                // Для подстановки аргументов в тело макроса используем полную замену
                replaceAll(expanded, macro.params[i], args[i]);
            }
            
            result.replace(namePos, argEnd - namePos + 1, expanded);
            pos = namePos + expanded.size();
        }
        
        return result;
    }
    
    void splitArguments(const std::string& argsStr, std::vector<std::string>& args) {
        args.clear();
        std::string current;
        int parenDepth = 0;
        
        for (char c : argsStr) {
            if (c == '(') {
                parenDepth++;
                current += c;
            } else if (c == ')') {
                parenDepth--;
                current += c;
            } else if (c == ',' && parenDepth == 0) {
                args.push_back(trim(current));
                current.clear();
            } else {
                current += c;
            }
        }
        
        if (!current.empty()) {
            args.push_back(trim(current));
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
                    output << "\n";
                }
            } else {
                std::string processed = expandInString(info.content);
                output << processed << "\n";
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
        collectIncludes(source, filename, 1, lines);
        
        return processDirectives(lines);
    }
};

