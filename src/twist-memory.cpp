// memory.cpp
#include "twist-values.cpp"
#include "twist-utils.cpp"
#include <map>
#include <string>
#include <iostream>
#include <unordered_map>

#pragma once

typedef int Address;

// Создайте отдельный класс для управления адресами
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
    bool is_const;
    bool is_static;
    bool is_final;
    bool is_global;
    bool is_private;
};

// memory object
struct MemoryObject {
    Value value;
    Type wait_type;

    
    Modifiers modifiers;

    void* memory_pointer;
    Address address;

    MemoryObject(Value value, Type wait_type, void* memory, Address address,
                 bool is_const, bool is_static, bool is_final, bool is_global, bool is_private         
    ) : value(value), wait_type(wait_type), modifiers({is_const, is_static, is_final, is_global, is_private}), memory_pointer(memory), address(address) {
        
    }

};

inline MemoryObject* CreateMemoryObject(Value value, Type wait_type, void* memory, bool is_const, bool is_static, bool is_final, bool is_global, bool is_private) {
    int address = AddressManager::get_next_address();
    return new MemoryObject(value, wait_type,memory, address,  is_const, is_static, is_final, is_global, is_private);
}

inline MemoryObject* CreateMemoryObjectWithAddress(Value value, Type wait_type, void* memory, Address address,bool is_const, bool is_static, bool is_final, bool is_global, bool is_private) {
    return new MemoryObject(value, wait_type, memory, address, is_const, is_static, is_final, is_global, is_private);
}

struct Memory {
    unordered_map<string, MemoryObject*> string_pool;

    inline void clear() {
        string_pool.clear();
    }

    void clear_unglobals() {
        for (auto it = string_pool.begin(); it != string_pool.end();) {
            if (!it->second->modifiers.is_global) {
                it = string_pool.erase(it);
            } else {
                it++;
            }
        }
    }

    // add memory object in all pools
    bool add_object(const string& literal, Value value, Type wait_type, bool is_const = false, bool is_static = false, bool is_final = false, bool is_global = false, bool is_private = false) {
        try {
            auto object = CreateMemoryObject(value, wait_type, this, is_const, is_static, is_final, is_global, is_private);
            string_pool.emplace(literal, object);

        } catch (...) {
            return false;
        }
        return true;
    }

    bool add_object(const string& literal, MemoryObject* object) {
        try {
            string_pool.emplace(literal, object);
        } catch (...) {
            return false;
        }
        return true;
    }

    bool copy_object(const string& literal, Value value, Type wait_type, Address address = 0, bool is_const = false, bool is_static = false, bool is_final = false, bool is_global = false, bool is_private = false) {
        try {
            auto object = CreateMemoryObjectWithAddress(value, wait_type,this, address, is_const, is_static, is_final, is_global, is_private);
            string_pool.emplace(literal, object);

        } catch (...) {
            return false;
        }
        return true;
    }

    bool add_object_in_lambda(const string& literal, Value value) {

        auto object = new MemoryObject(value, value.type, this, 0, false, true, false, true, false);
        if (check_literal(literal)) delete_variable(literal);
        try {
            string_pool.emplace(literal, object);
        } catch (...) {
            return false;
        }
        return true;
    }

    bool add_object_in_func(const string& literal, Value value, bool is_const = false, bool is_static = false, bool is_final = false, bool is_global = false, bool is_private = false) {

        auto object = new MemoryObject(value, value.type, this, 0, is_const, is_static, is_final, is_global, is_private);
        if (check_literal(literal)) delete_variable(literal);
        try {
            string_pool.emplace(literal, object);
        } catch (...) {
            return false;
        }
        return true;
    }

    inline MemoryObject* get_variable(const string& literal){
        auto it = string_pool.find(literal);
        return it != string_pool.end() ? it->second : nullptr;
    }

    inline void set_object_value(const string& literal, Value new_value) {
        get_variable(literal)->value = new_value;
    }

    void link_objects(Memory& target_memory) {
        // Копируем все объекты из этой памяти в target_memory
        for (auto pair : string_pool) {
            if (!pair.second->modifiers.is_global)
                continue;
            
            target_memory.string_pool[pair.first] = pair.second;
        }
    }


    void copy_objects(Memory& target_memory) {
        for (auto& pair : string_pool) {
            // Копируем ВСЕ объекты, не только глобальные
            target_memory.add_object(pair.first, pair.second->value, pair.second->wait_type,
                                    pair.second->modifiers.is_const, pair.second->modifiers.is_static,
                                    pair.second->modifiers.is_final, pair.second->modifiers.is_global, pair.second->modifiers.is_private);
        }
    }

    inline Type get_wait_type(const string& literal) {
        return string_pool.find(literal)->second->wait_type;
    }


    inline bool check_literal(const string& literal) {
        return string_pool.find(literal) != string_pool.end();
    }

    ///////////////////////////////////////////////////////////////////////////
    inline bool is_final(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_final;
    }

    ///////////////////////////////////////////////////////////////////////////
    inline bool is_private(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_private;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline bool is_const(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_const;
    }


    ///////////////////////////////////////////////////////////////////////////

    inline bool is_static(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_static;
    }


    ///////////////////////////////////////////////////////////////////////////

    inline bool is_global(const string& name) {
        return string_pool.find(name)->second->modifiers.is_global;
    }



    ///////////////////////////////////////////////////////////////////////////

    void delete_variable(const string& name) {
        string_pool.erase(name);
    }





    void debug_print() {
        cout << MT::INFO + "Memory Dump:" << endl;
        for (const auto& [name, obj] : string_pool) {
            cout << "\tVariable Name: " << name << ", Type: " << obj->value.type.pool;
            try {
                if (obj->value.type.pool == STANDART_TYPE::INT.pool) {
                    cout << ", Value: " << any_cast<int64_t>(obj->value.data);
                } else if (obj->value.type.pool == STANDART_TYPE::DOUBLE.pool) {
                    cout << ", Value: " << any_cast<long double>(obj->value.data);
                } else if (obj->value.type.pool == STANDART_TYPE::BOOL.pool) {
                    cout << ", Value: " << (any_cast<bool>(obj->value.data) ? "true" : "false");
                } else if (obj->value.type.pool == STANDART_TYPE::STRING.pool) {
                    cout << ", Value: " << any_cast<string>(obj->value.data);
                }
            } catch (const std::bad_any_cast& e) {
                cout << ", Value: <bad cast: " << e.what() << ">";
            }
            cout << endl;
        }
    }
};

struct GlobalMemory {
    static map<int, MemoryObject*> all_objects;

    static void debug_print() {
        for (auto& pair : all_objects) {
            cout << pair.first << ": " << pair.second->value.type.pool << endl;
        }
    }
    
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
};

// ДОБАВЬТЕ ЭТОТ КОД - определение статической переменной
map<int, MemoryObject*> GlobalMemory::all_objects;

static GlobalMemory STATIC_MEMORY;