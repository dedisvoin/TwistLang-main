#include "src/twist-preproc.cpp"
#include "src/twist-lexer.cpp"
#include "src/twist-utils.cpp"
#include "src/twist-tokenwalker.cpp"
#include "src/twist-parser.cpp"

#include "fstream"
#include <filesystem>
#include <fstream>

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
    out << "pif: " << err.pif << ":" << err.pif.lenght << ":" << err.message_type
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
            if (err.message_type == 1 || err.message_type == 2) {
                collected.push_back(err);
            } else {
                throw;
            }
        }
    }
    for (auto node : nodes) {
        delete node;
    }
    return collected;
}

void language_server(const std::string& file_path, std::string file_name) {
    Error::ClearBuffer();
    std::unique_ptr<Memory> g_memory = std::make_unique<Memory>();

    try {
        std::string source = OpenFile(file_path);
        ERROR::PREPROCESSOR_OUTPUT = source;
        ERROR_THROW::PREPROCESSOR_OUTPUT = source;

        // 1. Лексирование (только основного файла)
        Lexer lexer(file_path, source);
        lexer.run();

        // 2. Препроцессор — ВСТАВЛЯЕТ ВСЕ INCLUDE, раскрывает define/macro
        // После этого шага preprocessed_tokens содержит ВЕСЬ код: основной + все библиотеки
        Preprocessor preprocessor;
        vector<Token> preprocessed_tokens;
        preprocessed_tokens = preprocessor.process(lexer.tokens, file_path);

        // 3. Парсинг ЕДИНОГО потока токенов (основной файл + все include)
        TokenWalker walker(&preprocessed_tokens);
        ASTGenerator parser(walker, file_path);
        parser.parse();

        // 4. Генерация стандартных типов (один раз для всей программы)
        auto nodes = std::move(parser.nodes);
        GenerateStandartTypes(g_memory.get(), file_path);

        // 5. Выполнение ВСЕГО кода (основной + библиотеки) как единой программы
        auto assert_errors = run_with_collect(nodes, g_memory.get());

        // 6. Сбор всех ошибок (включая assert/warning из библиотек)
        if (!assert_errors.empty()) {
            for (const auto& err : assert_errors) {
                err.Write();
            }
        }

    } catch (const Error& err) {
        err.Write();
    } catch (const std::exception& e) {
        // Игнорируем
    }

    // Запись лога
    std::ofstream log(string("dbg/") + file_name + "_ls.dbg", std::ios::trunc);
    log << Error::GetBuffer();
    log.close();

    GlobalMemory::all_objects.clear();
    AddressManager::reset();
}

int main(int argc, char** argv) {
    std::locale::global(std::locale("ru_RU.UTF-8"));

    static ArgsParser args_parser = GenerateArgsParser(argc, argv);

    if (args_parser.as_debuger) {
        // Режим однократной проверки (языковой сервер)
        language_server(args_parser.file_path, args_parser.file_name);
        return 0;
    } else {
        // Обычный запуск или компиляция
        static string file_content = OpenFile(args_parser.file_path);

        ERROR::PREPROCESSOR_OUTPUT = file_content;
        ERROR_THROW::PREPROCESSOR_OUTPUT = file_content;

        if (!args_parser.compile_mod) {
            if (args_parser.save_preprocessed)
                SavePreprocessedFile("output.twist", file_content);

            static Lexer parser = Lexer(args_parser.file_path, file_content);

            TimeIt("Parse finished in ", [](){
                parser.run();
            });

            Preprocessor preprocessor = Preprocessor();
            vector<Token> preprocessed_tokens;
            try {
                preprocessed_tokens = preprocessor.process(parser.tokens, args_parser.file_path);
            } catch (Error& err) {
                err.print();
            }
            if (args_parser.save_token) {
                SaveTokensFile("tokens.txt", parser.tokens);
                SaveTokensFile("ptokens.txt", preprocessed_tokens);
            }

            parser.tokens = preprocessed_tokens;

            TokenWalker walker = TokenWalker(&parser.tokens);
            ASTGenerator generator = ASTGenerator(walker, args_parser.file_path);

            try {
                generator.parse();
            } catch (Error& err) {
                err.print();
            }

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
            // Компиляторный режим (без изменений)
            std::filesystem::path path_obj(args_parser.file_path);
            static std::string stem = "compiled_" + path_obj.stem().string() + ".cpp";
            std::ofstream out_file(stem);

            if (out_file.is_open()) {
                out_file << "// Подключаем реализацию лексера, обходчика токенов и парсера\n";
                out_file << "#include \"src/twist-lexer.cpp\"\n";
                out_file << "#include \"src/twist-tokenwalker.cpp\"\n";
                out_file << "#include \"src/twist-parser.cpp\"\n";
                out_file << "\n";
                out_file << "// Функция для выполнения списка узлов AST в заданном контексте памяти\n";
                out_file << "void run_with(vector<unique_ptr<Node>>* nodes, Memory& g_memory) {\n";
                out_file << "    for (size_t i = 0; i < nodes->size(); i++) {\n";
                out_file << "        (*nodes)[i]->exec_from(g_memory);\n";
                out_file << "    }\n";
                out_file << "}\n";
                out_file << "\n";
                out_file << "// Точка входа в сгенерированный исполняемый файл\n";
                out_file << "int main() {\n";
                out_file << "    std::string file_path = \"" << args_parser.file_path << "\";\n";
                out_file << "    \n";
                out_file << "    std::string preprocessor_output = R\"twist(" << file_content << ")twist\";\n";
                out_file << "    \n";
                out_file << "    ERROR::PREPROCESSOR_OUTPUT = preprocessor_output;\n";
                out_file << "    \n";
                out_file << "    static Lexer parser = Lexer(file_path, preprocessor_output);\n";
                out_file << "    parser.run();\n";
                out_file << "    \n";
                out_file << "    TokenWalker walker = TokenWalker(&parser.tokens);\n";
                out_file << "    \n";
                out_file << "    ASTGenerator generator = ASTGenerator(walker, file_path);\n";
                out_file << "    generator.parse();\n";
                out_file << "    \n";
                out_file << "    auto nodes = std::move(generator.nodes);\n";
                out_file << "    \n";
                out_file << "    auto g_memory = Memory();\n";
                out_file << "    \n";
                out_file << "    GenerateStandartTypes(&g_memory, \"" << args_parser.file_path << "\");\n";
                out_file << "    \n";
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