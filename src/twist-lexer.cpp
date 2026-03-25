#include "twist-tokens.cpp"
#include "twist-utils.cpp"
#include <string>
#include <vector>


#define CH const string

using namespace std;

// Full twist alphabet
namespace ALPHABET {
    CH CHAR = "abcdefghijklmnopqrstuvwxyz" + ToUpper("abcdefghijklmnopqrstuvwxyz") + "абвгдеёжхийклмнопрстуфхцчщшьъэюя" + ToUpper("абвгдеёжхийклмнопрстуфхцчщшьъэюя");
    CH RU_CHAR = "абвгдеёжхийклмнопрстуфхцчщшьъэюя" + ToUpper("абвгдеёжхийклмнопрстуфхцчщшьъэюя");
    CH NUMS = "0123456789";
    CH OPER = "+-=&/@^%!|~,.<>?";
    CH BRAC = "{[()]}";
    CH SEPP = ":;";
}
#define APT ALPHABET

// UTF-8 helper functions
struct UTF8Char {
    string chars;
    int byte_len;
    int char_len; // 1 for ASCII, 1 for Russian (but 2 bytes)
};

class UTF8Helper {
public:
    // Get UTF-8 character at position
    static UTF8Char getChar(const string& str, int pos) {
        UTF8Char result;
        if (pos >= (int)str.length()) {
            result.chars = "";
            result.byte_len = 0;
            result.char_len = 0;
            return result;
        }
        
        unsigned char c = static_cast<unsigned char>(str[pos]);
        
        // ASCII character (1 byte)
        if (c <= 0x7F) {
            result.chars = str.substr(pos, 1);
            result.byte_len = 1;
            result.char_len = 1;
            return result;
        }
        
        // Russian letters in UTF-8 (2 bytes)
        if (c == 0xD0 || c == 0xD1) {
            if (pos + 1 < (int)str.length()) {
                result.chars = str.substr(pos, 2);
                result.byte_len = 2;
                result.char_len = 1;
                return result;
            }
        }
        
        // Other multi-byte characters (3 or 4 bytes) - treat as single char
        int bytes = 1;
        if ((c & 0xE0) == 0xC0) bytes = 2;
        else if ((c & 0xF0) == 0xE0) bytes = 3;
        else if ((c & 0xF8) == 0xF0) bytes = 4;
        
        if (pos + bytes <= (int)str.length()) {
            result.chars = str.substr(pos, bytes);
            result.byte_len = bytes;
            result.char_len = 1;
            return result;
        }
        
        result.chars = "";
        result.byte_len = 0;
        result.char_len = 0;
        return result;
    }
    
    // Check if character is Russian
    static bool isRussianChar(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        
        unsigned char c = static_cast<unsigned char>(str[pos]);
        
        // Russian letters in UTF-8 start with D0 or D1
        if (c == 0xD0 && pos + 1 < (int)str.length()) {
            unsigned char next = static_cast<unsigned char>(str[pos + 1]);
            // D0 90 - D0 BF (А-п)
            if (next >= 0x90 && next <= 0xBF) return true;
        } else if (c == 0xD1 && pos + 1 < (int)str.length()) {
            unsigned char next = static_cast<unsigned char>(str[pos + 1]);
            // D1 80 - D1 8F (р-я)
            if (next >= 0x80 && next <= 0x8F) return true;
        }
        
        return false;
    }
    
    // Check if character is letter (Latin or Russian)
    static bool isLetter(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        
        unsigned char c = static_cast<unsigned char>(str[pos]);
        
        // ASCII letters
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            return true;
        }
        
