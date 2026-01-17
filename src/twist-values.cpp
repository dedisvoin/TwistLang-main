#include "string"
#include <memory>
#include <ostream>
#include <vcruntime_typeinfo.h>
#include "any"
#include <vector>
#pragma once


using namespace std;

struct Type {
    string name;
    vector<Type> union_types; // Для union типов (A | B | C)
    bool is_pointer = false;
    shared_ptr<Type> pointer_type; // Тип, на который указывает указатель (используем shared_ptr)

    Type(const string& name) {
        this->name = name;
        this->is_pointer = false;
    }

    Type(const vector<Type>& types) {
        this->name = "";
        this->is_pointer = false;
        this->union_types = types;
        for (const auto& t : types) {
            if (!this->name.empty()) {
                this->name += " | ";
            }
            this->name += t.name;
        }
    }

    // Конструктор для указателей
    Type(shared_ptr<Type> pointee_type) {
        this->pointer_type = pointee_type;
        this->is_pointer = true;
        this->name = "*" + this->pointer_type->name;
    }

    // Копирующий конструктор
    Type(const Type& other) {
        name = other.name;
        union_types = other.union_types;
        is_pointer = other.is_pointer;
        if (other.pointer_type) {
            pointer_type = make_shared<Type>(*other.pointer_type);
        }
    }

    // Оператор присваивания
    Type& operator=(const Type& other) {
        if (this != &other) {
            name = other.name;
            union_types = other.union_types;
            is_pointer = other.is_pointer;
            if (other.pointer_type) {
                pointer_type = make_shared<Type>(*other.pointer_type);
            } else {
                pointer_type.reset();
            }
        }
        return *this;
    }

    // Проверка, является ли этот тип union типом
    bool is_union_type() const {
        return !union_types.empty();
    }

    // Получить все базовые типы из union
    vector<Type> get_base_types() const {
        if (is_union_type()) {
            return union_types;
        }
        return {Type(*this)}; // Создаем копию
    }

    // Оператор для создания union типов
    Type operator|(const Type& other) const {
        vector<Type> new_types;
        
        // Добавляем все типы из левого операнда
        if (this->is_union_type()) {
            new_types.insert(new_types.end(), this->union_types.begin(), this->union_types.end());
        } else {
            new_types.push_back(Type(*this));
        }
        
        // Добавляем все типы из правого операнда
        if (other.is_union_type()) {
            new_types.insert(new_types.end(), other.union_types.begin(), other.union_types.end());
        } else {
            new_types.push_back(Type(other));
        }
        
        // Удаляем дубликаты
        vector<Type> unique_types;
        for (const auto& type : new_types) {
            bool exists = false;
            for (const auto& unique_type : unique_types) {
                if (type == unique_type) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                unique_types.push_back(type);
            }
        }
        
        return Type(unique_types);
    }

