#include "twist-generator.cpp"
#include "twist-nodetemp.cpp"
#include <string>
#include <sstream>
#include <vector>

// ANSI цветовые коды
namespace Colors {
    const std::string RESET = "\033[0m";
    
    // Основные цвета
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string GRAY = "\033[90m";
    
    // Жирные цвета
    const std::string BOLD_RED = "\033[1;31m";
    const std::string BOLD_GREEN = "\033[1;32m";
    const std::string BOLD_YELLOW = "\033[1;33m";
    const std::string BOLD_BLUE = "\033[1;34m";
    const std::string BOLD_MAGENTA = "\033[1;35m";
    const std::string BOLD_CYAN = "\033[1;36m";
    const std::string BOLD_WHITE = "\033[1;37m";
    
    // Подчеркнутые
    const std::string UNDERLINE = "\033[4m";
    
    // Фоны
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_MAGENTA = "\033[45m";
    const std::string BG_CYAN = "\033[46m";
    
    // Цвета для разных типов узлов
    const std::string TYPE_COLOR = BOLD_CYAN;
    const std::string VALUE_COLOR = BOLD_YELLOW;
    const std::string STRING_COLOR = GREEN;
    const std::string NUMBER_COLOR = CYAN;
    const std::string KEYWORD_COLOR = BOLD_MAGENTA;
    const std::string OPERATOR_COLOR = BOLD_BLUE;
    const std::string IDENTIFIER_COLOR = BOLD_WHITE;
    const std::string MODIFIER_COLOR = GRAY;
    const std::string ERROR_COLOR = BOLD_RED;
    const std::string STRUCTURE_COLOR = BOLD_GREEN;
    const std::string META_COLOR = MAGENTA;
}

class ASTPrinter {
private:
    std::stringstream output;
    bool use_colors = true;
    
    std::string colorize(const std::string& text, const std::string& color) {
        if (!use_colors) return text;
        return color + text + Colors::RESET;
    }
    
