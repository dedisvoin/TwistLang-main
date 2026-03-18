#include "twist-tokens.cpp"
#include "twist-utils.cpp"
#include <string>
#include <vector>

#define CH const string

using namespace std;

// Full twist alphabet
namespace ALPHABET {
    CH CHAR = "abcdefghijklmnopqrstuvwxyz" + ToUpper("abcdefghijklmnopqrstuvwxyz");
    CH NUMS = "0123456789";
    CH OPER = "+-=&/@^%!|~,.<>?";
    CH BRAC = "{[()]}";
    CH SEPP = ":;";
}
#define APT ALPHABET

inline static const bool IS_CHAR(const char c) { return InString(c, APT::CHAR); }
inline static const bool IS_NUMS(const char c) { return InString(c, APT::NUMS); }
inline static const bool IS_OPER(const char c) { return InString(c, APT::OPER); }
inline static const bool IS_BRAC(const char c) { return InString(c, APT::BRAC); }
inline static const bool IS_SEPP(const char c) { return InString(c, APT::SEPP); }



vector<string> KEYWORDS = { "if", "else", "for", "while", 
    "do", "break", "continue", "let", 
    "static", "final", "const", "global", "typeof", "sizeof",
     "del", "new" ,"true", "false", "null", "ret", "struct",
    "out", "outln", "input", "in" , "and", "or", "namespace", "assert", "lambda",  "func", "Func", "exit", "private"};

struct Lexer {
    int line = 1;
    int global_line = 1;
    int pos = 0;
    int pos_in_line = 0;
    int saved_pos_in_line = 0;
    string file_path;
    string file_name;


    string main_file_path, file_data;
    int main_file_size;
    vector<Token> tokens;

    Lexer(string main_file_path, string file_data) {
        this->main_file_path = main_file_path;
        
        // Удаление пробельных символов в конце строки
        while (!file_data.empty()) {
            char last = file_data.back();
            if (last == ' ' || last == '\t' || last == '\n' || last == '\r')
                file_data.pop_back();
            else
                break;
        }
        
        this->file_data = file_data + "\n";
        this->main_file_size = this->file_data.size();
        this->file_path = this->main_file_path;
        this->file_name = GetFileName(this->file_path);
    }

    inline void next() { this->pos++; }
    inline void next(int c) {
        for (int i = 0; i < c; i++) 
            this->pos++;
    }
    inline void next_in_line() { this->pos_in_line++; }
    inline void next_line() { 
        this->line++; 
        
        if (!(pos == this->main_file_size))  {
            this->pos_in_line = 0; 
            
        }
        this->global_line++;
        
            
        
    }
    inline char get(const int offset = 0) { return this->file_data[this->pos + offset]; }
    inline void get_type() { char c = this->get(); }

    /*
        Adding token to tokens vector
    */
    void add_token(string value, TokenType type, PosInFile pos) {
        Token T(type, value, pos);
        tokens.push_back(T);
    }

    /*
        Parse literal
    */
    void parse_literal() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V;
        char        C = this->get();
        
        while (IS_CHAR(C) || C == '_' || IS_NUMS(C)) {
            V += C;
            this->next();
            this->next_in_line();
            C = this->get();
        }

