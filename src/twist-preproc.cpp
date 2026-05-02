#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <set>

#include "twist-tokens.cpp"
#include "twist-lexer.cpp"
#include "twist-utils.cpp"
#include "twist-err.cpp"

namespace ErrorTypes {
    const std::string MACRO      = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::YELLOW + "macro"      + TERMINAL_COLORS::RESET;
    const std::string INCLUDE    = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::GREEN  + "include"    + TERMINAL_COLORS::RESET;
    const std::string PREPROCESSOR = TERMINAL_COLORS::BOLD + TERMINAL_COLORS::BLUE  + "preprocessor" + TERMINAL_COLORS::RESET;
}

// Структура для хранения define
struct Define {
    std::vector<Token> body;
    PosInFile definition_pos;
};

// Структура для хранения macro
struct Macro {
    std::vector<std::string> params;
    std::vector<Token> body;
    PosInFile definition_pos;
};

// Обёртка для потоковой обработки токенов
class TokenStream {
private:
    const std::vector<Token>& tokens;
    size_t position;
public:
    TokenStream(const std::vector<Token>& tokens) : tokens(tokens), position(0) {}
    
    bool hasNext() const {
        return position < tokens.size();
    }
    
    const Token& peek() const {
        if (!hasNext()) throw std::runtime_error("Unexpected end of token stream");
        return tokens[position];
    }
    
    const Token& peek(size_t offset) const {
        if (position + offset >= tokens.size()) throw std::runtime_error("Unexpected end of token stream");
        return tokens[position + offset];
    }
    
    const Token& next() {
        if (!hasNext()) throw std::runtime_error("Unexpected end of token stream");
        return tokens[position++];
    }
    
    size_t getPosition() const { return position; }
    void setPosition(size_t pos) { position = pos; }
};

class Preprocessor {
public:
    Preprocessor() = default;

    // Главный метод обработки
    // Главный метод обработки
    std::vector<Token> process(const std::vector<Token>& tokens, const std::string& file_path) {
        current_file_path_ = std::filesystem::absolute(file_path).string();
        current_file_name_ = GetFileName(file_path);
        included_files_.insert(current_file_path_);
        
        // ============================================
        // ПРОХОД 1: ТОЛЬКО #include (рекурсивно)
        // ============================================
        std::vector<Token> after_includes = processIncludes(tokens);
        
        // ============================================
        // ПРОХОД 2: сбор #define и #macro из ОБЪЕДИНЁННОГО списка
        // ============================================
        std::vector<Token> after_directives = processDefinesAndMacros(after_includes);
        
        return after_directives;
    }

private:
    std::unordered_map<std::string, Define> defines_;
    std::unordered_map<std::string, Macro> macros_;
    std::string current_file_path_;
    std::string current_file_name_;
    std::set<std::string> included_files_;

    // ---------------------------------------------------------------
// ПРОХОД 1: рекурсивная вставка всех #include
// ---------------------------------------------------------------
std::vector<Token> processIncludes(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    TokenStream stream(tokens);
    
    while (stream.hasNext()) {
        const Token& tok = stream.peek();
        
        if (tok.type == TokenType::PREPROC) {
            stream.next(); // '#'
            
            if (!stream.hasNext() || stream.peek().type != TokenType::LITERAL) {
                output.push_back(tok);
                continue;
            }
            
            const Token& dir = stream.peek();
            
            if (dir.value == "include") {
                stream.next(); // 'include'
                processIncludeDirective(stream, output);
            } else {
                // Оставляем #define и #macro для второго прохода
                output.push_back(tok);        // '#'
                output.push_back(stream.next()); // директива
            }
        } else {
            output.push_back(stream.next());
        }
    }
    
    return output;
}

// ---------------------------------------------------------------
// ПРОХОД 2: сбор #define/#macro и подстановка
// ---------------------------------------------------------------
std::vector<Token> processDefinesAndMacros(const std::vector<Token>& tokens) {
    defines_.clear();
    macros_.clear();
    
    // Шаг 2a: собираем все #define и #macro
    std::vector<Token> without_defines = collectDefinesAndMacros(tokens);
    
    // Шаг 2b: подставляем все define и macro
    std::set<std::string> forbidden;
    std::vector<Token> result = expandTokens(without_defines, forbidden);
    
    return result;
}

std::vector<Token> collectDefinesAndMacros(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    TokenStream stream(tokens);
    
    while (stream.hasNext()) {
        const Token& tok = stream.peek();
        
        if (tok.type == TokenType::PREPROC) {
            stream.next(); // '#'
            
            if (!stream.hasNext() || stream.peek().type != TokenType::LITERAL) {
                output.push_back(tok);
                continue;
            }
            
            const Token& dir = stream.next();
            
            if (dir.value == "define") {
                processDefineDirective(stream);
            } else if (dir.value == "macro") {
                processMacroDirective(stream);
            } else {
                output.push_back(tok);
                output.push_back(dir);
            }
        } else {
            output.push_back(stream.next());
        }
    }
    
    return output;
}

