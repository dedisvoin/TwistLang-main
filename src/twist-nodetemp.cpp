#pragma once
#include "twist-values.cpp"
#include "twist-memory.cpp"

enum NodeTypes {
    NODE_NUMBER,
    NODE_STRING,
    NODE_CHAR,
    NODE_BOOL,
    NODE_NULL,
    NODE_LITERAL,
    NODE_VALUE_HOLDER,
    NODE_NAME_RESOLUTION,
    NODE_SCOPES,
    NODE_UNARY,
    NODE_BINARY,
    NODE_VARIABLE_DECLARATION,
    NODE_OUT,
    NODE_OUTLN,
    NODE_VARIABLE_EQUAL,
    NODE_BLOCK_OF_NODES,
    NODE_IF,
    NODE_NAMESPACE_DECLARATION,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_WHILE,
    NODE_DO_WHILE,
    NODE_FOR,
    NODE_BLOCK_OF_DECLARATIONS,
    NODE_ADDRESS_OF,
    NODE_DEREFERENCE,
    NODE_LEFT_DEREFERENCE,
    NODE_TYPEOF,
    NODE_SIZEOF,
    NODE_DELETE,
    NODE_IF_EXPRESSION,
    NODE_INPUT,
    NODE_NAMESPACE_EXPRESSION,
    NODE_ASSERT,
    NODE_EXPRESSION_STATEMENT,
    NODE_LAMBDA,
    NODE_RETURN,
    NODE_CALL,
    NODE_NEW,
    NODE_FUNCTION_TYPE,
    NODE_FUNCTION_DECLARATION,
    NODE_EXIT,
    NODE_ARRAY_TYPE,
    NODE_ARRAY,
    NODE_GET_BY_INDEX,
    NODE_ARRAY_PUSH
};


struct Node {
    NodeTypes NODE_TYPE;                    // Node name
    virtual ~Node() = default;      // Destructor
    virtual Value eval_from(Memory& memory) = 0;
    virtual void exec_from(Memory& memory) = 0;
};