        // Russian letters
        return isRussianChar(str, pos);
    }
    
    // Check if character is digit
    static bool isDigit(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        char c = str[pos];
        return (c >= '0' && c <= '9');
    }
    
    // Check if character is operator
    static bool isOperator(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        char c = str[pos];
        return InString(c, APT::OPER);
    }
    
    // Check if character is bracket
    static bool isBracket(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        char c = str[pos];
        return InString(c, APT::BRAC);
    }
    
    // Check if character is separator
    static bool isSeparator(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        char c = str[pos];
        return InString(c, APT::SEPP);
    }
    
    // Check if character is whitespace
    static bool isWhitespace(const string& str, int pos) {
        if (pos >= (int)str.length()) return false;
        char c = str[pos];
        return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    }
    
    // Convert string to lowercase (supports Russian)
    static string toLower(const string& str) {
        string result;
        for (size_t i = 0; i < str.length(); ) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            
            // ASCII
            if (c <= 0x7F) {
                result += tolower(str[i]);
                i++;
                continue;
            }
            
            // Russian letters
            if (c == 0xD0 && i + 1 < str.length()) {
                unsigned char next = static_cast<unsigned char>(str[i + 1]);
                // Convert uppercase to lowercase
                if (next >= 0x90 && next <= 0xBF) {
                    // А-П (0x90-0x9F) -> а-п (0xB0-0xBF)
                    if (next <= 0x9F) {
                        next += 0x20;
                    }
                    // Р-Я (0xA0-0xAF) -> р-я (0x80-0x8F in D1 range)
                    else if (next >= 0xA0 && next <= 0xAF) {
                        result += static_cast<char>(0xD1);
                        next = next - 0x20;
                        result += static_cast<char>(next);
                        i += 2;
                        continue;
                    }
                }
                result += static_cast<char>(c);
                result += static_cast<char>(next);
                i += 2;
                continue;
            } else if (c == 0xD1 && i + 1 < str.length()) {
                unsigned char next = static_cast<unsigned char>(str[i + 1]);
                // Already lowercase Russian letters
                result += static_cast<char>(c);
                result += static_cast<char>(next);
                i += 2;
                continue;
            }
            
            // Other UTF-8
            UTF8Char utf = getChar(str, i);
            result += utf.chars;
            i += utf.byte_len;
        }
        return result;
    }
};

vector<string> KEYWORDS = { "if", "else", "for", "while", "echo",
    "do", "break", "continue", "let", 
    "static", "final", "const", "global", "typeof", "sizeof",
     "del", "new" ,"true", "false", "null", "ret", "struct",
    "out", "outln", "input", "in" , "and", "or", "namespace", "assert", "lambda",  "func", "Func", "exit", "private"};

struct Lexer {
    int line = 1;
    int global_line = 1;
    int pos = 0;           // byte position in file_data
    int pos_in_line = 0;   // character position in current line (UTF-8 safe)
    int saved_pos_in_line = 0;
    string file_path;
    string file_name;

    string main_file_path, file_data;
    int main_file_size;
    vector<Token> tokens;

