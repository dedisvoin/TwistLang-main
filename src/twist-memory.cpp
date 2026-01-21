// memory.cpp
#include "twist-values.cpp"
#include "twist-utils.cpp"
#include <map>
#include <string>
#include <iostream>
#include <unordered_map>

#pragma once

typedef int Address;

// global address counter
static Address GLOBAL_ADDRESS = 0;

struct Modifiers {
    bool is_const;
    bool is_static;
    bool is_final;
    bool is_global;
};

// memory object
struct MemoryObject {
    Value value;
    Type wait_type;

    
    Modifiers modifiers;

    void* memory_pointer;
    Address address;

    MemoryObject(Value value, Type wait_type,
                 bool is_const, bool is_static, bool is_final, bool is_global,
                 void* memory
    ) : value(value), wait_type(wait_type), modifiers({is_const, is_static, is_final, is_global}), memory_pointer(memory) {
        address = GLOBAL_ADDRESS;
        GLOBAL_ADDRESS++;
    }

    MemoryObject* copy() {
        return new MemoryObject(value, wait_type, modifiers.is_const, modifiers.is_static, modifiers.is_final, modifiers.is_global, memory_pointer);
    }
};

struct Memory {
    unordered_map<string, MemoryObject*> string_pool;
    unordered_map<Address, MemoryObject*> address_pool;

    void clear() {
        string_pool.clear();
        address_pool.clear();
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
    bool add_object(const string& literal, Value value, Type wait_type, bool is_const = false, bool is_static = false, bool is_final = false, bool is_global = false) {
        try {
            auto object = new MemoryObject(value, wait_type, is_const, is_static, is_final, is_global, this);
            string_pool.emplace(literal, object);
            address_pool.emplace(object->address, object);
        } catch (...) {
            return false;
        }
        return true;
    }

    bool add_object(const string& literal, MemoryObject* object) {
        
        try {
            string_pool.emplace(literal, object);
            address_pool.emplace(object->address, object);
        } catch (...) {
            return false;
        }
        return true;
    }

    bool add_object_in_lambda(const string& literal, Value value) {
        auto object = new MemoryObject(value, *value.type, false, true, false, true, this);
        if (check_literal(literal)) delete_variable(literal);
        try {
            string_pool.emplace(literal, object);
            address_pool.emplace(object->address, object);
        } catch (...) {
            return false;
        }
        return true;
    }

    MemoryObject* get_variable(const string& literal){
        auto it = string_pool.find(literal);
        return it != string_pool.end() ? it->second : nullptr;
    }

    MemoryObject* get_variable(const Address addr) {
        auto it = address_pool.find(addr);
        return it != address_pool.end() ? it->second : nullptr;
    }

    void set_object_value(const string& literal, Value new_value) {
        get_variable(literal)->value = new_value;
    }

    void set_object_value(const Address addr, Value new_value) {
        get_variable(addr)->value = new_value;
    }

    bool check_address(const Address addr) {
        return address_pool.find(addr) != address_pool.end();
    }

    void link_objects(Memory& target_memory) {
        // Копируем все объекты из этой памяти в target_memory
        for (auto& pair : string_pool) {
            if (!pair.second->modifiers.is_global)
                continue;
            
            target_memory.string_pool[pair.first] = pair.second;
            target_memory.address_pool[pair.second->address] = pair.second;
        }
    }


    void copy_objects(Memory& target_memory) {
        for (auto& pair : string_pool) {
            // Копируем ВСЕ объекты, не только глобальные
            target_memory.add_object(pair.first, pair.second->value, pair.second->wait_type,
                                    pair.second->modifiers.is_const, pair.second->modifiers.is_static,
                                    pair.second->modifiers.is_final, pair.second->modifiers.is_global);
        }
    }

    unique_ptr<Memory> clone() const {
        auto new_memory = make_unique<Memory>();
        
        // Копируем все переменные
        for (const auto& [name, obj] : string_pool) {
            new_memory->add_object(name, obj->value, obj->wait_type,
                                  obj->modifiers.is_const, obj->modifiers.is_static,
                                  obj->modifiers.is_final, obj->modifiers.is_global);
        }
        
        return new_memory;
    }

    Type get_wait_type(const string& literal) {
        return string_pool.find(literal)->second->wait_type;
    }

    Type get_wait_type(const Address addr) {
        return address_pool.find(addr)->second->wait_type;
    }

    bool check_literal(const string& literal) {
        return string_pool.find(literal) != string_pool.end();
    }

    ///////////////////////////////////////////////////////////////////////////
    inline bool is_final(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_final;
    }

    inline bool is_final(const Address addr) {
        return address_pool.find(addr)->second->modifiers.is_final;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline bool is_const(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_const;
    }

    inline bool is_const(const Address addr) {
        return address_pool.find(addr)->second->modifiers.is_const;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline bool is_static(const string& literal) {
        return string_pool.find(literal)->second->modifiers.is_static;
    }

    inline bool is_static(const Address addr) {
        return address_pool.find(addr)->second->modifiers.is_static;
    }

    ///////////////////////////////////////////////////////////////////////////

    inline bool is_global(const string& name) {
        return string_pool.find(name)->second->modifiers.is_global;
    }

    inline bool is_global(const Address addr) {
        return address_pool.find(addr)->second->modifiers.is_global;
    }

    ///////////////////////////////////////////////////////////////////////////

    void delete_variable(const string& name, Address addr) {
        address_pool.erase(addr);
        string_pool.erase(name);
    }

    void delete_variable(const string& name) {
        auto object = get_variable(name);
        address_pool.erase(object->address);
        string_pool.erase(name);
    }

    

    void debug_print() {
        cout << MT::INFO + "Memory Dump:" << endl;
        for (const auto& [name, obj] : string_pool) {
            cout << "\tVariable Name: " << name << ", Type: " << obj->value.type->name;
            try {
                if (obj->value.type->name == STANDART_TYPE::INT.name) {
                    cout << ", Value: " << any_cast<int64_t>(obj->value.data);
                } else if (obj->value.type->name == STANDART_TYPE::DOUBLE.name) {
                    cout << ", Value: " << any_cast<long double>(obj->value.data);
                } else if (obj->value.type->name == STANDART_TYPE::BOOL.name) {
                    cout << ", Value: " << (any_cast<bool>(obj->value.data) ? "true" : "false");
                } else if (obj->value.type->name == STANDART_TYPE::STRING.name) {
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
    
    static void register_object(MemoryObject* obj) {
        all_objects[obj->address] = obj;
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