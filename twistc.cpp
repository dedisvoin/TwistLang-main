
#include "src/twist-preproc.cpp"
#include "src/twist-lexer.cpp"
#include "src/twist-utils.cpp"
#include "src/twist-tokenwalker.cpp"
#include "src/twist-parser.cpp"

#include "fstream"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

Preprocessor preprocessor;

const ArgsParser GenerateArgsParser(const int argc, char** const argv) noexcept {
    auto string_args = ConvertArgs(argc, argv);
    ArgsParser args_parser = ArgsParser(string_args);
    args_parser.Parse();
    args_parser.FileIsExist();
    return args_parser;
}

void run_with(vector<Node*>* nodes, Memory* g_memory) {
    for (size_t i = 0; i < nodes->size(); i++) {
        (*nodes)[i]->exec_from(g_memory);
    }
}


void write_error_to_file(std::ostream& out, const Error& err) {
    out << "pif: " << err.pif << ":" << err.pif.lenght << ":" << err.assertion
        << " message: " << err.message << "\n";
    if (err.sub_error)
        write_error_to_file(out, *err.sub_error);

}

vector<Error> run_with_collect(vector<Node*>& nodes, Memory* mem) {
    vector<Error> collected;
    for (size_t i = 0; i < nodes.size(); ++i) {
        try {
            nodes[i]->exec_from(mem);
        } catch (const Error& err) {
            if (err.assertion) {
                // Assert – сохраняем и продолжаем
                collected.push_back(err);
            } else {
                // Фатальная ошибка – пробрасываем дальше
                throw;
            }
        }
    }
    return collected;
}

void language_server(const std::string& file_path, std::string file_name) {
    using namespace std::chrono_literals;
    Preprocessor preprocessor;

    while (true) {
        try {
            std::string source = OpenFile(file_path);
            
            ERROR::PREPROCESSOR_OUTPUT = source;
            ERROR_THROW::PREPROCESSOR_OUTPUT = source;

            std::string preprocessed = preprocessor.process(source, file_path);

            // 2. Лексический анализ
            Lexer lexer(file_path, preprocessed);
            lexer.run();

            // 3. Синтаксический анализ
            TokenWalker walker(&lexer.tokens);
            ASTGenerator parser(walker, file_path);
            parser.parse();  // если здесь ошибка – будет исключение

            // 4. Подготовка памяти и выполнение
            auto nodes = std::move(parser.nodes);
            Memory* g_memory = new Memory();
            GenerateStandartTypes(g_memory, file_path);

            // Выполняем с накоплением assert'ов
            auto assert_errors = run_with_collect(nodes, g_memory);

            
            if (assert_errors.empty()) {
                // Нет ошибок – очищаем файл
                std::ofstream clear_log(string("dbg/") + file_name  + "_ls.dbg", std::ios::trunc);
                clear_log.close();
            } else {
                // Есть assert'ы – перезаписываем файл всеми
                std::ofstream log(string("dbg/") + file_name + "_ls.dbg", std::ios::trunc);
                for (const auto& err : assert_errors) {
                    write_error_to_file(log, err);
                    log << "\n";
                }
            }

        } catch (const Error& err) {
            // Фатальная ошибка (не assertion) – записываем её одну
            std::ofstream log(string("dbg/") + file_name + "_ls.dbg", std::ios::trunc);
            write_error_to_file(log, err);
            log << "\n";
        } catch (const std::exception& e) {
            // Непредвиденная ошибка – можно проигнорировать или записать
            // (по желанию)
        }

        std::this_thread::sleep_for(0.1s);
    }
}




