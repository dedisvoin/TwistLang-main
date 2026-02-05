#include "string"
#include <ostream>
#include <vcruntime_typeinfo.h>
#include "any"
#include "iostream"
#include <vector>
#pragma once


using namespace std;

struct Type {
    string pool;
    
    Type(string pool) : pool(pool) {}

    bool operator==(const Type& other) const {
        return pool == other.pool;
    }

    bool operator!=(const Type& other) const {
        return !(*this == other);
    }

    bool is_sub_type(const Type& other) const {
        // Если типы равны
        if (pool == other.pool) return true;
        
        // Специальные случаи
        if (other.pool == "auto") return true;
        if (other.pool == "*auto") return true;
        
        // Разбиваем на компоненты union
        vector<string> current_components = split_union_components();
        vector<string> other_components = other.split_union_components();
        
        // Если текущий тип - union (T1 | T2 | ...)
        if (current_components.size() > 1) {
            // Union тип является подтипом другого типа, если
            // КАЖДЫЙ его компонент является подтипом другого типа
            for (const string& comp : current_components) {
                Type comp_type(comp);
                if (!comp_type.is_sub_type(other)) {
                    return false;
                }
            }
            return true;
        }
        
        // Если other - union (O1 | O2 | ...)
        if (other_components.size() > 1) {
            // Тип является подтипом union типа, если
            // он является подтипом ХОТЯ БЫ ОДНОГО компонента union
            for (const string& other_comp : other_components) {
                Type other_comp_type(other_comp);
                if (this->is_sub_type(other_comp_type)) {
                    return true;
                }
            }
            return false;
        }
        
        // Оба типа - базовые (не union)
        
        // Проверяем функциональные типы
        if (is_func() && other.is_func()) {
            return is_func_sub_type(other);
        }
        
        // Проверяем указатели
        if (is_pointer() && other.is_pointer()) {
            // *T1 ⊆ *T2 если T1 ⊆ T2
            string t1 = pool.substr(1);
            string t2 = other.pool.substr(1);
            Type base1(t1);
            Type base2(t2);
            return base1.is_sub_type(base2);
        }
        
        // Базовые типы должны совпадать
        return pool == other.pool;
    }

    bool is_func_sub_type(const Type& other) const {
        // Парсим текущий функциональный тип
        pair<vector<Type>, Type> current = parse_func_type();
        pair<vector<Type>, Type> other_pair = other.parse_func_type();
        
        // Проверяем, что оба успешно распарсились как функции
        if (current.first.empty() && !is_func()) return false;
        if (other_pair.first.empty() && !other.is_func()) return false;
        
        // Количество аргументов должно совпадать
        if (current.first.size() != other_pair.first.size()) {
            return false;
        }
        
        // Проверяем КОНТРАвариантность аргументов:
        // Для всех i, other_args[i] ⊆ current_args[i]
        for (size_t i = 0; i < current.first.size(); i++) {
            if (!current.first[i].is_sub_type(other_pair.first[i])) {
                return false;
            }
        }
        
        // Проверяем КОвариантность возвращаемого типа:
        // current_ret ⊆ other_ret
        return current.second.is_sub_type(other_pair.second);
    }

