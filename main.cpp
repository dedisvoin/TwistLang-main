#include <iostream>
#include <string>
#include <io.h>
#include <fcntl.h>

int main() {
    // Устанавливаем режим UTF-16 для вывода
    _setmode(_fileno(stdout), _O_U16TEXT);

    std::wstring text = L"ПривDedет";
    std::wcout << text.size() << std::endl;

    for (wchar_t ch : text) {
        std::wcout << ch;  // Выводим символ
        if (ch == L'е') {
            std::wcout << L"YES";  // Используем wcout вместо cout
        }
        std::wcout << L" ";  // Разделитель
    }
    std::wcout << std::endl;
    
    return 0;
}