    // Лексирование файла в токены
    std::vector<Token> lexFile(const std::string& path) {
        std::string content = OpenFile(path);
        if (content.empty()) {
            PosInFile errPos;
            errPos.file_path = path;
            errPos.file_name = GetFileName(path);
            errPos.line = 1;
            errPos.global_line = 1;
            errPos.index = 0;
            errPos.lenght = 1;
            Error err("Cannot open include file: " + path, errPos, ErrorTypes::INCLUDE, "");
            throw err;
        }
        Lexer lexer(path, content);
        lexer.run();
        return std::move(lexer.tokens);
    }

    // Сбор директив из потока токенов
    std::vector<Token> collectDirectives(TokenStream& stream) {
        std::vector<Token> output;
        while (stream.hasNext()) {
            const Token& tok = stream.peek();
            if (tok.type == TokenType::PREPROC) {
                stream.next(); // '#'
                if (!stream.hasNext() || stream.peek().type != TokenType::LITERAL) {
                    PosInFile errPos = tok.pif;
                    Error err("Expected preprocessor directive after '#'", errPos, ErrorTypes::SYNTAX, "");
                    throw err;
                }
                const Token& dir = stream.next();
                if (dir.value == "include") {
                    processIncludeDirective(stream, output);
                } else if (dir.value == "define") {
                    processDefineDirective(stream);
                } else if (dir.value == "macro") {
                    processMacroDirective(stream);
                } else {
                    PosInFile errPos = dir.pif;
                    Error err("Unknown preprocessor directive '#" + dir.value + "'", errPos, ErrorTypes::PREPROCESSOR, "");
                    throw err;
                }
            } else {
                output.push_back(stream.next());
            }
        }
        return output;
    }