        int L = this->pos - P;
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = L,
        };
        if (InVector(KEYWORDS, V)) this->add_token(V, TokenType::KEYWORD, PIF);
        else this->add_token(V, TokenType::LITERAL, PIF);
    }

    /*
        Parse number
    */
    void parse_number() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V;
        char        C = this->get();

        if (C == '.') {
            V += "0.";
            next();
            next_in_line();
            C = this->get();
        }
        

        while (IS_NUMS(C) || C == '.') {
            V += C;
            this->next();
            this->next_in_line();
            C = this->get();
        }

        int L = this->pos - P;
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = L
        };
        this->add_token(V, TokenType::NUMBER, PIF);
    }

    /*
        Parse string or char
    */
    void parse_string_or_char() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        int         SL = this->line;
        string      V;
        char        C = this->get();
        char        SC = this->get();

        next();
        next_in_line();
        C = this->get();

        while (C != SC) { 
            if (C == '\\') {
                // Handle escape sequences
                next();
                next_in_line();
                C = this->get();
                
                switch (C) {
                    case 'n':  V += '\n'; break;
                    case 't':  V += '\t'; break;
                    case 'r':  V += '\r'; break;
                    case '\\': V += '\\'; break;
                    case '"':  V += '"'; break;
                    case '\'': V += '\''; break;
                    case '0':  V += '\0'; break;
                    case 'x': {
                        // Hex escape: \xHH
                        next();
                        next_in_line();
                        char hex1 = this->get();
                        next();
                        next_in_line();
                        char hex2 = this->get();
                        
                        string hexStr;
                        hexStr += hex1;
                        hexStr += hex2;
                        int hexVal = stoi(hexStr, nullptr, 16);
                        V += (char)hexVal;
                        break;
                    }
                    default: 
                        V += '\\';
                        V += C;
                        break;
                }
            } else {
                V += C;
            }
            
            next();
            next_in_line();
            C = this->get();
            if (C == '\n') {next_line();}
        }
        
        next();
        next_in_line();

        int L = this->pos - P;
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = SL,
            .global_line = global_line,
            .index = PL,
            .lenght = L
        };

        TokenType T = (V.size() == 1) ? TokenType::CHAR : TokenType::STRING;
        this->add_token(V, T, PIF);
    }

    /*
        Parse sepparator
    */
    void parse_sepparator() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        char        C = this->get();

        next();
        next_in_line();

        TokenType T = (C == ';') ? TokenType::DAC :
                      (C == ':') ? TokenType::DAD : 
                                   TokenType::DUMMY;
        
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = 1
        };

        
        this->add_token(string() + C, T, PIF);
    }

    /*
        Parse operator
    */
    void parse_operator() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V;
        char        C = this->get();
        
        while (IS_OPER(C)) {
            V += C;
            this->next();
            this->next_in_line();
            C = this->get();
        }

        int L = this->pos - P;
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = L
        };
        this->add_token(V, TokenType::OPERATOR, PIF);
    }

    void parse_dereference() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V;
        char        C = this->get();
        V = "*";
        
        next();
        this->next_in_line();
        int L = this->pos - P;
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = L
        };
        this->add_token(V, TokenType::DEREFERENCE, PIF);
    }

    /*
        Parse bracket
    */
    void parse_bracket() {
        int PL = this->pos_in_line;
        char C = this->get();
        TokenType T;
        
        switch (C) {
            case '(' : T = TokenType::L_BRACKET; break;
            case ')' : T = TokenType::R_BRACKET; break;
            case '[' : T = TokenType::L_RECT_BRACKET; break;
            case ']' : T = TokenType::R_RECT_BRACKET; break;
            case '{' : T = TokenType::L_CURVE_BRACKET; break;
            case '}' : T = TokenType::R_CURVE_BRACKET; break;
            default: T = TokenType::DUMMY; break;
        }
        
        this->next();
        this->next_in_line();
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = 1
        };
        
        this->add_token(string(1, C), T, PIF);
    }

    /*
        Parse comment
    */
    void parse_comment() {
        char        C = this->get(); 
        
        while (C != '\n') {
            this->next();
            this->next_in_line();
            C = this->get();
        }

    }

    void run() {
        while (pos < this->main_file_size) {
            char _ = get();
            if (_ == '\n') {
                next();
                next_line();
                continue;
            }
            if (get() == '<' && 
                IsSubString(this->file_data, "start", this->pos + 1)) {
                next(13);
                string new_file_path;

                // new included file name parsing
                while (get() != '"') {
                    new_file_path += get();
                    next();
                }
                this->file_path = new_file_path;
                this->file_name = GetFileName(this->file_path);
                this->line = 1; // start reline
                
                // found new line 
                while (get() != '\n') { next(); }
                next(); // pass next line sym
                global_line++;
                continue;
            }
            if (get() == '<' && 
                IsSubString(this->file_data, "end", this->pos + 1)) {
                next(11);
                string new_file_path;

                // saved included file name parsing
                while (get() != '"') {
                    new_file_path += get();
                    next();
                }
                this->file_path = new_file_path;
                this->file_name = GetFileName(this->file_path);
                
                // found equal sym
                while (get() != '=') { next(); }
                next(); // pass equal sym

                
                string saved_line_num;
                // parse saved line number
                while (IS_NUMS(get())) {
                    saved_line_num += get();
                    next();
                }
                this->line = atoi(saved_line_num.c_str()) + 1;
                
                while (get() != '\n') { next(); }
                next();
                global_line++;
                continue;
            }
            if (_ == ' ') {
                next();
                next_in_line();
                continue;
            }
            if (IS_CHAR(_) || _ == '_') {
                parse_literal();
                continue;
            }
            
            if (IS_NUMS(_) || (_ == '.' && IS_NUMS(get(1)))) {
                parse_number();
                continue;
            }
            
            if (_ == '"' || _ == (const char)39) {
                parse_string_or_char();
                continue;
            }

            if (_ == ';' || _ == ':') {
                parse_sepparator();
                continue;
            }
            if (_ == '*') {
                parse_dereference();
                continue;
            }

            if (_ == '/' && get(1) == '/') {
                parse_comment();
                continue;
            }

            if (IS_OPER(_)) {
                parse_operator();
                continue;
            }

            if (IS_BRAC(_)) {
                parse_bracket();
                continue;
            }

            
        }
         if (!tokens.empty()) {
        // Берём последний реальный токен
        Token& last_token = tokens.back();
        PosInFile last_pos = last_token.pif;
        // Вычисляем позицию его последнего символа
        int last_char_index = last_pos.index + last_pos.lenght - 1;
        PosInFile eof_pos = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = last_pos.line,
            .global_line = last_pos.global_line,
            .index = last_char_index,
            .lenght = 1               // длина 1, указывает на последний символ
        };
        add_token("END_OF_FILE", TokenType::END_OF_FILE, eof_pos);
    } else {
        // Файл пуст – указываем на самое начало (позиция 0, длина 1)
        PosInFile eof_pos = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = 1,
            .global_line = 1,
            .index = 0,
            .lenght = 1
        };
        add_token("END_OF_FILE", TokenType::END_OF_FILE, eof_pos);
    }
    }

};