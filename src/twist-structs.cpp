#include "twist-values.cpp"
#include "twist-memory.cpp"
#include "twist-nodetemp.cpp"

#pragma once

struct Struct {
    Memory* memory = nullptr;
    string name;
    Type type;
    Node* body = nullptr;

    Struct(Memory* memory, string name) : memory(memory), name(name) {};
    Struct(string name) : name(name) {};
};

Value NewStruct(Memory* memory, const string& name) {
    auto T = Type(name);
    auto S = new Struct(memory, name);
    S->type = T;
    return Value(T, S);
}

Value NewStruct(const string& name) {
    auto T = Type(name);
    auto S = new Struct(name);
    S->type = T;
    return Value(T, S);
}