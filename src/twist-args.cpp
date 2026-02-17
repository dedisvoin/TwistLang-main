#include "string"
#include "twist-nodetemp.cpp"
#include <memory>

#pragma once

using namespace std;

struct Arg {
    unique_ptr<Node> type_expr = nullptr;
    unique_ptr<Node> default_parameter = nullptr;
    string name;
    bool is_const = false;
    bool is_final = false;
    bool is_static = false;
    bool is_global = false;
    
    // Для сборки аргументов в один массив
    bool is_variadic = false;
    unique_ptr<Node> variadic_size = nullptr;

    Arg(string name) : name(name) {
        
    }
};


