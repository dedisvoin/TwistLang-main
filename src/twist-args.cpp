#include "string"
#include "twist-nodetemp.cpp"
#include "twist-values.cpp"

#pragma once

using namespace std;

struct Arg {
    unique_ptr<Node> type_expr = nullptr;
    unique_ptr<Node> default_parameter = nullptr;
    string name;
    bool is_const = false;
    bool is_final = false;
    bool is_static = false;
    Type default_type = STANDART_TYPE::NULL_T;
    Value default_value = NewNull();

    Arg(string name) : name(name) {
        
    }
};


