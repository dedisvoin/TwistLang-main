#include "twist-values.cpp"
#include "twist-memory.cpp"
#include <memory>

#pragma once

struct Namespace {
    Memory* memory;
    string name;

    Namespace(Memory* memory, string name) : memory(memory), name(name) {};
    Namespace(string name) : name(name) {};
};

Value NewNamespace(Memory* memory, const string& name) {
    return Value(STANDART_TYPE::NAMESPACE, new Namespace(memory, name));
}

Value NewNamespace(const string& name) {
    return Value(STANDART_TYPE::NAMESPACE, new Namespace(name));
}