    Lexer(string main_file_path, string file_data) {
        this->main_file_path = main_file_path;
        
        // Remove trailing whitespace
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

    // Move to next character (byte-wise)
    inline void next() { 
        if (this->pos < this->main_file_size) {
            this->pos++; 
        }
    }
    
    // Move forward by specified number of bytes
    inline void next(int bytes) {
        for (int i = 0; i < bytes; i++) {
            if (this->pos < this->main_file_size) {
                this->pos++;
            }
        }
    }
    
    // Increment position in line (by characters, not bytes)
    inline void next_in_line() { 
        this->pos_in_line++; 
    }
    
    // Move to next line
    inline void next_line() { 
        this->line++; 
        this->pos_in_line = 0;
        this->global_line++;
    }
    
    // Get character at current position (returns full UTF-8 char as string)
    inline string get_char() {
        if (this->pos >= this->main_file_size) return "";
        UTF8Char utf = UTF8Helper::getChar(this->file_data, this->pos);
        return utf.chars;
    }
    
    // Get character at offset (returns full UTF-8 char as string)
    inline string get_char(int offset) {
        int new_pos = this->pos + offset;
        if (new_pos >= this->main_file_size) return "";
        UTF8Char utf = UTF8Helper::getChar(this->file_data, new_pos);
        return utf.chars;
    }
    
    // Get raw byte at position
    inline char get_byte(int offset = 0) { 
        return this->file_data[this->pos + offset]; 
    }
    
    // Get current UTF-8 character info
    inline UTF8Char get_current_utf8() {
        return UTF8Helper::getChar(this->file_data, this->pos);
    }
    
    // Get length of current character in bytes
    inline int get_current_char_len() {
        UTF8Char utf = get_current_utf8();
        return utf.byte_len;
    }

    /*
        Adding token to tokens vector
    */
    void add_token(string value, TokenType type, PosInFile pos) {
        Token T(type, value, pos);
        tokens.push_back(T);
    }

    /*
        Parse literal (identifier)
    */
    void parse_literal() {
        int         P = this->pos;           // byte position
        int         PL = this->pos_in_line;  // character position in line
        string      V;
        
        while (this->pos < this->main_file_size) {
            string current_char = this->get_char();
            if (current_char.empty()) break;
            
            bool is_valid = false;
            
            // Check if it's a letter (Latin or Russian)
            if (UTF8Helper::isLetter(this->file_data, this->pos)) {
                is_valid = true;
            }
            // Check for underscore
            else if (current_char == "_") {
                is_valid = true;
            }
            // Check for digit (only after first character)
            else if (!V.empty() && UTF8Helper::isDigit(this->file_data, this->pos)) {
                is_valid = true;
            }
            
            if (is_valid) {
                V += current_char;
                int char_len = this->get_current_char_len();
                this->next(char_len);
                this->next_in_line();
            } else {
                break;
            }
        }
        
        int L = this->pos - P;  // byte length
        int char_count = static_cast<int>(V.length()); // approximate, but fine for display
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = char_count,
        };
        
    
        
        if (InVector(KEYWORDS, V)) 
            this->add_token(V, TokenType::KEYWORD, PIF);
        else 
            this->add_token(V, TokenType::LITERAL, PIF);
    }

    /*
        Parse number
    */
    void parse_number() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V;
        char        C = this->get_byte();

        if (C == '.') {
            V += "0.";
            next();
            next_in_line();
            C = this->get_byte();
        }
        
        while (UTF8Helper::isDigit(this->file_data, this->pos) || C == '.') {
            V += C;
            this->next();
            this->next_in_line();
            C = this->get_byte();
        }

        int L = this->pos - P;
        int char_count = static_cast<int>(V.length());
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = char_count
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
        string      quote = this->get_char(); // " or '
        
        this->next(static_cast<int>(quote.length()));
        this->next_in_line();
        
        while (this->pos < this->main_file_size) {
            string current_char = this->get_char();
            
            // Check for closing quote
            if (current_char == quote) {
                this->next(static_cast<int>(quote.length()));
                this->next_in_line();
                break;
            }
            
            // Handle escape sequences
            if (current_char == "\\") {
                this->next(1);
                this->next_in_line();
                string escaped = this->get_char();
                
                if (escaped == "n") V += '\n';
                else if (escaped == "t") V += '\t';
                else if (escaped == "r") V += '\r';
                else if (escaped == "\\") V += '\\';
                else if (escaped == "\"") V += '"';
                else if (escaped == "'") V += '\'';
                else if (escaped == "0") V += '\0';
                else if (escaped == "x") {
                    // Hex escape: \xHH
                    this->next(1);
                    this->next_in_line();
                    string hex1 = this->get_char();
                    this->next(1);
                    this->next_in_line();
                    string hex2 = this->get_char();
                    
                    string hexStr = hex1 + hex2;
                    int hexVal = stoi(hexStr, nullptr, 16);
                    V += (char)hexVal;
                }
                else {
                    V += '\\';
                    V += escaped;
                }
                
                this->next(static_cast<int>(escaped.length()));
                this->next_in_line();
            } 
            else if (current_char == "\n") {
                // Unclosed string literal
                V += current_char;
                this->next(1);
                this->next_line();
            }
            else {
                V += current_char;
                this->next(static_cast<int>(current_char.length()));
                this->next_in_line();
            }
        }
        
