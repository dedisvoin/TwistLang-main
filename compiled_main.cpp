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
    std::string preprocessor_output = R"twist(global func _print(args[]: auto) -> Null {
    let size = sizeof(args);
    for (let i = 0; i < size; i = i + 1;) {
        out args[i];
    }
    out "\n";
}


struct Vec2 {
    static const let Number: Type = Int | Double;
    static let x: Number?;
    static let y: Number?;

    func __init__(_x: Number, _y: Number) -> Vec2 {
        let v = Vec2();
        v.x = _x;
        v.y = _y;
        ret v;
    }

    func print() {
        _print("Vec(", x, ", ", y, ")");
    }


}

let v1 = Vec2.__init__(10, 20);
let v2 = Vec2.__init__(3.14, 10);

v2.print();
v1.print();
)twist";
    
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
