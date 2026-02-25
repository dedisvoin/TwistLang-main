#pragma once
#include "twist-values.cpp"
#include "twist-memory.cpp"

// В одном макросе генерируем все
#define GENERATE_NODE_TYPES \
    _(NODE_NUMBER) \
    _(NODE_STRING) \
    _(NODE_CHAR) \
    _(NODE_BOOL) \
    _(NODE_NULL) \
    _(NODE_LITERAL) \
    _(NODE_VALUE_HOLDER) \
    _(NODE_NAME_RESOLUTION) \
    _(NODE_SCOPES) \
    _(NODE_UNARY) \
    _(NODE_BINARY) \
    _(NODE_VARIABLE_DECLARATION) \
    _(NODE_OUT) \
    _(NODE_OUTLN) \
    _(NODE_VARIABLE_EQUAL) \
    _(NODE_BLOCK_OF_NODES) \
    _(NODE_IF) \
    _(NODE_NAMESPACE_DECLARATION) \
    _(NODE_BREAK) \
    _(NODE_CONTINUE) \
    _(NODE_WHILE) \
    _(NODE_DO_WHILE) \
    _(NODE_FOR) \
    _(NODE_BLOCK_OF_DECLARATIONS) \
    _(NODE_ADDRESS_OF) \
    _(NODE_DEREFERENCE) \
    _(NODE_LEFT_DEREFERENCE) \
    _(NODE_TYPEOF) \
    _(NODE_SIZEOF) \
    _(NODE_DELETE) \
    _(NODE_IF_EXPRESSION) \
    _(NODE_INPUT) \
    _(NODE_NAMESPACE_EXPRESSION) \
    _(NODE_ASSERT) \
    _(NODE_EXPRESSION_STATEMENT) \
    _(NODE_LAMBDA) \
    _(NODE_RETURN) \
    _(NODE_CALL) \
    _(NODE_NEW) \
    _(NODE_FUNCTION_TYPE) \
    _(NODE_FUNCTION_DECLARATION) \
    _(NODE_EXIT) \
    _(NODE_ARRAY_TYPE) \
    _(NODE_ARRAY) \
    _(NODE_GET_BY_INDEX) \
    _(NODE_ARRAY_PUSH) \
    _(NODE_OBJECT_RESOLUTION) \
    _(NODE_STRUCT_DECLARATION)

// Enum
enum NodeTypes {
    #define _(x) x,
    GENERATE_NODE_TYPES
    #undef _
    NODE_COUNT
};

// Массив имен
static const char* node_type_names[] = {
    #define _(x) #x,
    GENERATE_NODE_TYPES
    #undef _
};


inline const char* get_node_type_name(NodeTypes type) {
    return (type >= 0 && type < NODE_COUNT) ? node_type_names[type] : "NODE_UNKNOWN";
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"


#define NO_EXEC \
    void exec_from(Memory& _memory) override {} \

#define NO_EVAL \
    Value eval_from(Memory& _memory) override {} \


struct Node {
    NodeTypes NODE_TYPE;            // Node name
    virtual ~Node() = default;      // Destructor
    virtual Value eval_from(Memory& memory) = 0;
    virtual void exec_from(Memory& memory) = 0;
};