int main(int argc, char** argv) {
    std::locale::global(std::locale("ru_RU.UTF-8"));
    std::wcout.imbue(std::locale());
    // Create and generate argparser for parse system arguments
    static ArgsParser args_parser = GenerateArgsParser(argc, argv);

    if (args_parser.as_debuger) {
        language_server(args_parser.file_path, args_parser.file_name);
        return 0;
    } else {
        // Open main file and read it
        static string file_content = OpenFile(args_parser.file_path);

        // Full preprocessing
        static string preprocessor_output;
        TimeIt("Preprocessing finished in ", []() {
            preprocessor_output = preprocessor.process(file_content, args_parser.file_path);
        });

        ERROR::PREPROCESSOR_OUTPUT = preprocessor_output;
        ERROR_THROW::PREPROCESSOR_OUTPUT = preprocessor_output;

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

            try {
                generator.parse();
            } catch (Error& err) {
                err.print();
            }

            // if (args_parser.save_ast) {
            //     save_ast_to_file(generator.nodes, "ast.txt");
            // }
            // if (args_parser.print_ast) {
            //     debug_print_ast(generator.nodes);
            //     return 0;
            // }

            auto nodes = std::move(generator.nodes);
            auto g_memory = new Memory();
            GenerateStandartTypes(g_memory, args_parser.file_path);


            if (args_parser.middle_run_time) {
                middleTimeIt("Middle interpretation time compute", [&](){
                    GenerateStandartTypes(g_memory, args_parser.file_path);
                    run_with(&nodes, g_memory);
                    g_memory->clear();
                }, 100);
                exit(0);
            } else {
                if (args_parser.run_time)
                    TimeIt("Parse finished in ", [&](){
                        try {
                            run_with(&nodes, g_memory);
                        } catch (Error& err) {
                            err.print();
                        }
                    });
                else
                    try {
                        run_with(&nodes, g_memory);
                    } catch (Error& err) {
                        err.print();
                    }
            }


        } else {
            // Компиляторный режим

            std::filesystem::path path_obj(args_parser.file_path);
            static std::string stem = "compiled_" + path_obj.stem().string() + ".cpp";
            std::ofstream out_file(stem);


            if (out_file.is_open()) {
                // Просто записываем все напрямую
                out_file << "// Подключаем реализацию лексера, обходчика токенов и парсера\n";
                out_file << "#include \"src/twist-lexer.cpp\"\n";
                out_file << "#include \"src/twist-tokenwalker.cpp\"\n";
                out_file << "#include \"src/twist-parser.cpp\"\n";
                out_file << "\n";
                out_file << "// Функция для выполнения списка узлов AST в заданном контексте памяти\n";
                out_file << "void run_with(vector<unique_ptr<Node>>* nodes, Memory& g_memory) {\n";
                out_file << "    for (size_t i = 0; i < nodes->size(); i++) {\n";
                out_file << "        // Выполняем код каждого узла в глобальной памяти\n";
                out_file << "        (*nodes)[i]->exec_from(g_memory);\n";
                out_file << "    }\n";
                out_file << "}\n";
                out_file << "\n";
                out_file << "// Точка входа в сгенерированный исполняемый файл\n";
                out_file << "int main() {\n";
                out_file << "    // Сохраняем путь к исходному файлу\n";
                out_file << "    std::string file_path = \"" << args_parser.file_path << "\";\n";
                out_file << "    \n";
                out_file << "    // Сохраняем вывод препроцессора как строковый литерал (raw string)\n";
                out_file << "    std::string preprocessor_output = R\"twist(" << preprocessor_output << ")twist\";\n";
                out_file << "    \n";
                out_file << "    // Устанавливаем глобальный вывод препроцессора для отчётов об ошибках\n";
                out_file << "    ERROR::PREPROCESSOR_OUTPUT = preprocessor_output;\n";
                out_file << "    \n";
                out_file << "    // Создаём и запускаем лексер для токенизации обработанного препроцессором ввода\n";
                out_file << "    static Lexer parser = Lexer(file_path, preprocessor_output);\n";
                out_file << "    parser.run();\n";
                out_file << "    \n";
                out_file << "    // Создаём обходчик токенов для навигации по сгенерированным токенам\n";
                out_file << "    TokenWalker walker = TokenWalker(&parser.tokens);\n";
                out_file << "    \n";
                out_file << "    // Создаём и запускаем генератор AST (парсер) на потоке токенов\n";
                out_file << "    ASTGenerator generator = ASTGenerator(walker, file_path);\n";
                out_file << "    generator.parse();\n";
                out_file << "    \n";
                out_file << "    // Передаём владение сгенерированными узлами AST\n";
                out_file << "    auto nodes = std::move(generator.nodes);\n";
                out_file << "    \n";
                out_file << "    // Создаём экземпляр памяти для выполнения программы\n";
                out_file << "    auto g_memory = Memory();\n";
                out_file << "    \n";
                out_file << "    // Заполняем память определениями стандартных типов\n";
                out_file << "    GenerateStandartTypes(&g_memory, \"" << args_parser.file_path << "\");\n";
                out_file << "    \n";
                out_file << "    // Выполняем программу, используя узлы AST и память\n";
                out_file << "    run_with(&nodes, g_memory);\n";
                out_file << "    \n";
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
    }
    return 0;
}
