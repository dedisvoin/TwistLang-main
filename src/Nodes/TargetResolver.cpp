// TargetResolver.cpp
#include "../twist-nodetemp.cpp"
#include "../twist-errors.cpp"
#include "NodeLiteral.cpp"
#include "NodeObjectResolution.cpp"
#include "NodeNamespaceResolution.cpp"

#ifndef TARGET_RESOLVER_CPP
#define TARGET_RESOLVER_CPP

pair<Memory*, string> resolveTargetMemory(Node* node, Memory& current_memory) {
    if (node->NODE_TYPE == NodeTypes::NODE_LITERAL) {
        NodeLiteral* lit = static_cast<NodeLiteral*>(node);
        
        return {&current_memory, lit->name};
    }
    else if (node->NODE_TYPE == NodeTypes::NODE_OBJECT_RESOLUTION) {
        NodeObjectResolution* resolution = static_cast<NodeObjectResolution*>(node);
        Value obj_value = resolution->obj_expr->eval_from(current_memory);
        if (STANDART_TYPE::UNTYPED.is_sub_type(obj_value.type)) {
            ERROR::InvalidAccessorType(resolution->start, resolution->end, obj_value.type.pool);
        }
        auto& obj = any_cast<Struct&>(obj_value.data);
        return {obj.memory.get(), resolution->current_name};
    }
    else if (node->NODE_TYPE == NodeTypes::NODE_NAME_RESOLUTION) {
        NodeNamespaceResolution* resolution = static_cast<NodeNamespaceResolution*>(node);
        Value ns_value = resolution->namespace_expr->eval_from(current_memory);
        if (ns_value.type != STANDART_TYPE::NAMESPACE) {
            ERROR::InvalidAccessorType(resolution->start, resolution->end, ns_value.type.pool);
        }
        auto& ns = any_cast<Namespace&>(ns_value.data);
        return {ns.memory.get(), resolution->name};
    }
    else {
        // Неподдерживаемый тип узла
        return {nullptr, ""}; // заглушка
    }
}

#endif