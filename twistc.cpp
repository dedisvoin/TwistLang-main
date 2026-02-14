#include "src/twist-ast-printer.cpp"
#include "src/twist-preproc.cpp"
#include "src/twist-lexer.cpp"
#include "src/twist-utils.cpp"
#include "src/twist-tokenwalker.cpp"
#include "src/twist-generator.cpp"


#include "fstream"
#include <filesystem>
#include <fstream>


Preprocessor preprocessor;

const ArgsParser GenerateArgsParser(const int argc, char** const argv) noexcept {
    auto string_args = ConvertArgs(argc, argv);
    ArgsParser args_parser = ArgsParser(string_args);
    args_parser.Parse();
    args_parser.FileIsExist();
    return args_parser;
}


int main(int argc, char** argv) {
    std::locale::global(std::locale("ru_RU.UTF-8"));
    std::wcout.imbue(std::locale());
    // Create and generate argparser for parse system arguments
    static ArgsParser args_parser = GenerateArgsParser(argc, argv);

    // Open main file and read it
    static string file_content = OpenFile(args_parser.file_path);

    // Full preprocessing
    // - Execute include directives
    // - Macroses
    // - Defines
    static string preprocessor_output;
    TimeIt("Preprocessing finished in ", []() {
        preprocessor_output = preprocessor.process(file_content, args_parser.file_path);
    });

    ERROR::PREPROCESSOR_OUTPUT = preprocessor_output;

    if (!args_parser.compile_mod) {
        // Save preprocessed file if flag setted
        if (args_parser.save_preprocessed)
            SavePreprocessedFile("output.twist", preprocessor_output);

        // Create Parser object
        static Lexer parser = Lexer(args_parser.file_path, preprocessor_output);

        // Run parsing
        TimeIt("Parse finished in ", [](){
            parser.run();
        });

        // Save tokens to file if flag setted
        if (args_parser.save_token)
            SaveTokensFile("tokens.txt", parser.tokens);

        TokenWalker walker = TokenWalker(&parser.tokens);
        ASTGenerator generator = ASTGenerator(walker, args_parser.file_path);
        
        auto context = generator.run();
        if (args_parser.print_ast) {
            debug_print_ast(context);
            return 0;
        }
        
        static ContextExecutor executor = ContextExecutor(std::move(context));

        
        if (args_parser.math_middle_run_time) {
            middleTimeIt("Middle interpretation time compute", [](){
                executor.run();
            }, 100);
            exit(0);
        }
        if (args_parser.interp_time)
            TimeIt("Interpretation finished in ", [](){
            executor.run();
        });
        else
            executor.run();
    } else {
        // Компиляторный режим
        
        std::filesystem::path path_obj(args_parser.file_path);
        static std::string stem = "compiled_" + path_obj.stem().string() + ".cpp";
        std::ofstream out_file(stem);
        

        if (out_file.is_open()) {
            // Просто записываем все напрямую
            out_file << "#include \"src/twist-lexer.cpp\"\n";
            out_file << "#include \"src/twist-tokenwalker.cpp\"\n";
            out_file << "#include \"src/twist-generator.cpp\"\n";
            out_file << "\n";
            out_file << "int main() {\n";
            out_file << "    std::string file_path = \"" << args_parser.file_path << "\";\n";
            out_file << "    std::string preprocessor_output = R\"(" << preprocessor_output << ")\";\n";
            out_file << "    ERROR::PREPROCESSOR_OUTPUT = preprocessor_output;\n";
            out_file << "    static Lexer parser = Lexer(file_path, preprocessor_output);\n";
            out_file << "    parser.run();\n";
            out_file << "    \n";
            out_file << "    TokenWalker walker = TokenWalker(&parser.tokens);\n";
            out_file << "    ASTGenerator generator = ASTGenerator(walker, file_path);\n";
            out_file << "    auto context = generator.run();\n";
            out_file << "    static ContextExecutor executor = ContextExecutor(std::move(context));\n";
            out_file << "    \n";
            out_file << "    executor.run();\n";
            out_file << "    return 0;\n";
            out_file << "}\n";

            out_file.close();
            
            string command = "clang -O3 -std=c++23 " + stem + " -o main.exe";
            TimeIt("Compilation finished in ", [command](){
                system(command.c_str());
                if (args_parser.delete_precompiled)
                    filesystem::remove(stem.c_str());
            });

        } else {
            std::cout << "Error opening file: " << stem << std::endl;
        }
    }
    return 0;
}