    // Парсинг функционального типа: возвращает (аргументы, возвращаемый_тип)
    pair<vector<Type>, Type> parse_func_type() const {
        vector<Type> args;
        Type ret_type("Void");
        
        if (!is_func()) {
            return make_pair(args, ret_type);
        }
        
        string str = pool;
        
        // Убираем внешние скобки, если есть
        if (str[0] == '(' && str[str.length()-1] == ')') {
            // Проверяем, что это действительно функция в скобках
            if (str.substr(1, 4) == "Func") {
                str = str.substr(1, str.length() - 2);
            }
        }
        
        // Убираем "Func"
        size_t func_pos = str.find("Func");
        if (func_pos != string::npos) {
            str = str.substr(func_pos + 4);
        }
        
        // Находим начало списка аргументов
        if (str[0] == '(') {
            // Ищем закрывающую скобку для списка аргументов
            int balance = 1;
            size_t i = 1;
            for (; i < str.length(); i++) {
                if (str[i] == '(') balance++;
                else if (str[i] == ')') {
                    balance--;
                    if (balance == 0) break;
                }
            }
            
            string args_str = str.substr(1, i - 1);
            string rest = (i + 1 < str.length()) ? str.substr(i + 1) : "";
            
            // Парсим аргументы
            args = parse_type_list(args_str);
            
            // Парсим возвращаемый тип, если есть
            size_t arrow_pos = rest.find("->");
            if (arrow_pos != string::npos) {
                string ret_str = rest.substr(arrow_pos + 2);
                // Убираем пробелы
                size_t start = ret_str.find_first_not_of(" \t");
                size_t end = ret_str.find_last_not_of(" \t");
                if (start != string::npos) {
                    ret_str = ret_str.substr(start, end - start + 1);
                }
                ret_type = Type(ret_str);
            }
        } else {
            // Нет скобок вокруг аргументов
            size_t arrow_pos = str.find("->");
            if (arrow_pos != string::npos) {
                string args_str = str.substr(0, arrow_pos);
                string ret_str = str.substr(arrow_pos + 2);
                
                // Убираем пробелы
                args_str.erase(args_str.find_last_not_of(" \t") + 1);
                ret_str.erase(0, ret_str.find_first_not_of(" \t"));
                ret_str.erase(ret_str.find_last_not_of(" \t") + 1);
                
                args = parse_type_list(args_str);
                ret_type = Type(ret_str);
            } else {
                // Нет стрелки -> все это аргументы
                args = parse_type_list(str);
            }
        }
        
        return make_pair(args, ret_type);
    }

