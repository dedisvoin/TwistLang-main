// memory.cpp
#include "twist-values.cpp"
#include <string>
#include <iostream>
#include <unordered_map>

#pragma once

typedef int Address;

class AddressManager {
private:
    static int next_address;

public:
    static int get_next_address() {
        return ++next_address;
    }

    static int get_current_address() {
        return next_address;
    }

    static void reset() {
        next_address = 0;
    }
};
int AddressManager::next_address = 0;

struct Modifiers {
    bool is_const = false;
    bool is_static = false;
    bool is_final = false;
    bool is_global = false;
    bool is_private = false;
};

// Forward declaration
struct Memory;

// memory object
struct MemoryObject {
    Value value;
    Type wait_type;
    Modifiers modifiers;
    void* memory_pointer;
    Address address;
    std::string var_name;      // имя переменной (пустое для безымянных)
    Memory* owner;              // память-владелец (nullptr для безымянных)

    MemoryObject(Value value, Type wait_type, void* memory, Address address,
                 bool is_const, bool is_static, bool is_final, bool is_global, bool is_private,
                 const std::string& name = "", Memory* owner = nullptr)
        : value(value), wait_type(wait_type),
          modifiers({is_const, is_static, is_final, is_global, is_private}),
          memory_pointer(memory), address(address),
          var_name(name), owner(owner) {}
};

inline MemoryObject* CreateMemoryObject(Value value, Type wait_type, void* memory,
                                        bool is_const, bool is_static, bool is_final, bool is_global, bool is_private,
                                        const std::string& name = "", Memory* owner = nullptr) {
    int address = AddressManager::get_next_address();
    return new MemoryObject(value, wait_type, memory, address,
                            is_const, is_static, is_final, is_global, is_private,
                            name, owner);
}

inline MemoryObject* CreateMemoryObjectWithAddress(Value value, Type wait_type, void* memory, Address address,
                                                   bool is_const, bool is_static, bool is_final, bool is_global, bool is_private,
                                                   const std::string& name = "", Memory* owner = nullptr) {
    return new MemoryObject(value, wait_type, memory, address,
                            is_const, is_static, is_final, is_global, is_private,
                            name, owner);
}

struct Memory {
    std::unordered_map<std::string, MemoryObject*> string_pool;

    void clear();
    void clear_unglobals();
    bool add_object(const std::string& literal, Value& value, Type wait_type,
                    bool is_const = false, bool is_static = false, bool is_final = false,
                    bool is_global = false, bool is_private = false);
    bool add_object(const std::string& literal, MemoryObject* object);
    bool copy_object(const std::string& literal, Value value, Type wait_type, Address address = 0,
                     bool is_const = false, bool is_static = false, bool is_final = false,
                     bool is_global = false, bool is_private = false);
    bool add_object_in_lambda(const std::string& literal, Value value, bool is_global = false);
    bool add_object_in_func(const std::string& literal, Value value, Type type,
                            bool is_const = false, bool is_static = false, bool is_final = false,
                            bool is_global = false, bool is_private = false);
    bool add_object_in_struct(const std::string& literal, Value& value,
                              bool is_const = false, bool is_static = false, bool is_final = false,
                              bool is_global = false, bool is_private = false);
    
    

    inline MemoryObject* get_variable(const std::string& literal) {
        auto it = string_pool.find(literal);
        return it != string_pool.end() ? it->second : nullptr;
    }

    inline void set_object_value(const std::string& literal, Value new_value) {
        get_variable(literal)->value = new_value;
    }

    inline void link_objects(Memory* target_memory) {
        for (auto& pair : string_pool) {
            if (!pair.second->modifiers.is_global)
                continue;
            target_memory->string_pool[pair.first] = pair.second;
        }
    }

    inline void copy_objects(Memory& target_memory) {
        for (auto& pair : string_pool) {
            if (!pair.second->modifiers.is_global)
                continue;
            target_memory.string_pool[pair.first] = new MemoryObject(*pair.second);
        }
    }

    inline Type get_wait_type(const std::string& literal) {
        return string_pool.find(literal)->second->wait_type;
    }

    inline bool check_literal(const std::string& literal) {
        return string_pool.find(literal) != string_pool.end();
    }

    inline bool is_final(const std::string& literal) {
        return string_pool.find(literal)->second->modifiers.is_final;
    }

    inline bool is_private(const std::string& literal) {
        return string_pool.find(literal)->second->modifiers.is_private;
    }

    inline bool is_const(const std::string& literal) {
        return string_pool.find(literal)->second->modifiers.is_const;
    }

    inline bool is_static(const std::string& literal) {
        return string_pool.find(literal)->second->modifiers.is_static;
    }

    inline bool is_global(const std::string& name) {
        return string_pool.find(name)->second->modifiers.is_global;
    }

    void delete_variable(const std::string& name);
    void debug_print();
};

struct GlobalMemory {
    static std::unordered_map<int, MemoryObject*> all_objects;

    static void register_object(MemoryObject* obj) {
        all_objects[obj->address] = obj;
    }

    static bool is_registered(int address) {
        return all_objects.find(address) != all_objects.end();
    }

    static void unregister_object(int address) {
        all_objects.erase(address);
    }