        int L = this->pos - P;
        int char_count = static_cast<int>(V.length());
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = SL,
            .global_line = global_line,
            .index = PL,
            .lenght = char_count + 2
        };
        
        TokenType T = (V.length() == 1) ? TokenType::CHAR : TokenType::STRING;
        this->add_token(V, T, PIF);
    }

    /*
        Parse separator (; or :)
    */
    void parse_separator() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      C = this->get_char();
        
        this->next(static_cast<int>(C.length()));
        this->next_in_line();
        
        TokenType T = (C == ";") ? TokenType::DAC :
                      (C == ":") ? TokenType::DAD : 
                                   TokenType::DUMMY;
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = 1
        };
        
        this->add_token(C, T, PIF);
    }

    /*
        Parse operator
    */
    void parse_operator() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V;
        
        while (this->pos < this->main_file_size) {
            string current_char = this->get_char();
            if (current_char.empty()) break;
            
            // Check if it's an operator character
            if (current_char.length() == 1 && 
                InString(current_char[0], APT::OPER)) {
                V += current_char;
                this->next(1);
                this->next_in_line();
            } else {
                break;
            }
        }
        
        int L = this->pos - P;
        int char_count = static_cast<int>(V.length());
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = char_count
        };
        this->add_token(V, TokenType::OPERATOR, PIF);
    }

    /*
        Parse dereference (*)
    */
    void parse_dereference() {
        int         P = this->pos;
        int         PL = this->pos_in_line;
        string      V = "*";
        
        this->next(1);
        this->next_in_line();
        
        int L = this->pos - P;
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = 1
        };
        this->add_token(V, TokenType::DEREFERENCE, PIF);
    }

    /*
        Parse bracket
    */
    void parse_bracket() {
        int PL = this->pos_in_line;
        string C = this->get_char();
        TokenType T;
        
        if (C == "(") T = TokenType::L_BRACKET;
        else if (C == ")") T = TokenType::R_BRACKET;
        else if (C == "[") T = TokenType::L_RECT_BRACKET;
        else if (C == "]") T = TokenType::R_RECT_BRACKET;
        else if (C == "{") T = TokenType::L_CURVE_BRACKET;
        else if (C == "}") T = TokenType::R_CURVE_BRACKET;
        else T = TokenType::DUMMY;
        
        this->next(static_cast<int>(C.length()));
        this->next_in_line();
        
        PosInFile PIF = {
            .file_path = this->file_path,
            .file_name = this->file_name,
            .line = this->line,
            .global_line = global_line,
            .index = PL,
            .lenght = 1
        };
        
        this->add_token(C, T, PIF);
    }

    /*
        Parse single-line comment (// ...)
    */
    void parse_comment() {
        while (this->pos < this->main_file_size) {
            string current_char = this->get_char();
            if (current_char == "\n") {
                break;
            }
            this->next(static_cast<int>(current_char.length()));
            this->next_in_line();
        }
    }

    /*
        Parse multi-line comment (slash star ... star slash)
    */
    void parse_multiline_comment() {
        this->next(2); // Skip /*
        this->next_in_line();
        this->next_in_line();
        
        while (this->pos < this->main_file_size) {
            string current_char = this->get_char();
            string next_chars = this->get_char(1);
            
            if (current_char == "*" && next_chars == "/") {
                this->next(2);
                this->next_in_line();
                this->next_in_line();
                break;
            }
            
            if (current_char == "\n") {
                this->next(1);
                this->next_line();
            } else {
                this->next(static_cast<int>(current_char.length()));
                this->next_in_line();
            }
        }
    }

    void run() {
        while (pos < this->main_file_size) {
            string current_char = this->get_char();
            
            if (current_char.empty()) break;
            
            // Handle newline
            if (current_char == "\n") {
                this->next(1);
                this->next_line();
                continue;
            }
            
            // Handle whitespace
            if (current_char == " " || current_char == "\t" || current_char == "\r") {
                this->next(static_cast<int>(current_char.length()));
                this->next_in_line();
                continue;
            }
            
            // Handle include directives <start "file">
            if (current_char == "<" && 
                IsSubString(this->file_data, "start", this->pos + 1)) {
                this->next(13);
                string new_file_path;
                
                // Parse new included file name
                while (this->pos < this->main_file_size) {
                    string c = this->get_char();
                    if (c == "\"") {
                        this->next(1);
                        break;
                    }
                    new_file_path += c;
                    this->next(static_cast<int>(c.length()));
                }
                
                this->file_path = new_file_path;
                this->file_name = GetFileName(this->file_path);
                this->line = 1;
                
                // Find newline
                while (this->pos < this->main_file_size && this->get_char() != "\n") {
                    this->next(static_cast<int>(this->get_char().length()));
                }
                this->next(1); // pass newline
                this->global_line++;
                continue;
            }
            
            // Handle include end directives <end "file"=line>
            if (current_char == "<" && 
                IsSubString(this->file_data, "end", this->pos + 1)) {
                this->next(11);
                string new_file_path;
                
                // Parse saved included file name
                while (this->pos < this->main_file_size) {
                    string c = this->get_char();
                    if (c == "\"") {
                        this->next(1);
                        break;
                    }
                    new_file_path += c;
                    this->next(static_cast<int>(c.length()));
                }
                
                this->file_path = new_file_path;
                this->file_name = GetFileName(this->file_path);
                
                // Find '='
                while (this->pos < this->main_file_size && this->get_char() != "=") {
                    this->next(static_cast<int>(this->get_char().length()));
                }
                this->next(1); // pass '='
                
                // Parse saved line number
                string saved_line_num;
                while (this->pos < this->main_file_size && 
                       UTF8Helper::isDigit(this->file_data, this->pos)) {
                    saved_line_num += this->get_char();
                    this->next(1);
                }
                
                if (!saved_line_num.empty()) {
                    this->line = atoi(saved_line_num.c_str()) + 1;
                }
                
                // Find newline
                while (this->pos < this->main_file_size && this->get_char() != "\n") {
                    this->next(static_cast<int>(this->get_char().length()));
                }
                this->next(1);
                this->global_line++;
                continue;
            }
            
            // Handle identifiers (literals)
            if (UTF8Helper::isLetter(this->file_data, this->pos) || current_char == "_") {
                parse_literal();
                continue;
            }
            
            // Handle numbers
            if (UTF8Helper::isDigit(this->file_data, this->pos) || 
                (current_char == "." && this->pos + 1 < this->main_file_size && 
                 UTF8Helper::isDigit(this->file_data, this->pos + 1))) {
                parse_number();
                continue;
            }
            
            // Handle strings and chars
            if (current_char == "\"" || current_char == "'") {
                parse_string_or_char();
                continue;
            }
            
            // Handle separators
            if (current_char == ";" || current_char == ":") {
                parse_separator();
                continue;
            }
            
            // Handle dereference
            if (current_char == "*") {
                parse_dereference();
                continue;
            }
            
            // Handle single-line comment
            if (current_char == "/" && this->get_char(1) == "/") {
                parse_comment();
                continue;
            }
            
            // Handle multi-line comment
            if (current_char == "/" && this->get_char(1) == "*") {
                parse_multiline_comment();
                continue;
            }
            
            // Handle operators
            if (current_char.length() == 1 && InString(current_char[0], APT::OPER)) {
                parse_operator();
                continue;
            }
            
            // Handle brackets
            if (UTF8Helper::isBracket(this->file_data, this->pos)) {
                parse_bracket();
                continue;
            }
            
            // Unknown character - skip it (maybe add error token)
            this->next(static_cast<int>(current_char.length()));
            this->next_in_line();
        }
        
        // Add END_OF_FILE token
        if (!tokens.empty()) {
            Token& last_token = tokens.back();
            PosInFile last_pos = last_token.pif;
            int last_char_index = last_pos.index + last_pos.lenght;
            
            PosInFile eof_pos = {
                .file_path = last_pos.file_path,
                .file_name = last_pos.file_name,
                .line = last_pos.line,
                .global_line = last_pos.global_line,
                .index = last_pos.index + last_pos.lenght - 1,  // Last character of last token
                .lenght = 1  // Length 1 to indicate it points to a character
            };
            add_token("END_OF_FILE", TokenType::END_OF_FILE, eof_pos);
        } else {
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