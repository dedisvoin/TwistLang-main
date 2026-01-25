#include "string"
#include <ostream>
#include <vcruntime_typeinfo.h>
#include "any"
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
        // Разбиваем текущий тип на компоненты и проверяем, содержит ли он other
        vector<string> components = split_union_components();

        
        // Разбиваем other на компоненты (если это union, проверяем все компоненты)
        vector<string> other_components = other.split_union_components();
        int size = components.size();
        int counter = 0;
        
        // Проверяем, что каждый компонент other содержится в текущем типе
        for (const string& other_comp : other_components) {
            for (const string& comp : components) {
                if (comp == other_comp) {
                    counter++;
                }
            }
            
        }
        if (counter == size) {
            return true;
        }
        return false;
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

    bool is_base_type() const {
        // Базовый тип - без '|' на верхнем уровне
        return !is_union_type();
    }

    bool is_pointer() const {
        // Проверяем, является ли тип указателем
        // Указатель должен быть базовым типом и начинаться с '*'
        if (!is_base_type()) return false;
        return !pool.empty() && pool[0] == '*';
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

Value NewDouble(long double value) {
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

Value NewPointer(int value, const Type& pointer_type) {
    auto type = pointer_type;
    return Value(create_pointer_type(pointer_type), value);
}

// Создать тип указателя из другого типа
Type PointerType(const Type& pointer_type) {
    return create_pointer_type(pointer_type);
}

// Создать указатель из значения
Value NewPointerValue(int address, const Type& pointee_type) {
    return NewPointer(address, pointee_type);
}
