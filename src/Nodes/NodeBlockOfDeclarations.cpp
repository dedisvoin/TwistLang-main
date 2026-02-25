#include "../twist-nodetemp.cpp"

#include "NodeVariableDeclaration.cpp"
#include "NodeNamespaceDeclaration.cpp"
#include "NodeFunctionDeclaration.cpp"
#include "NodeStructDeclaration.cpp"


struct NodeBlockDecl : public Node { NO_EVAL
    bool is_static = false;
    bool is_final = false;
    bool is_const = false;
    bool is_global = false;
    bool is_private = false;

    vector<unique_ptr<Node>> decls;

    NodeBlockDecl(vector<unique_ptr<Node>> decls) : decls(std::move(decls)) {
        this->NODE_TYPE = NodeTypes::NODE_BLOCK_OF_DECLARATIONS;
    }

    void exec_from(Memory& _memory) override {
        for (int i = 0; i < decls.size(); i++) {
            if (decls[i]->NODE_TYPE == NodeTypes::NODE_VARIABLE_DECLARATION) {
                ((NodeVariableDeclaration*)decls[i].get())->is_const = is_const;
                ((NodeVariableDeclaration*)decls[i].get())->is_static = is_static;
                ((NodeVariableDeclaration*)decls[i].get())->is_final = is_final;
                ((NodeVariableDeclaration*)decls[i].get())->is_global = is_global;
                ((NodeVariableDeclaration*)decls[i].get())->is_private = is_private;
            }
            else if (decls[i]->NODE_TYPE == NodeTypes::NODE_NAMESPACE_DECLARATION) {
                ((NodeNamespaceDeclaration*)decls[i].get())->is_const = is_const;
                ((NodeNamespaceDeclaration*)decls[i].get())->is_static = is_static;
                ((NodeNamespaceDeclaration*)decls[i].get())->is_final = is_final;
                ((NodeNamespaceDeclaration*)decls[i].get())->is_global = is_global;
                ((NodeNamespaceDeclaration*)decls[i].get())->is_private = is_private;
            }
            else if (decls[i]->NODE_TYPE == NodeTypes::NODE_FUNCTION_DECLARATION) {
                ((NodeFunctionDeclaration*)decls[i].get())->is_const = is_const;
                ((NodeFunctionDeclaration*)decls[i].get())->is_static = is_static;
                ((NodeFunctionDeclaration*)decls[i].get())->is_final = is_final;
                ((NodeFunctionDeclaration*)decls[i].get())->is_global = is_global;
                ((NodeFunctionDeclaration*)decls[i].get())->is_private = is_private;
            }
            else if (decls[i]->NODE_TYPE == NodeTypes::NODE_STRUCT_DECLARATION) {
                ((NodeStructDeclaration*)decls[i].get())->is_const = is_const;
                ((NodeStructDeclaration*)decls[i].get())->is_static = is_static;
                ((NodeStructDeclaration*)decls[i].get())->is_final = is_final;
                ((NodeStructDeclaration*)decls[i].get())->is_global = is_global;
                ((NodeStructDeclaration*)decls[i].get())->is_private = is_private;
            }
            decls[i]->exec_from(_memory);
        }
    }
};