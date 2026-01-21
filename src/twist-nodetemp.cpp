

#pragma once
#include "twist-values.cpp"
#include "twist-memory.cpp"



struct Node {
    string NAME;                    // Node name
    virtual ~Node() = default;      // Destructor
    virtual Value eval_from(Memory& memory) = 0;
    virtual void exec_from(Memory& memory) = 0;
};
