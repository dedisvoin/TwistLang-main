// memory-dump.hpp
#pragma once

#include "../src/json.cpp"
#include "twist-memory.cpp"
#include "twist-values.cpp"
#include "twist-structs.cpp"
#include "twist-namespace.cpp"
#include "twist-lambda.cpp"
#include <fstream>
#include <set>

using json = nlohmann::json;

class MemoryDumper {
private:
    std::set<int> visited;
    
    // Проверка, является ли объект стандартным типом
    bool is_standart_object(MemoryObject* obj) {
        if (!obj) return true;
        return obj->is_standart;
    }
    
    json serialize_value(Value& v, bool recursive = true) {
        json res;
        res["type"] = v.type.pool;
        
        if (v.type == STANDART_TYPE::INT) res["value"] = std::any_cast<int64_t>(v.data);
        else if (v.type == STANDART_TYPE::DOUBLE) res["value"] = std::any_cast<long double>(v.data);
        else if (v.type == STANDART_TYPE::BOOL) res["value"] = std::any_cast<bool>(v.data);
        else if (v.type == STANDART_TYPE::STRING) res["value"] = std::any_cast<std::string>(v.data);
        else if (v.type == STANDART_TYPE::CHAR) res["value"] = std::string(1, std::any_cast<char>(v.data));
        else if (v.type == STANDART_TYPE::NULL_T) res["value"] = nullptr;
        else if (v.type == STANDART_TYPE::TYPE) {
            res["value"] = std::any_cast<Type>(v.data).pool; 
        }
        else if (v.type.is_pointer()) {
            int addr = std::any_cast<int>(v.data);
            res["value"] = addr;
            if (recursive && addr && visited.find(addr) == visited.end()) {
                if (auto* target = GlobalMemory::get_by_address(addr)) {
                    // Проверяем, не является ли целевой объект стандартным
                    if (!is_standart_object(target)) {
                        res["points_to"] = serialize_memory_object(target);
                    } else {
                        res["points_to"] = nullptr;
                    }
                }
            }
            
        }
        else if (v.type == STANDART_TYPE::LAMBDA) {
            try {
                auto* lam = std::any_cast<Lambda*>(v.data);
                if (lam) {
                    res["lambda"]["name"] = lam->name;
                    if (lam->memory) {
                        for (auto& [name, obj] : lam->memory->string_pool) {
                            if (!is_standart_object(obj)) {
                                res["lambda"]["captures"][name] = serialize_memory_object(obj);
                            }
                        }
                    }
                }
            } catch(...) { res["value"] = "<lambda>"; }
        }
        else if (v.type == STANDART_TYPE::NAMESPACE) {
            try {
                auto* ns = std::any_cast<Namespace*>(v.data);
                if (ns && ns->memory) {
                    for (auto& [name, obj] : ns->memory->string_pool) {
                        if (!is_standart_object(obj)) {
                            res["namespace"][name] = serialize_memory_object(obj);
                        }
                    }
                }
            } catch(...) { res["value"] = "<namespace>"; }
        }
        else if (IsStructure(v.type)) {
            try {
                auto* str = std::any_cast<Struct*>(v.data);
                if (str && str->memory) {
                    res["struct_name"] = str->name;
                    for (auto& [name, obj] : str->memory->string_pool) {
                        if (!is_standart_object(obj)) {
                            res["fields"][name] = serialize_memory_object(obj);
                        }
                    }
                }
            } catch(...) { res["value"] = "<struct>"; }
        }
        else res["value"] = "<unknown>";
        
        return res;
    }
    
    json serialize_memory_object(MemoryObject* obj) {
        // Пропускаем стандартные объекты
        if (!obj || is_standart_object(obj)) {
            return nullptr;
        }
        
        // Проверка на циклы
        if (visited.find(obj->address) != visited.end()) {
            return {{"cyclic", true}};
        }
        
        visited.insert(obj->address);
        
        return {
            {"addr", obj->address},
            {"name", obj->var_name},
            {"type", obj->wait_type.pool},
            {"value", serialize_value(obj->value, true)}
        };
    }
    
public:
    void dump(Memory* memory, const std::string& file = "dump.json") {
        visited.clear();
        json out;
        
        for (auto& [name, obj] : memory->string_pool) {
            // Пропускаем стандартные объекты
            if (!is_standart_object(obj)) {
                out["objects"][name] = serialize_memory_object(obj);
            }
        }
        
        std::ofstream f(file);
        if (f.is_open()) {
            f << out.dump(4);
            std::cout << "[Dump] Saved to " << file << std::endl;
        } else {
            std::cerr << "[Dump] Failed to open " << file << std::endl;
        }
    }
};

inline void dump_memory(Memory* mem, const std::string& file = "dump.json") {
    MemoryDumper().dump(mem, file);
}