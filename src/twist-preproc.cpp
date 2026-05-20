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

struct Define {
    std::vector<Token> body;
    PosInFile definition_pos;
};

struct Macro {
    std::vector<std::string> params;
    std::vector<Token> body;
    PosInFile definition_pos;
    bool variadic;
};

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

    std::vector<Token> process(const std::vector<Token>& tokens, const std::string& file_path) {
        current_file_path_ = std::filesystem::absolute(file_path).string();
        current_file_name_ = GetFileName(file_path);
        included_files_.insert(current_file_path_);
        
        std::vector<Token> after_includes = processIncludes(tokens);
        std::vector<Token> after_directives = processDefinesAndMacros(after_includes);
        return after_directives;
    }

private:
    std::unordered_map<std::string, Define> defines_;
    std::unordered_map<std::string, Macro> macros_;
    std::string current_file_path_;
    std::string current_file_name_;
    std::set<std::string> included_files_;

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
                    stream.next();
                    processIncludeDirective(stream, output);
                } else if (dir.value != "include" && dir.value != "define" && dir.value != "macro") {
                    throw Error("unsupported directive: " + dir.value, dir.pif, ErrorTypes::PREPROCESSOR, "");
                } else {
                    output.push_back(tok);
                    output.push_back(stream.next());
                }
            } else {
                output.push_back(stream.next());
            }
        }
        return output;
    }

    std::vector<Token> processDefinesAndMacros(const std::vector<Token>& tokens) {
        defines_.clear();
        macros_.clear();
        std::vector<Token> without_defines = collectDefinesAndMacros(tokens);
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
                stream.next();
                if (!stream.hasNext() || stream.peek().type != TokenType::LITERAL) {
                    output.push_back(tok);
                    throw Error("waited directive", stream.peek().pif, ErrorTypes::PREPROCESSOR, "");
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

        std::filesystem::path base_dir = std::filesystem::path(current_file_path_).parent_path();
        std::filesystem::path full_included_path = base_dir / included_path;
        std::string resolved_path = std::filesystem::absolute(full_included_path).string();

        if (included_files_.count(resolved_path)) return;

        included_files_.insert(resolved_path);
        
        auto included_tokens = lexFile(resolved_path);
        Preprocessor nested;
        nested.included_files_ = this->included_files_;
        auto processed = nested.processIncludes(included_tokens);
        this->included_files_ = nested.included_files_;

        if (!processed.empty() && processed.back().type == TokenType::END_OF_FILE) processed.pop_back();
        
        size_t total_size = 0;
        for (const auto& token : processed) total_size += token.value.size();
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
        
        std::vector<std::string> params;
        bool variadic = false;
        
        if (stream.hasNext() && stream.peek().type != TokenType::R_BRACKET) {
            while (true) {
                if (!stream.hasNext()) {
                    Error err("Unexpected end of macro parameters", PosInFile(), ErrorTypes::SYNTAX, "");
                    throw err;
                }
                if (stream.peek().type == TokenType::OPERATOR && stream.peek().value == "...") {
                    variadic = true;
                    stream.next();
                    break;
                }
                if (stream.peek().type != TokenType::LITERAL) {
                    PosInFile errPos = stream.peek().pif;
                    Error err("Expected parameter name in macro", errPos, ErrorTypes::SYNTAX, "");
                    throw err;
                }
                std::string param_name = stream.next().value;
                params.push_back(param_name);
                if (!stream.hasNext()) {
                    Error err("Unexpected end of macro parameters", PosInFile(), ErrorTypes::SYNTAX, "");
                    throw err;
                }
                if (stream.peek().type == TokenType::R_BRACKET) break;
                if (stream.peek().type == TokenType::OPERATOR && stream.peek().value == ",") {
                    stream.next();
                    if (stream.hasNext() && stream.peek().type == TokenType::LITERAL && stream.peek().value == "...") {
                        variadic = true;
                        stream.next();
                        break;
                    }
                    continue;
                }
                PosInFile errPos = stream.peek().pif;
                Error err("Expected ',' or ')' in macro parameters", errPos, ErrorTypes::SYNTAX, "");
                throw err;
            }
        }
        
        if (!stream.hasNext() || stream.peek().type != TokenType::R_BRACKET) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected ')' after macro parameters", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // ')'
        
        if (!stream.hasNext() || stream.peek().type != TokenType::L_CURVE_BRACKET) {
            PosInFile errPos = stream.hasNext() ? stream.peek().pif : PosInFile();
            Error err("Expected '{' to start macro body", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        stream.next(); // '{'
        
        std::vector<Token> body;
        int brace_depth = 1;
        while (stream.hasNext() && brace_depth > 0) {
            const Token& tok = stream.peek();
            if (tok.type == TokenType::L_CURVE_BRACKET) {
                brace_depth++;
                body.push_back(stream.next());
            } else if (tok.type == TokenType::R_CURVE_BRACKET) {
                brace_depth--;
                if (brace_depth == 0) {
                    stream.next();
                    break;
                } else {
                    body.push_back(stream.next());
                }
            } else {
                body.push_back(stream.next());
            }
        }
        if (brace_depth != 0) {
            PosInFile errPos = body.empty() ? PosInFile() : body.back().pif;
            Error err("Unclosed '{' in macro body", errPos, ErrorTypes::SYNTAX, "");
            throw err;
        }
        
        if (stream.hasNext() && stream.peek().type == TokenType::DAC) {
            stream.next();
        }
        
        Macro macro;
        macro.params = params;
        macro.body = body;
        macro.variadic = variadic;
        macro.definition_pos = body.empty() ? PosInFile() : body[0].pif;
        macros_[name] = macro;
    }

    // Преобразование вектора токенов в строку (для @ параметра)
    std::string tokensToString(const std::vector<Token>& tokens) {
        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            result += tokens[i].value;
            if (i + 1 < tokens.size()) {
                result += " ";
            }
        }
        return result;
    }

    // Основной метод раскрытия макросов с поддержкой @ (stringify) и % (конкатенация)
    std::vector<Token> expandTokens(const std::vector<Token>& tokens,
                                    std::set<std::string>& forbidden_macros) {
        std::vector<Token> result;
        TokenStream stream(tokens);

        while (stream.hasNext()) {
            const Token& tok = stream.peek();
            
            // Обработка #define (простых макросов-объектов)
            if (tok.type == TokenType::LITERAL) {
                auto defIt = defines_.find(tok.value);
                if (defIt != defines_.end()) {
                    stream.next();
                    const auto& body = defIt->second.body;
                    auto expanded = expandTokens(body, forbidden_macros);
                    result.insert(result.end(), expanded.begin(), expanded.end());
                    continue;
                }
            }
            
            // Обработка макросов-функций
            if (tok.type == TokenType::LITERAL) {
                auto macIt = macros_.find(tok.value);
                if (macIt != macros_.end()) {
                    size_t savedPos = stream.getPosition();
                    stream.next(); // имя макроса
                    
                    if (stream.hasNext() && stream.peek().type == TokenType::L_BRACKET) {
                        stream.next(); // '('
                        
                        if (forbidden_macros.find(tok.value) != forbidden_macros.end()) {
                            stream.setPosition(savedPos);
                            result.push_back(stream.next());
                            continue;
                        }
                        
                        // Парсинг аргументов
                        std::vector<std::vector<Token>> args;
                        int paren_depth = 1;
                        
                        if (macIt->second.params.empty() && !macIt->second.variadic &&
                            stream.hasNext() && stream.peek().type == TokenType::R_BRACKET) {
                            stream.next(); // ')'
                        } else {
                            std::vector<Token> current_arg;
                            while (stream.hasNext() && paren_depth > 0) {
                                const Token& arg_tok = stream.peek();
                                if (arg_tok.type == TokenType::L_BRACKET) {
                                    paren_depth++;
                                    current_arg.push_back(stream.next());
                                } else if (arg_tok.type == TokenType::R_BRACKET) {
                                    paren_depth--;
                                    if (paren_depth == 0) {
                                        if (!current_arg.empty() || !args.empty()) {
                                            args.push_back(current_arg);
                                        }
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
                        
                        // Проверка количества аргументов
                        if (!macIt->second.variadic) {
                            if (args.size() != macIt->second.params.size()) {
                                PosInFile errPos = tok.pif;
                                Error err("Wrong number of arguments for macro '" + tok.value +
                                          "': expected " + std::to_string(macIt->second.params.size()) +
                                          ", got " + std::to_string(args.size()),
                                          errPos, ErrorTypes::MACRO, "");
                                throw err;
                            }
                        } else {
                            if (args.size() < macIt->second.params.size()) {
                                PosInFile errPos = tok.pif;
                                Error err("Wrong number of arguments for variadic macro '" + tok.value +
                                          "': expected at least " + std::to_string(macIt->second.params.size()) +
                                          ", got " + std::to_string(args.size()),
                                          errPos, ErrorTypes::MACRO, "");
                                throw err;
                            }
                        }
                        
                        forbidden_macros.insert(tok.value);
                        
                        // Формируем substituted_body, обрабатывая @ перед параметрами
                        std::vector<Token> substituted_body;
                        for (size_t i = 0; i < macIt->second.body.size(); ++i) {
                            const Token& bt = macIt->second.body[i];
                            
                            // Проверяем, не является ли текущий токен оператором @ и следующий - литералом-параметром
                            if (bt.type == TokenType::OPERATOR && bt.value == "@" && i + 1 < macIt->second.body.size()) {
                                const Token& next_bt = macIt->second.body[i + 1];
                                if (next_bt.type == TokenType::LITERAL) {
                                    auto paramIt = std::find(macIt->second.params.begin(), macIt->second.params.end(), next_bt.value);
                                    if (paramIt != macIt->second.params.end()) {
                                        size_t idx = paramIt - macIt->second.params.begin();
                                        if (idx < args.size()) {
                                            // Превращаем аргумент в строку
                                            std::string str_value = tokensToString(args[idx]);
                                            Token string_token;
                                            string_token.type = TokenType::STRING;
                                            string_token.value = str_value;
                                            string_token.pif = bt.pif; // позиция @
                                            substituted_body.push_back(string_token);
                                            i++; // пропускаем имя параметра
                                            continue;
                                        }
                                    }
                                }
                                // Если не параметр, то оставляем как есть (оба токена)
                                substituted_body.push_back(bt);
                                substituted_body.push_back(next_bt);
                                i++;
                                continue;
                            }
                            
                            // Обычная подстановка параметра (без @)
                            if (bt.type == TokenType::LITERAL) {
                                auto paramIt = std::find(macIt->second.params.begin(), macIt->second.params.end(), bt.value);
                                if (paramIt != macIt->second.params.end()) {
                                    size_t idx = paramIt - macIt->second.params.begin();
                                    if (idx < args.size()) {
                                        substituted_body.insert(substituted_body.end(), args[idx].begin(), args[idx].end());
                                        continue;
                                    }
                                }
                            }
                            substituted_body.push_back(bt);
                        }
                        
                        // Первое раскрытие макросов в подставленном теле
                        auto expanded_body = expandTokens(substituted_body, forbidden_macros);
                        
                        // === ОБРАБОТКА ОПЕРАТОРА КОНКАТЕНАЦИИ % ===
                        std::vector<Token> concatenated_body;
                        for (size_t i = 0; i < expanded_body.size(); ++i) {
                            if (expanded_body[i].type == TokenType::OPERATOR && expanded_body[i].value == "%") {
                                if (concatenated_body.empty() || i + 1 >= expanded_body.size()) {
                                    Error err("Invalid use of % operator for concatenation",
                                              expanded_body[i].pif, ErrorTypes::MACRO, "");
                                    throw err;
                                }
                                Token left = concatenated_body.back();
                                concatenated_body.pop_back();
                                Token right = expanded_body[i + 1];
                                std::string concatenated = left.value + right.value;
                                Token new_token;
                                new_token.type = TokenType::LITERAL;
                                new_token.value = concatenated;
                                new_token.pif = left.pif;
                                concatenated_body.push_back(new_token);
                                i++; // пропускаем правый операнд
                            } else {
                                concatenated_body.push_back(expanded_body[i]);
                            }
                        }
                        
                        // Повторное раскрытие результата конкатенации
                        auto final_expanded = expandTokens(concatenated_body, forbidden_macros);
                        
                        forbidden_macros.erase(tok.value);
                        result.insert(result.end(), final_expanded.begin(), final_expanded.end());
                        continue;
                    } else {
                        stream.setPosition(savedPos);
                    }
                }
            }
            
            // Обычный токен
            result.push_back(stream.next());
        }
        
        return result;
    }
};