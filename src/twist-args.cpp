#include "string"
#include "twist-nodetemp.cpp"

#pragma once

using namespace std;

struct Arg {
    unique_ptr<Node> type_expr = nullptr;
    unique_ptr<Node> default_parameter = nullptr;
    string name;
    bool is_const = false;
    bool is_final = false;
    bool is_static = false;

    Arg(string name) : name(name) {
        
    }
};