    std::string escape_string(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '\n': result += "\\n"; break;
                case '\t': result += "\\t"; break;
                case '\r': result += "\\r"; break;
                case '\"': result += "\\\""; break;
                case '\'': result += "\\'"; break;
                case '\\': result += "\\\\"; break;
                default: result += c;
            }
        }
        return result;
    }
    
    void print_node(Node* node, const std::string& prefix, bool is_last) {
        if (!node) {
            output << prefix << (is_last ? "└── " : "├── ") 
                   << colorize("(null)", Colors::ERROR_COLOR) << std::endl;
            return;
        }
        
        // Вывод текущего узла
        output << prefix << (is_last ? "└── " : "├── ");
        
        switch (node->NODE_TYPE) {
            case NodeTypes::NODE_NUMBER: {
                auto* n = static_cast<NodeNumber*>(node);
                output << colorize("Number", Colors::NUMBER_COLOR) << ": ";
                if (n->value.type == STANDART_TYPE::INT) {
                    output << colorize(std::to_string(any_cast<int64_t>(n->value.data)), Colors::VALUE_COLOR);
                } else if (n->value.type == STANDART_TYPE::DOUBLE) {
                    std::stringstream ss;
                    ss << any_cast<long double>(n->value.data);
                    output << colorize(ss.str(), Colors::VALUE_COLOR);
                }
                output << " (" << colorize(n->value.type.pool, Colors::TYPE_COLOR) << ")";
                break;
            }
            
            case NodeTypes::NODE_STRING: {
                auto* n = static_cast<NodeString*>(node);
                output << colorize("String", Colors::STRING_COLOR) << ": "
                       << colorize("\"" + escape_string(any_cast<std::string>(n->value.data)) + "\"", 
                                 Colors::VALUE_COLOR);
                break;
            }
            
            case NodeTypes::NODE_CHAR: {
                auto* n = static_cast<NodeChar*>(node);
                output << colorize("Char", Colors::STRING_COLOR) << ": "
                       << colorize("'" + escape_string(std::string(1, any_cast<char>(n->char_value))) + "'", 
                                 Colors::VALUE_COLOR);
                break;
            }
            
            case NodeTypes::NODE_BOOL: {
                auto* n = static_cast<NodeBool*>(node);
                output << colorize("Bool", Colors::KEYWORD_COLOR) << ": "
                       << colorize((any_cast<bool>(n->value.data) ? "true" : "false"), 
                                 Colors::VALUE_COLOR);
                break;
            }
            
            case NodeTypes::NODE_NULL: {
                output << colorize("Null", Colors::KEYWORD_COLOR);
                break;
            }
            
            case NodeTypes::NODE_LITERAL: {
                auto* n = static_cast<NodeLiteral*>(node);
                output << colorize("Literal", Colors::IDENTIFIER_COLOR) << ": "
                       << colorize(n->name, Colors::VALUE_COLOR);
                break;
            }
            
            case NodeTypes::NODE_VALUE_HOLDER: {
                output << colorize("ValueHolder", Colors::META_COLOR);
                break;
            }
            
            case NodeTypes::NODE_NAME_RESOLUTION: {
                auto* n = static_cast<NodeNameResolution*>(node);
                output << colorize("NameResolution", Colors::STRUCTURE_COLOR) << ": "
                       << colorize(n->current_name, Colors::IDENTIFIER_COLOR);
                if (!n->remaining_chain.empty()) {
                    output << " " << colorize("->", Colors::OPERATOR_COLOR) << " ";
                    for (size_t i = 0; i < n->remaining_chain.size(); i++) {
                        output << colorize(n->remaining_chain[i], Colors::IDENTIFIER_COLOR);
                        if (i != n->remaining_chain.size() - 1) 
                            output << colorize("::", Colors::OPERATOR_COLOR);
                    }
                }
                break;
            }
            
            case NodeTypes::NODE_SCOPES: {
                auto* n = static_cast<NodeScopes*>(node);
                output << colorize("Scopes", Colors::STRUCTURE_COLOR);
                output << std::endl;
                print_node(n->expression.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_UNARY: {
                auto* n = static_cast<NodeUnary*>(node);
                output << colorize("Unary", Colors::OPERATOR_COLOR) << ": "
                       << colorize(n->op, Colors::VALUE_COLOR);
                output << std::endl;
                print_node(n->operand.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_BINARY: {
                auto* n = static_cast<NodeBinary*>(node);
                output << colorize("Binary", Colors::OPERATOR_COLOR) << ": "
                       << colorize(n->op, Colors::VALUE_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                print_node(n->left.get(), new_prefix, false);
                output << new_prefix << colorize("└── Right:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->right.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_VARIABLE_DECLARATION: {
                auto* n = static_cast<NodeBaseVariableDecl*>(node);
                output << colorize("VarDecl", Colors::KEYWORD_COLOR) << ": "
                       << colorize(n->var_name, Colors::IDENTIFIER_COLOR);
                
                if (n->is_const) output << " " << colorize("const", Colors::MODIFIER_COLOR);
                if (n->is_static) output << " " << colorize("static", Colors::MODIFIER_COLOR);
                if (n->is_final) output << " " << colorize("final", Colors::MODIFIER_COLOR);
                if (n->is_global) output << " " << colorize("global", Colors::MODIFIER_COLOR);
                if (n->is_private) output << " " << colorize("private", Colors::MODIFIER_COLOR);
                if (n->nullable) output << " " << colorize("nullable", Colors::MODIFIER_COLOR);
                
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                
                if (n->type_expr) {
                    output << new_prefix << colorize("├── TypeExpr:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->type_expr.get(), new_prefix + "│   ", true);
                }
                
                output << new_prefix << colorize("└── ValueExpr:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->value_expr.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_OUT: {
                auto* n = static_cast<NodeBaseOut*>(node);
                output << colorize("Out", Colors::KEYWORD_COLOR) 
                       << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->expression.size()), Colors::NUMBER_COLOR)
                       << colorize(" args)", Colors::GRAY);
                output << std::endl;
                print_children(n->expression, "arguments", prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_OUTLN: {
                auto* n = static_cast<NodeBaseOutLn*>(node);
                output << colorize("OutLn", Colors::KEYWORD_COLOR)
                       << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->expression.size()), Colors::NUMBER_COLOR)
                       << colorize(" args)", Colors::GRAY);
                output << std::endl;
                print_children(n->expression, "arguments", prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_VARIABLE_EQUAL: {
                auto* n = static_cast<NodeVariableEqual*>(node);
                output << colorize("Assignment", Colors::OPERATOR_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Target:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->variable.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Value:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->expression.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_BLOCK_OF_NODES: {
                auto* n = static_cast<NodeBlock*>(node);
                output << colorize("Block", Colors::STRUCTURE_COLOR)
                       << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->nodes_array.size()), Colors::NUMBER_COLOR)
                       << colorize(" statements)", Colors::GRAY);
                output << std::endl;
                print_children(n->nodes_array, "statements", prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_IF: {
                auto* n = static_cast<NodeIf*>(node);
                output << colorize("If", Colors::KEYWORD_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Condition:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->eq_expression.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("├── Then:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->true_statement.get(), new_prefix + "│   ", true);
                
                if (n->else_statement) {
                    output << new_prefix << colorize("└── Else:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->else_statement.get(), new_prefix + "    ", true);
                } else {
                    output << new_prefix << colorize("└── Else: (none)", Colors::GRAY) << std::endl;
                }
                return;
            }
            
            case NodeTypes::NODE_NAMESPACE_DECLARATION: {
                auto* n = static_cast<NodeNamespaceDecl*>(node);
                output << colorize("Namespace", Colors::KEYWORD_COLOR) << ": "
                       << colorize(n->name, Colors::IDENTIFIER_COLOR);
                
                if (n->is_const) output << " " << colorize("const", Colors::MODIFIER_COLOR);
                if (n->is_static) output << " " << colorize("static", Colors::MODIFIER_COLOR);
                if (n->is_final) output << " " << colorize("final", Colors::MODIFIER_COLOR);
                if (n->is_global) output << " " << colorize("global", Colors::MODIFIER_COLOR);
                if (n->is_private) output << " " << colorize("private", Colors::MODIFIER_COLOR);
                
                output << std::endl;
                
                if (n->statement) {
                    print_node(n->statement.get(), prefix + (is_last ? "    " : "│   "), true);
                }
                return;
            }
            
            case NodeTypes::NODE_BREAK: {
                output << colorize("Break", Colors::KEYWORD_COLOR);
                break;
            }
            
            case NodeTypes::NODE_CONTINUE: {
                output << colorize("Continue", Colors::KEYWORD_COLOR);
                break;
            }
            
            case NodeTypes::NODE_WHILE: {
                auto* n = static_cast<NodeWhile*>(node);
                output << colorize("While", Colors::KEYWORD_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Condition:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->eq_expression.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Body:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->statement.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_DO_WHILE: {
                auto* n = static_cast<NodeDoWhile*>(node);
                output << colorize("DoWhile", Colors::KEYWORD_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Body:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->statement.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Condition:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->eq_expression.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_FOR: {
                auto* n = static_cast<NodeFor*>(node);
                output << colorize("For", Colors::KEYWORD_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Init:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->start_state.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("├── Condition:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->check_expr.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("├── Update:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->update_state.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Body:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->body.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_ADDRESS_OF: {
                auto* n = static_cast<NodeAddressOf*>(node);
                output << colorize("AddressOf", Colors::OPERATOR_COLOR) << " "
                       << colorize("(&)", Colors::VALUE_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_DEREFERENCE: {
                auto* n = static_cast<NodeDereference*>(node);
                output << colorize("Dereference", Colors::OPERATOR_COLOR) << " "
                       << colorize("(*)", Colors::VALUE_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_LEFT_DEREFERENCE: {
                auto* n = static_cast<NodeLeftDereference*>(node);
                output << colorize("LeftDereference", Colors::OPERATOR_COLOR) << " "
                       << colorize("(* =)", Colors::VALUE_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Pointer:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->left_expr.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Value:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->right_expr.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_TYPEOF: {
                auto* n = static_cast<NodeTypeof*>(node);
                output << colorize("Typeof", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_SIZEOF: {
                auto* n = static_cast<NodeSizeof*>(node);
                output << colorize("Sizeof", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_DELETE: {
                auto* n = static_cast<NodeDelete*>(node);
                output << colorize("Delete", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->target.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_IF_EXPRESSION: {
                auto* n = static_cast<NodeIfExpr*>(node);
                output << colorize("IfExpression", Colors::KEYWORD_COLOR) << " "
                       << colorize("(ternary)", Colors::GRAY);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Condition:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->condition.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("├── Then:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->true_expr.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Else:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->else_expr.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_INPUT: {
                auto* n = static_cast<NodeInput*>(node);
                output << colorize("Input", Colors::KEYWORD_COLOR);
                if (n->expr) {
                    output << std::endl;
                    output << prefix + (is_last ? "    " : "│   ") 
                           << colorize("└── Prompt:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->expr.get(), prefix + (is_last ? "    " : "│   ") + "    ", true);
                } else {
                    output << std::endl;
                }
                return;
            }
            
            case NodeTypes::NODE_NAMESPACE_EXPRESSION: {
                auto* n = static_cast<NodeNamespace*>(node);
                output << colorize("NamespaceExpression", Colors::KEYWORD_COLOR) << " "
                       << colorize("(anonymous)", Colors::GRAY);
                output << std::endl;
                
                if (n->statement) {
                    print_node(n->statement.get(), prefix + (is_last ? "    " : "│   "), true);
                }
                return;
            }
            
            case NodeTypes::NODE_ASSERT: {
                auto* n = static_cast<NodeAssert*>(node);
                output << colorize("Assert", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_EXPRESSION_STATEMENT: {
                auto* n = static_cast<NodeExpressionStatement*>(node);
                output << colorize("ExpressionStatement", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_LAMBDA: {
                auto* n = static_cast<NodeLambda*>(node);
                output << colorize("Lambda", Colors::KEYWORD_COLOR);
                if (!n->name.empty()) 
                    output << " " << colorize("[" + n->name + "]", Colors::META_COLOR);
                output << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->args.size()), Colors::NUMBER_COLOR)
                       << colorize(" args)", Colors::GRAY);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                
                // Считаем сколько элементов будет выведено
                int total_items = (n->args.empty() ? 0 : 1) + (n->return_type ? 1 : 0) + (n->body ? 1 : 0);
                int current_item = 0;
                
                // Аргументы
                if (!n->args.empty()) {
                    current_item++;
                    bool args_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (args_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("Arguments:", Colors::STRUCTURE_COLOR) << std::endl;
                    
                    std::string args_prefix = new_prefix + (args_is_last ? "    " : "│   ");
                    for (size_t i = 0; i < n->args.size(); i++) {
                        auto* arg = n->args[i];
                        bool arg_is_last_in_list = (i == n->args.size() - 1);
                        
                        // Определяем, есть ли у этого аргумента тип или значение по умолчанию
                        bool has_type = arg->type_expr != nullptr;
                        bool has_default = arg->default_parameter != nullptr;
                        
                        if (has_type || has_default) {
                            // Имя аргумента с двоеточием
                            output << args_prefix 
                                   << (arg_is_last_in_list ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                                   << colorize(arg->name, Colors::IDENTIFIER_COLOR) << ":" << std::endl;
                            
                            // Префикс для типа/значения по умолчанию
                            std::string child_prefix = args_prefix + (arg_is_last_in_list ? "    " : "│   ");
                            
                            if (has_type && has_default) {
                                // Есть и тип, и значение по умолчанию
                                output << child_prefix << colorize("├── Type:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->type_expr.get(), child_prefix + "│   ", true);
                                output << child_prefix << colorize("└── Default:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->default_parameter.get(), child_prefix + "    ", true);
                            } else if (has_type) {
                                // Только тип
                                output << child_prefix << colorize("└── Type:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->type_expr.get(), child_prefix + "    ", true);
                            } else {
                                // Только значение по умолчанию
                                output << child_prefix << colorize("└── Default:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->default_parameter.get(), child_prefix + "    ", true);
                            }
                        } else {
                            // Если нет детей, просто выводим имя аргумента
                            output << args_prefix 
                                   << (arg_is_last_in_list ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                                   << colorize(arg->name, Colors::IDENTIFIER_COLOR) << std::endl;
                        }
                    }
                }
                
                // Тип возврата
                if (n->return_type) {
                    current_item++;
                    bool return_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (return_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("ReturnType:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string return_prefix = new_prefix + (return_is_last ? "    " : "│   ");
                    print_node(n->return_type.get(), return_prefix, true);
                }
                
                // Тело
                if (n->body) {
                    current_item++;
                    bool body_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (body_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("Body:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string body_prefix = new_prefix + (body_is_last ? "    " : "│   ");
                    print_node(n->body.get(), body_prefix, true);
                }
                
                return;
            }
            
            case NodeTypes::NODE_RETURN: {
                auto* n = static_cast<NodeReturn*>(node);
                output << colorize("Return", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_CALL: {
                auto* n = static_cast<NodeCall*>(node);
                output << colorize("Call", Colors::STRUCTURE_COLOR)
                       << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->args.size()), Colors::NUMBER_COLOR)
                       << colorize(" args)", Colors::GRAY);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                
                if (!n->args.empty()) {
                    // Есть аргументы - Callable не последний
                    output << new_prefix << colorize("├── Callable:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string callable_prefix = new_prefix + "│   ";
                    print_node(n->callable.get(), callable_prefix, true);
                    
                    output << new_prefix << colorize("└── Arguments:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string args_prefix = new_prefix + "    ";
                    for (size_t i = 0; i < n->args.size(); i++) {
                        bool arg_is_last = (i == n->args.size() - 1);
                        print_node(n->args[i].get(), args_prefix, arg_is_last);
                    }
                } else {
                    // Нет аргументов - Callable последний
                    output << new_prefix << colorize("└── Callable:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string callable_prefix = new_prefix + "    ";
                    print_node(n->callable.get(), callable_prefix, true);
                }
                return;
            }
            
            case NodeTypes::NODE_NEW: {
                auto* n = static_cast<NodeNew*>(node);
                output << colorize("New", Colors::KEYWORD_COLOR);
                if (n->is_const) output << " " << colorize("const", Colors::MODIFIER_COLOR);
                if (n->is_static) output << " " << colorize("static", Colors::MODIFIER_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                
                if (n->type_expr) {
                    output << new_prefix;
                    if (n->expr) {
                        output << colorize("├── ", Colors::STRUCTURE_COLOR);
                    } else {
                        output << colorize("└── ", Colors::STRUCTURE_COLOR);
                    }
                    output << colorize("Type:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->type_expr.get(), 
                              new_prefix + (n->expr ? "│   " : "    "), 
                              true);
                }
                
                if (n->expr) {
                    output << new_prefix << colorize("└── Value:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->expr.get(), new_prefix + "    ", true);
                } else if (!n->type_expr) {
                    output << new_prefix << colorize("└── Value: (null)", Colors::GRAY) << std::endl;
                }
                return;
            }
            
            case NodeTypes::NODE_FUNCTION_TYPE: {
                auto* n = static_cast<NodeNewFuncType*>(node);
                output << colorize("FunctionType", Colors::TYPE_COLOR)
                       << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->args_types_expr.size()), Colors::NUMBER_COLOR)
                       << colorize(" params)", Colors::GRAY);
                if (n->return_type_expr) 
                    output << " " << colorize("-> (return)", Colors::OPERATOR_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                
                bool has_params = !n->args_types_expr.empty();
                bool has_return = n->return_type_expr != nullptr;
                
                if (has_params && has_return) {
                    // Есть и параметры, и тип возврата
                    output << new_prefix << colorize("├── Parameters:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string param_prefix = new_prefix + "│   ";
                    for (size_t i = 0; i < n->args_types_expr.size(); i++) {
                        bool param_is_last = (i == n->args_types_expr.size() - 1);
                        print_node(n->args_types_expr[i].get(), param_prefix, param_is_last);
                    }
                    
                    output << new_prefix << colorize("└── ReturnType:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->return_type_expr.get(), new_prefix + "    ", true);
                } else if (has_params && !has_return) {
                    // Только параметры
                    output << new_prefix << colorize("└── Parameters:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string param_prefix = new_prefix + "    ";
                    for (size_t i = 0; i < n->args_types_expr.size(); i++) {
                        bool param_is_last = (i == n->args_types_expr.size() - 1);
                        print_node(n->args_types_expr[i].get(), param_prefix, param_is_last);
                    }
                } else if (!has_params && has_return) {
                    // Только тип возврата
                    output << new_prefix << colorize("└── ReturnType:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->return_type_expr.get(), new_prefix + "    ", true);
                } else {
                    // Ничего нет
                    output << new_prefix << colorize("└── (empty)", Colors::GRAY) << std::endl;
                }
                return;
            }
            
            case NodeTypes::NODE_FUNCTION_DECLARATION: {
                auto* n = static_cast<NodeFuncDecl*>(node);
                output << colorize("Function", Colors::KEYWORD_COLOR) << ": "
                       << colorize(n->name, Colors::IDENTIFIER_COLOR);
                
                if (n->is_const) output << " " << colorize("const", Colors::MODIFIER_COLOR);
                if (n->is_static) output << " " << colorize("static", Colors::MODIFIER_COLOR);
                if (n->is_final) output << " " << colorize("final", Colors::MODIFIER_COLOR);
                if (n->is_global) output << " " << colorize("global", Colors::MODIFIER_COLOR);
                if (n->is_private) output << " " << colorize("private", Colors::MODIFIER_COLOR);
                
                output << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->args.size()), Colors::NUMBER_COLOR)
                       << colorize(" args)", Colors::GRAY);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                
                // Считаем сколько элементов будет выведено
                int total_items = 0;
                if (!n->args.empty()) total_items++;           // Arguments блок
                if (n->return_type) total_items++;             // ReturnType блок
                if (n->body) total_items++;                    // Body блок
                if (n->args.empty() && !n->return_type && !n->body) total_items++; // Пустой случай
                
                int current_item = 0;
                
                // Аргументы
                if (!n->args.empty()) {
                    current_item++;
                    bool args_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (args_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("Arguments:", Colors::STRUCTURE_COLOR) << std::endl;
                    
                    std::string args_prefix = new_prefix + (args_is_last ? "    " : "│   ");
                    for (size_t i = 0; i < n->args.size(); i++) {
                        auto* arg = n->args[i];
                        bool arg_is_last_in_list = (i == n->args.size() - 1);
                        
                        // Определяем, есть ли у этого аргумента тип или значение по умолчанию
                        bool has_type = arg->type_expr != nullptr;
                        bool has_default = arg->default_parameter != nullptr;
                        
                        if (has_type || has_default) {
                            // Имя аргумента с двоеточием
                            output << args_prefix 
                                   << (arg_is_last_in_list ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                                   << colorize(arg->name, Colors::IDENTIFIER_COLOR) << ":" << std::endl;
                            
                            // Префикс для типа/значения по умолчанию
                            std::string child_prefix = args_prefix + (arg_is_last_in_list ? "    " : "│   ");
                            
                            if (has_type && has_default) {
                                // Есть и тип, и значение по умолчанию
                                output << child_prefix << colorize("├── Type:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->type_expr.get(), child_prefix + "│   ", true);
                                output << child_prefix << colorize("└── Default:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->default_parameter.get(), child_prefix + "    ", true);
                            } else if (has_type) {
                                // Только тип
                                output << child_prefix << colorize("└── Type:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->type_expr.get(), child_prefix + "    ", true);
                            } else {
                                // Только значение по умолчанию
                                output << child_prefix << colorize("└── Default:", Colors::STRUCTURE_COLOR) << std::endl;
                                print_node(arg->default_parameter.get(), child_prefix + "    ", true);
                            }
                        } else {
                            // Если нет детей, просто выводим имя аргумента
                            output << args_prefix 
                                   << (arg_is_last_in_list ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                                   << colorize(arg->name, Colors::IDENTIFIER_COLOR) << std::endl;
                        }
                    }
                }
                
                // Тип возврата
                if (n->return_type) {
                    current_item++;
                    bool return_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (return_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("ReturnType:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string return_prefix = new_prefix + (return_is_last ? "    " : "│   ");
                    print_node(n->return_type.get(), return_prefix, true);
                }
                
                // Тело
                if (n->body) {
                    current_item++;
                    bool body_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (body_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("Body:", Colors::STRUCTURE_COLOR) << std::endl;
                    std::string body_prefix = new_prefix + (body_is_last ? "    " : "│   ");
                    print_node(n->body.get(), body_prefix, true);
                } else if (!n->args.empty() && !n->return_type) {
                    // Если есть аргументы или тип возврата, но нет тела
                    current_item++;
                    bool body_is_last = (current_item == total_items);
                    output << new_prefix 
                           << (body_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                           << colorize("Body: (none)", Colors::GRAY) << std::endl;
                }
                
                return;
            }
            
            case NodeTypes::NODE_EXIT: {
                auto* n = static_cast<NodeExit*>(node);
                output << colorize("Exit", Colors::KEYWORD_COLOR);
                output << std::endl;
                print_node(n->expr.get(), prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_ARRAY_TYPE: {
                auto* n = static_cast<NodeNewArrayType*>(node);
                output << colorize("ArrayType", Colors::TYPE_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix;
                if (n->size_expr) {
                    output << colorize("├── ", Colors::STRUCTURE_COLOR);
                } else {
                    output << colorize("└── ", Colors::STRUCTURE_COLOR);
                }
                output << colorize("ElementType:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->type_expr.get(), 
                          new_prefix + (n->size_expr ? "│   " : "    "), 
                          true);
                
                if (n->size_expr) {
                    output << new_prefix << colorize("└── Size:", Colors::STRUCTURE_COLOR) << std::endl;
                    print_node(n->size_expr.get(), new_prefix + "    ", true);
                } else {
                    output << new_prefix << colorize("└── Size: dynamic", Colors::GRAY) << std::endl;
                }
                return;
            }
            
            case NodeTypes::NODE_ARRAY: {
                auto* n = static_cast<NodeArray*>(node);
                output << colorize("Array", Colors::STRUCTURE_COLOR)
                       << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->elements.size()), Colors::NUMBER_COLOR)
                       << colorize(" elements)", Colors::GRAY);
                output << std::endl;
                print_children(n->elements, "elements", prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_GET_BY_INDEX: {
                auto* n = static_cast<NodeGetIndex*>(node);
                output << colorize("ArrayIndex", Colors::STRUCTURE_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Array:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->expr.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Index:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->index_expr.get(), new_prefix + "    ", true);
                return;
            }
            
            case NodeTypes::NODE_BLOCK_OF_DECLARATIONS: {
                auto* n = static_cast<NodeBlockDecl*>(node);
                output << colorize("BlockDecl", Colors::STRUCTURE_COLOR);
                
                if (n->is_const) output << " " << colorize("const", Colors::MODIFIER_COLOR);
                if (n->is_static) output << " " << colorize("static", Colors::MODIFIER_COLOR);
                if (n->is_final) output << " " << colorize("final", Colors::MODIFIER_COLOR);
                if (n->is_global) output << " " << colorize("global", Colors::MODIFIER_COLOR);
                if (n->is_private) output << " " << colorize("private", Colors::MODIFIER_COLOR);
                
                output << colorize(" (", Colors::GRAY) 
                       << colorize(std::to_string(n->decls.size()), Colors::NUMBER_COLOR)
                       << colorize(" declarations)", Colors::GRAY);
                output << std::endl;
                print_children(n->decls, "declarations", prefix + (is_last ? "    " : "│   "), true);
                return;
            }
            
            case NodeTypes::NODE_ARRAY_PUSH: {
                auto* n = static_cast<NodeArrayPush*>(node);
                output << colorize("ArrayPush", Colors::OPERATOR_COLOR) << " "
                       << colorize("(<-)", Colors::VALUE_COLOR);
                output << std::endl;
                
                std::string new_prefix = prefix + (is_last ? "    " : "│   ");
                output << new_prefix << colorize("├── Array:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->left_expr.get(), new_prefix + "│   ", true);
                output << new_prefix << colorize("└── Value:", Colors::STRUCTURE_COLOR) << std::endl;
                print_node(n->right_expr.get(), new_prefix + "    ", true);
                return;
            }
            
            default:
                output << colorize("UnknownNode[", Colors::ERROR_COLOR) 
                       << colorize(std::to_string(node->NODE_TYPE), Colors::NUMBER_COLOR)
                       << colorize("]", Colors::ERROR_COLOR);
                break;
        }
        
        output << std::endl;
    }
    
    void print_children(const std::vector<std::unique_ptr<Node>>& children, 
                       const std::string& label,
                       const std::string& prefix,
                       bool parent_is_last) {
        if (children.empty()) return;
        
        std::string new_prefix = prefix;
        if (!label.empty()) {
            output << prefix << (parent_is_last ? colorize("└── ", Colors::STRUCTURE_COLOR) : colorize("├── ", Colors::STRUCTURE_COLOR))
                   << colorize("[", Colors::META_COLOR)
                   << colorize(label, Colors::NUMBER_COLOR)
                   << colorize("]", Colors::META_COLOR) << std::endl;
            new_prefix = prefix + (parent_is_last ? "    " : "│   ");
        }
        
        for (size_t i = 0; i < children.size(); i++) {
            bool is_last = (i == children.size() - 1);
            print_node(children[i].get(), new_prefix, is_last);
        }
    }
    
public:
    void disable_colors() {
        use_colors = false;
    }
    
    void enable_colors() {
        use_colors = true;
    }
    
    void print_ast(const std::vector<std::unique_ptr<Node>>& nodes, const std::string& title = "AST Tree") {
        output.str("");
        output.clear();
        for (size_t i = 0; i < nodes.size(); i++) {
            bool is_last = (i == nodes.size() - 1);
            print_node(nodes[i].get(), "", is_last);
        }
    }
    
    std::string get_output() const {
        return output.str();
    }
    
    void print_to_console(const std::vector<std::unique_ptr<Node>>& nodes, const std::string& title = "AST Tree") {
        print_ast(nodes, title);
        std::cout << get_output() << std::endl;
    }
};

// Пример использования в ASTGenerator:
void debug_print_ast(Context& generator, bool use_colors = true) {
    ASTPrinter printer;
    if (!use_colors) {
        printer.disable_colors();
    }
    printer.print_to_console(generator.nodes, "Twist Language AST");
}