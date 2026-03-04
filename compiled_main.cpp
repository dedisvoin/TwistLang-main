// Подключаем реализацию лексера, обходчика токенов и парсера
#include "src/twist-lexer.cpp"
#include "src/twist-tokenwalker.cpp"
#include "src/twist-parser.cpp"

// Функция для выполнения списка узлов AST в заданном контексте памяти
void run_with(vector<unique_ptr<Node>>* nodes, Memory& g_memory) {
    for (size_t i = 0; i < nodes->size(); i++) {
        // Выполняем код каждого узла в глобальной памяти
        (*nodes)[i]->exec_from(g_memory);
    }
}

// Точка входа в сгенерированный исполняемый файл
int main() {
    // Сохраняем путь к исходному файлу
    std::string file_path = "main.twist";
    
    // Сохраняем вывод препроцессора как строковый литерал (raw string)
    std::string preprocessor_output = R"(let a = new 10;

outln *a;
)";
    
    // Устанавливаем глобальный вывод препроцессора для отчётов об ошибках
    ERROR::PREPROCESSOR_OUTPUT = preprocessor_output;
    
    // Создаём и запускаем лексер для токенизации обработанного препроцессором ввода
    static Lexer parser = Lexer(file_path, preprocessor_output);
    parser.run();
    
    // Создаём обходчик токенов для навигации по сгенерированным токенам
    TokenWalker walker = TokenWalker(&parser.tokens);
    
    // Создаём и запускаем генератор AST (парсер) на потоке токенов
    ASTGenerator generator = ASTGenerator(walker, file_path);
    generator.parse();
    
    // Передаём владение сгенерированными узлами AST
    auto nodes = std::move(generator.nodes);
    
    // Создаём экземпляр памяти для выполнения программы
    auto g_memory = Memory();
    
    // Заполняем память определениями стандартных типов
    GenerateStandartTypes(&g_memory, "main.twist");
    
    // Выполняем программу, используя узлы AST и память
    run_with(&nodes, g_memory);
    
    return 0;
}
