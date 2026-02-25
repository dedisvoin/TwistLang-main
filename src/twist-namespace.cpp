#include "twist-values.cpp"
#include "twist-memory.cpp"
#include <memory>

#pragma once

struct Namespace {
    shared_ptr<Memory> memory;
    string name;

    Namespace(shared_ptr<Memory> memory, string name) : memory(memory), name(name) {};
    Namespace(string name) : name(name) {};
};

Value NewNamespace(shared_ptr<Memory> memory, const string& name) {
    return Value(STANDART_TYPE::NAMESPACE, Namespace(memory, name));
}

Value NewNamespace(const string& name) {
    return Value(STANDART_TYPE::NAMESPACE, Namespace(name));
}