#include "twist-values.cpp"
#include "twist-memory.cpp"
#include <memory>

#pragma once

struct Struct {
    shared_ptr<Memory> memory = nullptr;
    string name;
    Type type;

    Struct(shared_ptr<Memory> memory, string name) : memory(memory), name(name) {};
    Struct(string name) : name(name) {};
};

Value NewStruct(shared_ptr<Memory> memory, const string& name) {
    auto T = Type(name);
    auto S = Struct(memory, name);
    S.type = T;
    return Value(T, S);
}

Value NewStruct(const string& name) {
    auto T = Type(name);
    auto S = Struct(name);
    S.type = T;
    return Value(T, S);
}