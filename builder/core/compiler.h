#ifndef COMPILER_H
# define COMPILER_H

# include "node.h"

enum op_t {
    OP_CREATE_NODE,   // [op <u8>, name <null-terminated string>]
    OP_CONNECT_PORTS, // [op <u8>, from_node_index <u8>, from_node_port_index <u8>, to_node_index <u8>, to_node_port_index <u8>]
                      // - (node_index == (uint8_t) -1) refers to result node
};

class compiler_t {
public:
    bool register_node(const std::vector<uint8_t>& binary);

    template <typename T>
    bool register_node(std::string name);

    node_t* create_node(std::string name);

    bool expand_compound_node(std::string name, node_t* node);

    bool binary_to_node(const std::vector<uint8_t>& binary, node_t* out);
    bool binary_to_human_readable(const std::vector<uint8_t>& binary, std::string& out);
    std::vector<uint8_t> node_to_binary(node_t* node);

    // todo: implement, need to realias nodes..
    // bool are_binaries_equal(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);

    bool exists(std::string node);

    void clear();

private:
    std::unordered_map<std::string, std::vector<uint8_t>> m_node_binaries;
    std::unordered_map<std::string, std::function<node_t*()>> m_primitive_nodes;
};

extern compiler_t COMPILER;

template <typename T>
bool compiler_t::register_node(std::string name) {
    if (exists(name)) {
        return false;
    }

    return m_primitive_nodes.emplace(name, [name] -> node_t* {
        node_t* result = new T();

        result->name(name);

        return result;
    }).second;
}

#endif // COMPILER_H
