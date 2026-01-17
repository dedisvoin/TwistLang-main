#include "twist-values.cpp"
#include "twist-memory.cpp"

struct Namespace {
    Memory memory;
    string name;

    Namespace(Memory memory, string name) : memory(memory), name(name) {};
};

Value NewNamespace(Memory memory, const string& name) {
    return Value(make_unique<Type>(STANDART_TYPE::NAMESPACE), Namespace(memory, name));
}