    bool operator==(const Type& other) const {
        if (this->is_pointer != other.is_pointer) {
            return false;
        }
        
        if (this->is_pointer) {
            // Для указателей сравниваем типы, на которые они указывают
            if (!this->pointer_type || !other.pointer_type) {
                return false;
            }
            
            // Специальная обработка для void*
            if ((*this->pointer_type).name == "void" || (*other.pointer_type).name == "void") {
                return (*this->pointer_type).name == (*other.pointer_type).name;
            }
            
            return *this->pointer_type == *other.pointer_type;
        }
        
        if (this->is_union_type() != other.is_union_type()) {
            return false;
        }
        
        if (this->is_union_type()) {
            // Для union типов сравниваем наборы типов
            auto this_types = this->get_base_types();
            auto other_types = other.get_base_types();
            
            if (this_types.size() != other_types.size()) {
                return false;
            }
            
            for (const auto& type : this_types) {
                bool found = false;
                for (const auto& other_type : other_types) {
                    if (type == other_type) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        }
        
        // Для обычных типов сравниваем имена
        return this->name == other.name;
    }

    bool operator!=(const Type& other) const {
        return !(*this == other);
    }
};

// Функция проверки, является ли один тип подтипом другого
bool IsSubType(const Type& type, const Type& possible_super_type) {
    // Если типы равны, то это подтип
    if (type == possible_super_type) {
        return true;
    }
    
    // Обработка union типов
    if (possible_super_type.is_union_type()) {
        // Для union типов проверяем, что type совместим с одним из типов в union
        auto super_types = possible_super_type.get_base_types();
        for (const auto& super_type : super_types) {
            if (IsSubType(type, super_type)) {
                return true;
            }
        }
        return false;
    }
    
    // Если type - union тип
    if (type.is_union_type()) {
        // Все базовые типы type должны быть подтипами possible_super_type
        auto type_bases = type.get_base_types();
        for (const auto& base : type_bases) {
            if (!IsSubType(base, possible_super_type)) {
                return false;
            }
        }
        return true;
    }
    
    // Обработка указателей
    if (possible_super_type.is_pointer) {
        // Тип тоже должен быть указателем
        if (!type.is_pointer) {
            return false;
        }
        
        // Для указателей проверяем инвариантность (в C/C++ указатели инвариантны)
        // *T1 является подтипом *T2 только если T1 == T2
        if (!type.pointer_type || !possible_super_type.pointer_type) {
            return false;
        }
        
        // В C/C++ указатели имеют строгую типизацию
        // *Int не является подтипом *String и наоборот
        // *Int не является подтипом *void, но *void является супертипом для всех указателей
        return *type.pointer_type == *possible_super_type.pointer_type;
    }
    
    // Если type - указатель, а possible_super_type - не указатель
    // (указатель не является подтипом не-указателя, кроме случая с void*)
    if (type.is_pointer) {
        return false;
    }
    
    // Для обычных типов: базовые типы
    // Здесь можно добавить иерархию типов, если она будет в языке
    // Пока что считаем, что разные обычные типы не являются подтипами друг друга
    return false;
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
    const Type VOID = Type("Void");
    const Type LAMBDA = Type("Lambda");
    
    // Указатель на void (универсальный указатель)
    static Type get_void_ptr() {
        static shared_ptr<Type> void_type = make_shared<Type>(VOID);
        static Type void_ptr(void_type);
        return void_ptr;
    }
    
    const Type& VOID_PTR = get_void_ptr();
}

// Функция проверки совместимости типов для присваивания
bool IsTypeCompatible(const Type& target_type, const Type& source_type) {
    // Сначала проверяем обычные подтипы
    if (IsSubType(source_type, target_type)) {
        return true;
    }
    
    // Специальная обработка для void* - супертип всех указателей
    if (source_type == STANDART_TYPE::VOID_PTR) {
        return target_type.is_pointer;
    }
    if (target_type == STANDART_TYPE::VOID_PTR) {
        // void* совместим с любым указателем
        return source_type.is_pointer;
    }
    
    // Специальная обработка для nullptr
    if (source_type == STANDART_TYPE::NULL_T) {
        // nullptr можно присвоить любому указателю
        return target_type.is_pointer;
    }
    
    return false;
}

struct Value {
    std::unique_ptr<Type> type;
    std::any data;

    Value(std::unique_ptr<Type> type, std::any data) 
        : type(std::move(type)), data(std::move(data)) {}
    
    Value(const Value& other) 
        : type(other.type ? std::make_unique<Type>(*other.type) : nullptr), 
          data(other.data) {}
  
    Value(Value&& other) = default;

    Value() = default;
    
    // Добавить operator= для правильного копирования
    Value& operator=(const Value& other) {
        if (this != &other) {
            type = other.type ? std::make_unique<Type>(*other.type) : nullptr;
            data = other.data;
        }
        return *this;
    }
    
    Value& operator=(Value&& other) = default;

    Value copy() {
        return Value(*this);
    }

    friend ostream& operator<<(ostream& os, const Value& value) {
        if (*value.type == STANDART_TYPE::STRING){
            os << any_cast<string>(value.data) << endl;
        }
        if (*value.type == STANDART_TYPE::NAMESPACE){
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
    return Value(std::make_unique<Type>(STANDART_TYPE::INT), value);
}

Value NewDouble(long double value) {
    return Value(std::make_unique<Type>(STANDART_TYPE::DOUBLE), value);
}

Value NewBool(bool value) {
    return Value(std::make_unique<Type>(STANDART_TYPE::BOOL), value);
}

Value NewType(string name) {
    return Value(std::make_unique<Type>(STANDART_TYPE::TYPE), Type(name));
}

Value NewType(Type type) {
    return Value(std::make_unique<Type>(STANDART_TYPE::TYPE), type);
}


Value NewNull() {
    return Value(std::make_unique<Type>(STANDART_TYPE::NULL_T), Null());
}

Value NewString(string value) {
    return Value(std::make_unique<Type>(STANDART_TYPE::STRING), value);
} 

Value NewChar(char value) {
    return Value(std::make_unique<Type>(STANDART_TYPE::CHAR), value);
}

Value NewPointer(int value, const Type& pointer_type) {
    auto type = std::make_unique<Type>(std::make_shared<Type>(pointer_type));
    return Value(std::move(type), value);
}

// Создать тип указателя из другого типа
Type PointerType(const Type& pointee_type) {
    return Type(std::make_shared<Type>(pointee_type));
}

// Создать указатель из значения
Value NewPointerValue(int address, const Type& pointee_type) {
    return NewPointer(address, pointee_type);
}

// Получить тип, на который указывает указатель
Type GetPointeeType(const Type& pointer_type) {
    if (!pointer_type.is_pointer || !pointer_type.pointer_type) {
        return STANDART_TYPE::VOID;
    }
    return Type(*pointer_type.pointer_type);
}

// Проверка, является ли тип указателем
bool IsPointerType(const Type& type) {
    return type.is_pointer;
}