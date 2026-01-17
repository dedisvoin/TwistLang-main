#include "string"
#pragma once

using namespace std;

struct Arg {
    void* type_expr = nullptr;
    void* default_parameter = nullptr;
    string name;
    bool is_const = false;
    bool is_final = false;

    Arg(string name, void* default_parameter, void* type_expr) : name(name), default_parameter(default_parameter), type_expr(type_expr) {}
};