    vector<Type> parse_type_list(const string& str) const {
        vector<Type> types;
        if (str.empty()) return types;
        
        string current;
        int balance = 0;
        
        for (size_t i = 0; i < str.length(); i++) {
            char c = str[i];
            
            if (c == '(') {
                balance++;
                current += c;
            } else if (c == ')') {
                balance--;
                current += c;
            } else if (c == ',' && balance == 0) {
                // Нашли разделитель между типами
                if (!current.empty()) {
                    // Убираем пробелы
                    size_t start = current.find_first_not_of(" \t");
                    size_t end = current.find_last_not_of(" \t");
                    if (start != string::npos && end != string::npos) {
                        types.push_back(Type(current.substr(start, end - start + 1)));
                    }
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        
        // Добавляем последний тип
        if (!current.empty()) {
            size_t start = current.find_first_not_of(" \t");
            size_t end = current.find_last_not_of(" \t");
            if (start != string::npos && end != string::npos) {
                types.push_back(Type(current.substr(start, end - start + 1)));
            }
        }
        
        return types;
    }

    // Находит конец типа (с учетом вложенных скобок)
    size_t find_type_end(const string& str, size_t start) const {
        int balance = 0;
        bool in_func = false;
        int func_depth = 0;
        
        for (size_t i = start; i < str.length(); i++) {
            char c = str[i];
            
            if (c == '(') {
                balance++;
                // Проверяем, не начинается ли это с "Func"
                if (i >= 4 && str.substr(i-4, 4) == "Func") {
                    in_func = true;
                    func_depth = balance;
                }
            } else if (c == ')') {
                balance--;
                if (in_func && balance < func_depth) {
                    in_func = false;
                }
            } else if (c == ',' && balance == 0 && !in_func) {
                return i;
            }
        }
        
        return string::npos;
    }

    bool is_union_type() const {
        // Проверяем, является ли тип union type
        // Union type имеет " | " на верхнем уровне (не внутри скобок)
        int balance = 0;
        for (size_t i = 0; i < pool.length(); i++) {
            if (pool[i] == '(') {
                balance++;
            } else if (pool[i] == ')') {
                balance--;
            } else if (pool[i] == '|') {
                // Если нашли '|' и balance == 0, значит это разделитель union type
                // Проверяем, что перед и после '|' есть пробелы (формат " | ")
                if (balance == 0 && 
                    i > 0 && pool[i-1] == ' ' && 
                    i + 1 < pool.length() && pool[i+1] == ' ') {
                    return true;
                }
            }
        }
        return false;
    }

    bool is_array_type() const {
        if (!is_base_type()) return false;
        return pool.substr(0,1) == "[";
    }

    bool is_base_type() const {
        // Базовый тип - без '|' на верхнем уровне
        return !is_union_type();
    }

    bool is_func() const {
        // Проверяем, является ли тип функцией
        // Функция должна быть базовым типом и начинаться с '('
        if (pool.substr(0,5) == "(Func") return true;
        if (pool.substr(0,4) == "Func") return true;
        return false;
    }

    bool is_pointer() const {
        // Проверяем, является ли тип указателем
        // Указатель должен быть базовым типом и начинаться с '*'
        vector<string> components = split_union_components();
        int is_p = 0;
        for (const string& comp : components) {
            if (comp.substr(0,1) == "*") {
                is_p++;
            }
        }
        return is_p == components.size();
    }

    // Вспомогательная функция для разбора union типа на компоненты
    vector<string> split_union_components() const {
        vector<string> components;
        
        if (!is_union_type()) {
            components.push_back(pool);
            return components;
        }
        
        int balance = 0;
        string current;
        
        for (size_t i = 0; i < pool.length(); i++) {
            char c = pool[i];
            
            if (c == '(') {
                balance++;
                current += c;
            } else if (c == ')') {
                balance--;
                current += c;
            } else if (c == '|' && balance == 0 && 
                    i > 0 && pool[i-1] == ' ' && 
                    i + 1 < pool.length() && pool[i+1] == ' ') {
                // Нашли разделитель union
                if (!current.empty()) {
                    // Удаляем пробелы в начале и конце
                    size_t start = current.find_first_not_of(" \t");
                    size_t end = current.find_last_not_of(" \t");
                    if (start != string::npos && end != string::npos) {
                        components.push_back(current.substr(start, end - start + 1));
                    }
                    current.clear();
                }
                i++; // Пропускаем пробел после '|'
            } else {
                current += c;
            }
        }
        
        // Добавляем последний компонент
        if (!current.empty()) {
            size_t start = current.find_first_not_of(" \t");
            size_t end = current.find_last_not_of(" \t");
            if (start != string::npos && end != string::npos) {
                components.push_back(current.substr(start, end - start + 1));
            }
        }
        
        return components;
    }

    Type operator|(const Type& other) const {
        // Если текущий тип пустой, возвращаем other
        if (pool.empty()) {
            return other;
        }
        
        // Если other пустой, возвращаем текущий
        if (other.pool.empty()) {
            return *this;
        }
        
        // Если типы одинаковые, возвращаем тот же
        if (pool == other.pool) {
            return *this;
        }
        
        // Получаем компоненты текущего типа
        vector<string> current_components = split_union_components();
        
        // Получаем компоненты другого типа
        vector<string> other_components = other.split_union_components();
        
        // Объединяем компоненты без дублирования
        vector<string> result_components = current_components;
        
        for (const string& other_comp : other_components) {
            bool found = false;
            
            // Проверяем, есть ли уже такой компонент
            for (const string& current_comp : current_components) {
                if (current_comp == other_comp) {
                    found = true;
                    break;
                }
            }
            
            // Если не нашли, добавляем
            if (!found) {
                result_components.push_back(other_comp);
            }
        }
        
        // Если остался только один компонент, возвращаем его
        if (result_components.size() == 1) {
            return Type(result_components[0]);
        }
        
        // Собираем union строку
        string result;
        for (size_t i = 0; i < result_components.size(); i++) {
            result += result_components[i];
            if (i != result_components.size() - 1) {
                result += " | ";
            }
        }
        
        return Type(result);
    }
};

// Создание типа указателя
Type create_pointer_type(const Type& type) {
    // Проверяем, является ли тип union
    if (type.is_union_type()) {
        // Для union типов применяем указатель к каждому компоненту
        vector<string> components = type.split_union_components();
        string result;
        
        for (size_t i = 0; i < components.size(); i++) {
            Type component_type(components[i]);
            result += "*" + component_type.pool;
            if (i != components.size() - 1) {
                result += " | ";
            }
        }
        
        return Type(result);
    } else {
        // Для базовых типов просто добавляем '*'
        return Type("*" + type.pool);
    }
}

// Создание функционального типа
Type create_function_type(Type return_type, vector<Type> argument_types) {
    string func_str = "(Func(";
    
    for (size_t i = 0; i < argument_types.size(); i++) {
        func_str += argument_types[i].pool;
        if (i != argument_types.size() - 1) {
            func_str += ", ";
        }
    }
    
    func_str += ") -> " + return_type.pool + ")";
    return Type(func_str);
}

Type create_function_type(vector<Type> argument_types) {
    string func_str = "Func(";
    
    for (size_t i = 0; i < argument_types.size(); i++) {
        func_str += argument_types[i].pool;
        if (i != argument_types.size() - 1) {
            func_str += ", ";
        }
    }
    
    func_str += ")";
    return Type(func_str);
}



namespace STANDART_TYPE {
    const Type INT = Type("Int");
    const Type BOOL = Type("Bool");
    const Type STRING = Type("String");
    const Type CHAR = Type("Char");
    const Type DOUBLE = Type("Double");
    const Type TYPE = Type("Type");
    const Type NAMESPACE = Type("Namespace");
    const Type NULL_T = Type("Null");
    const Type LAMBDA = Type("Lambda");
    const Type AUTO = Type("auto");
    
}

// Функция проверки совместимости типов для присваивания
bool IsTypeCompatible(const Type& target_type, const Type& source_type) {
    return source_type.is_sub_type(target_type);
}

struct Value {
    Type type;
    std::any data;

    Value(Type type, std::any data) 
        : type(type), data(std::move(data)) {}
    
    Value(const Value& other) 
        : type(other.type), 
          data(other.data) {}
  
    Value(Value&& other) = default;


    // Добавить operator= для правильного копирования
    Value& operator=(const Value& other) {
        if (this != &other) {
            type = other.type;
            data = other.data;
        }
        return *this;
    }
    
    Value& operator=(Value&& other) = default;

    Value copy() {
        return Value(*this);
    }

    friend ostream& operator<<(ostream& os, const Value& value) {
        if (value.type == STANDART_TYPE::STRING){
            os << any_cast<string>(value.data) << endl;
        }
        if (value.type == STANDART_TYPE::NAMESPACE){
            os << "NAMESPACE" << endl;
        }
        return os;
    }
};

struct Null {
    Null() = default;

    template<typename T>
    bool operator==(T) {
        return typeid(Null).name() == typeid(T).name();
    }

    template<typename T>
    bool operator!=(T) {
        return typeid(Null).name() != typeid(T).name();
    }
};

Value NewInt(int64_t value) {
    return Value(STANDART_TYPE::INT, value);
}

Value NewDouble(float value) {
    return Value(STANDART_TYPE::DOUBLE, value);
}

Value NewBool(bool value) {
    return Value(STANDART_TYPE::BOOL, value);
}

Value NewType(string name) {
    return Value(STANDART_TYPE::TYPE, Type(name));
}

Value NewType(Type type) {
    return Value(STANDART_TYPE::TYPE, type);
}


Value NewNull() {
    return Value(STANDART_TYPE::NULL_T, Null());
}

Value NewString(string value) {
    return Value(STANDART_TYPE::STRING, value);
} 

Value NewChar(char value) {
    return Value(STANDART_TYPE::CHAR, value);
}

Value NewPointer(int value, const Type& pointer_type, const bool create_pointer = true) {

    if (create_pointer)
        return Value(create_pointer_type(pointer_type), value);
    else
        return Value(pointer_type, value);
}

// Создать тип указателя из другого типа
Type PointerType(const Type& pointer_type) {
    return create_pointer_type(pointer_type);
}

// Создать указатель из значения
Value NewPointerValue(int address, const Type& pointee_type) {
    return NewPointer(address, pointee_type);
}