    static MemoryObject* get_by_address(int address) {
        auto it = all_objects.find(address);
        return it != all_objects.end() ? it->second : nullptr;
    }

    void set_object_value(int address, Value new_value) {
        get_by_address(address)->value = new_value;
    }

    Modifiers get_modifiers(int address) {
        return get_by_address(address)->modifiers;
    }

    static void clear() {
        // НЕ удаляем объекты здесь, только очищаем карту
        // Объекты уже удалены деструктором Memory
        all_objects.clear();
    }
};

// Определение статической переменной
std::unordered_map<int, MemoryObject*> GlobalMemory::all_objects;

// Глобальный объект (определён после объявления GlobalMemory)
static GlobalMemory STATIC_MEMORY;


void Memory::clear() {
    string_pool.clear();
}

void Memory::clear_unglobals() {
    for (auto it = string_pool.begin(); it != string_pool.end();) {
        if (!it->second->modifiers.is_global) {
            it = string_pool.erase(it);
        } else {
            ++it;
        }
    }
}

bool Memory::add_object(const std::string& literal, Value& value, Type wait_type,
                        bool is_const, bool is_static, bool is_final,
                        bool is_global, bool is_private) {
    try {
        auto object = CreateMemoryObject(value, wait_type, this,
                                         is_const, is_static, is_final, is_global, is_private,
                                         literal, this);
        string_pool.emplace(literal, object);
        STATIC_MEMORY.register_object(object);
        return true;
    } catch (...) {
        return false;
    }
}

bool Memory::add_object(const std::string& literal, MemoryObject* object) {
    try {
        string_pool.emplace(literal, object);
        // Предполагаем, что object уже зарегистрирован в STATIC_MEMORY
        return true;
    } catch (...) {
        return false;
    }
}

bool Memory::copy_object(const std::string& literal, Value value, Type wait_type, Address address,
                         bool is_const, bool is_static, bool is_final,
                         bool is_global, bool is_private) {
    try {
        auto object = CreateMemoryObjectWithAddress(value, wait_type, this, address,
                                                    is_const, is_static, is_final, is_global, is_private,
                                                    literal, this);
        string_pool.emplace(literal, object);
        STATIC_MEMORY.register_object(object);
        return true;
    } catch (...) {
        return false;
    }
}

bool Memory::add_object_in_lambda(const std::string& literal, Value value, bool is_global) {
    auto object = new MemoryObject(value, value.type, this, 0,
                                   false, true, false, is_global, false,
                                   literal, this);
    if (check_literal(literal)) delete_variable(literal);
    try {
        string_pool.emplace(literal, object);
        STATIC_MEMORY.register_object(object);
        return true;
    } catch (...) {
        return false;
    }
}

bool Memory::add_object_in_func(const std::string& literal, Value value, Type type,
                                bool is_const, bool is_static, bool is_final,
                                bool is_global, bool is_private) {
    auto object = new MemoryObject(value, type, this, 0,
                                   is_const, is_static, is_final, is_global, is_private,
                                   literal, this);
    if (check_literal(literal)) delete_variable(literal);
    try {
        string_pool.emplace(literal, object);
        STATIC_MEMORY.register_object(object);
        return true;
    } catch (...) {
        return false;
    }
}

bool Memory::add_object_in_struct(const std::string& literal, Value& value,
                                  bool is_const, bool is_static, bool is_final,
                                  bool is_global, bool is_private) {
    auto object = new MemoryObject(value, value.type, this, 0,
                                   is_const, is_static, is_final, is_global, is_private,
                                   literal, this);
    if (check_literal(literal)) delete_variable(literal);
    try {
        string_pool.emplace(literal, object);
        STATIC_MEMORY.register_object(object);
        return true;
    } catch (...) {
        return false;
    }
}

void Memory::delete_variable(const std::string& name) {
    auto it = string_pool.find(name);
    if (it != string_pool.end()) {
        MemoryObject* obj = it->second;
        STATIC_MEMORY.unregister_object(obj->address);
        string_pool.erase(it);
        delete obj;
    }
}

void Memory::debug_print() {
    std::cout << "Memory Dump:" << std::endl;
    for (const auto& [name, obj] : string_pool) {
        std::cout << "\tVariable Name: " << name << ", Type: " << obj->value.type.pool;
        try {
            if (obj->value.type.pool == STANDART_TYPE::INT.pool) {
                std::cout << ", Value: " << std::any_cast<int64_t>(obj->value.data);
            } else if (obj->value.type.pool == STANDART_TYPE::DOUBLE.pool) {
                std::cout << ", Value: " << std::any_cast<long double>(obj->value.data);
            } else if (obj->value.type.pool == STANDART_TYPE::BOOL.pool) {
                std::cout << ", Value: " << (std::any_cast<bool>(obj->value.data) ? "true" : "false");
            } else if (obj->value.type.pool == STANDART_TYPE::STRING.pool) {
                std::cout << ", Value: " << std::any_cast<std::string>(obj->value.data);
            }
        } catch (...) {
            std::cout << ", Value: <bad cast>";
        }
        std::cout << " static:" << obj->modifiers.is_static;
        std::cout << std::endl;
    }
}
