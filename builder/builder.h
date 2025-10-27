#ifndef BUILDER_H
# define BUILDER_H

# include "core/node.h"
# include "core/typesystem.h"

namespace builder {

extern std::unordered_map<std::string, std::function<node_t*()>> g_builtin;

void init();

/**
 * Tries to create a builtin node.
 * Otherwise tries to create a node from the cached binaries.
 * Otherwise tries to load from persistent storage.
*/
node_t* create_node(const std::string& name);

template <typename BuiltinNode>
node_t* create_primitive();

bool persist_save(node_t* node);
node_t* persist_load(const std::string& name);

//----------------------------------------------------------------------------------------------------

template <typename BuiltinNode>
node_t* create_node() {
    auto it = g_builtin.find(id.name);
    if (it == g_builtin.end()) {
        return nullptr;
    }
    return it->second();
}

} // namespace builder

#endif // BUILDER_H