    // Обработка #include "file_path";
    void processIncludeDirective(TokenStream& stream, std::vector<Token>& output) {
        auto directive_pos = stream.peek();
        if (!stream.hasNext() || stream.peek().type != TokenType::STRING) {
            Error err("Expected string literal after #include", 
                    stream.hasNext() ? stream.peek().pif : PosInFile(), ErrorTypes::SYNTAX, "");
            throw err;
        }
        std::string included_path = stream.next().value;
        if (!stream.hasNext() || stream.peek().type != TokenType::DAC) {
            Error err("Expected ';' after #include path", 
                    stream.hasNext() ? stream.peek().pif : PosInFile(), ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // ';'

        // Разрешение пути относительно текущего файла
        std::filesystem::path base_dir = std::filesystem::path(current_file_path_).parent_path();
        std::filesystem::path full_included_path = base_dir / included_path;
        std::string resolved_path = std::filesystem::absolute(full_included_path).string();

        if (included_files_.count(resolved_path)) return; // уже включён

        included_files_.insert(resolved_path);
        
        auto included_tokens = lexFile(resolved_path);
        Preprocessor nested;
        nested.included_files_ = this->included_files_;
        
        auto processed = nested.processIncludes(included_tokens);
        
        this->included_files_ = nested.included_files_;
        
        // Удаляем END_OF_FILE из включаемого файла перед вставкой
        if (!processed.empty() && processed.back().type == TokenType::END_OF_FILE) processed.pop_back();
        
        // Суммируем длину значений всех токенов (включая вложенные include)
        size_t total_size = 0;
        for (const auto& token : processed) {
            total_size += token.value.size();
        }
        
        // Форматируем размер
        std::string size_str;
        if (total_size >= 1024 * 1024) {
            double mb = static_cast<double>(total_size) / (1024.0 * 1024.0);
            size_str = "~" + std::to_string(mb).substr(0, 5) + " mb";
        } else if (total_size >= 1024) {
            double kb = static_cast<double>(total_size) / 1024.0;
            size_str = "~" + std::to_string(kb).substr(0, 5) + " kb";
        } else {
            size_str = "~" + std::to_string(total_size) + " byte";
        }
        
        Error err(size_str, directive_pos.pif, ErrorTypes::ECHO, "");
        err.message_type = 2;
        err.Write();
        
        output.insert(output.end(), processed.begin(), processed.end());
    }

    // Обработка #define name = ...;
    void processDefineDirective(TokenStream& stream) {
        if (!stream.hasNext() || (stream.peek().type != TokenType::LITERAL && stream.peek().type != TokenType::KEYWORD)) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected identifier after #define", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        std::string name = stream.next().value;
        if (!stream.hasNext() || !(stream.peek().type == TokenType::OPERATOR && stream.peek().value == "=")) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected '=' in #define", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // '='
        std::vector<Token> body;
        while (stream.hasNext() && stream.peek().type != TokenType::DAC) {
            body.push_back(stream.next());
        }
        if (!stream.hasNext()) {
            PosInFile errPos = body.empty() ? PosInFile() : body.back().pif;
            Error err("Missing ';' after #define body", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // ';'
        defines_[name] = {body, body.empty() ? PosInFile() : body[0].pif};
    }

    // Обработка #macro name(params) = [...];
    void processMacroDirective(TokenStream& stream) {
        if (!stream.hasNext() || stream.peek().type != TokenType::LITERAL) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected macro name after #macro", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        std::string name = stream.next().value;
        
        if (!stream.hasNext() || stream.peek().type != TokenType::L_BRACKET) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected '(' after macro name", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // '('
        
        // Парсим параметры
        std::vector<std::string> params;
        if (stream.hasNext() && stream.peek().type != TokenType::R_BRACKET) {
            while (true) {
                if (!stream.hasNext() || stream.peek().type != TokenType::LITERAL) {
                    PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
                    Error err("Expected parameter name in macro", errPos, ErrorTypes::SYNTAX, "");
                    throw err;
                }
                params.push_back(stream.next().value);
                
                if (!stream.hasNext()) {
                    Error err("Unexpected end of macro parameters", PosInFile(), ErrorTypes::SYNTAX, "");
                    throw err;
                }
                if (stream.peek().type == TokenType::R_BRACKET) break;
                
                if (!(stream.peek().type == TokenType::OPERATOR && stream.peek().value == ",")) {
                    PosInFile errPos = stream.peek().pif;
                    Error err("Expected ',' or ')' in macro parameters", errPos, ErrorTypes::SYNTAX, "");
                    throw err;
                }
                stream.next(); // ','
            }
        }
        
        if (!stream.hasNext() || stream.peek().type != TokenType::R_BRACKET) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected ')' after macro parameters", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // ')'
        
        if (!stream.hasNext() || !(stream.peek().type == TokenType::OPERATOR && stream.peek().value == "=")) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected '=' after macro signature", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // '='
        
        if (!stream.hasNext() || stream.peek().type != TokenType::L_RECT_BRACKET) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected '[' to start macro body", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // '['
        
        // Собираем тело макроса до ']' с учётом вложенности
        std::vector<Token> body;
        int bracket_depth = 1;
        while (stream.hasNext() && bracket_depth > 0) {
            const Token& tok = stream.peek();
            if (tok.type == TokenType::L_RECT_BRACKET) {
                bracket_depth++;
                body.push_back(stream.next());
            } else if (tok.type == TokenType::R_RECT_BRACKET) {
                bracket_depth--;
                if (bracket_depth == 0) {
                    stream.next(); // поглощаем закрывающую скобку, но не добавляем в тело
                    break;
                } else {
                    body.push_back(stream.next());
                }
            } else {
                body.push_back(stream.next());
            }
        }
        
        if (bracket_depth != 0) {
            PosInFile errPos = body.empty() ? PosInFile() : body.back().pif;
            Error err("Unclosed '[' in macro body", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        
        if (!stream.hasNext() || stream.peek().type != TokenType::DAC) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected ';' after macro body", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // ';'
        
        macros_[name] = {params, body, body.empty() ? PosInFile() : body[0].pif};
    }

    // Рекурсивное раскрытие токенов с защитой от бесконечной рекурсии
    std::vector<Token> expandTokens(const std::vector<Token>& tokens,
                                    std::set<std::string>& forbidden_macros) {
        std::vector<Token> result;
        TokenStream stream(tokens);

        while (stream.hasNext()) {
            const Token& tok = stream.peek();

            // Обрабатываем define
            if (tok.type == TokenType::LITERAL) {
                auto defIt = defines_.find(tok.value);
                if (defIt != defines_.end()) {
                    stream.next(); // потребляем имя
                    const auto& body = defIt->second.body;
                    auto expanded = expandTokens(body, forbidden_macros);
                    result.insert(result.end(), expanded.begin(), expanded.end());
                    continue;
                }
            }

            // Обрабатываем macro
            if (tok.type == TokenType::LITERAL) {
                auto macIt = macros_.find(tok.value);
                if (macIt != macros_.end()) {
                    size_t savedPos = stream.getPosition();
                    stream.next(); // потребляем имя макроса

                    if (stream.hasNext() && stream.peek().type == TokenType::L_BRACKET) {
                        stream.next(); // '('

                        if (forbidden_macros.find(tok.value) != forbidden_macros.end()) {
                            stream.setPosition(savedPos);
                            result.push_back(stream.next());
                            continue;
                        }

                        // Парсим аргументы
                        std::vector<std::vector<Token>> args;
                        if (macIt->second.params.empty() &&
                            stream.hasNext() && stream.peek().type == TokenType::R_BRACKET) {
                            stream.next();
                        } else {
                            std::vector<Token> current_arg;
                            int paren_depth = 1;
                            while (stream.hasNext() && paren_depth > 0) {
                                const Token& arg_tok = stream.peek();
                                if (arg_tok.type == TokenType::L_BRACKET) {
                                    paren_depth++;
                                    if (paren_depth > 1) current_arg.push_back(stream.next());
                                    else stream.next();
                                } else if (arg_tok.type == TokenType::R_BRACKET) {
                                    paren_depth--;
                                    if (paren_depth == 0) {
                                        if (!current_arg.empty() || !args.empty())
                                            args.push_back(current_arg);
                                        stream.next();
                                        break;
                                    } else {
                                        current_arg.push_back(stream.next());
                                    }
                                } else if (arg_tok.type == TokenType::OPERATOR && arg_tok.value == "," && paren_depth == 1) {
                                    args.push_back(current_arg);
                                    current_arg.clear();
                                    stream.next();
                                } else {
                                    current_arg.push_back(stream.next());
                                }
                            }
                            if (paren_depth != 0) {
                                PosInFile errPos = tok.pif;
                                Error err("Unmatched '(' in macro call", errPos, ErrorTypes::MACRO, "");
                                throw err;
                            }
                        }

                        if (args.size() != macIt->second.params.size()) {
                            PosInFile errPos = tok.pif;
                            Error err("Wrong number of arguments for macro '" + tok.value +
                                      "': expected " + std::to_string(macIt->second.params.size()) +
                                      ", got " + std::to_string(args.size()),
                                      errPos, ErrorTypes::MACRO, "");
                            throw err;
                        }

                        forbidden_macros.insert(tok.value);

                        std::vector<Token> substituted_body;
                        for (const Token& bt : macIt->second.body) {
                            if (bt.type == TokenType::LITERAL) {
                                auto paramIt = std::find(macIt->second.params.begin(),
                                                        macIt->second.params.end(), bt.value);
                                if (paramIt != macIt->second.params.end()) {
                                    size_t idx = paramIt - macIt->second.params.begin();
                                    if (idx < args.size()) {
                                        substituted_body.insert(substituted_body.end(),
                                                               args[idx].begin(), args[idx].end());
                                        continue;
                                    }
                                }
                            }
                            substituted_body.push_back(bt);
                        }

                        auto expanded_body = expandTokens(substituted_body, forbidden_macros);
                        forbidden_macros.erase(tok.value);

                        result.insert(result.end(), expanded_body.begin(), expanded_body.end());
                        continue;
                    } else {
                        stream.setPosition(savedPos);
                    }
                }
            }

            result.push_back(stream.next());
        }

        return result;
    }
};