#include "compiler.h"
#include "nodes.h"

compiler_t COMPILER;

bool compiler_t::register_node(const std::vector<uint8_t>& binary) {
    std::string name;
    size_t ip = 0;
    while (ip < binary.size() && binary[ip] != 0) {
        name.push_back(binary[ip]);
        ++ip;
    }
    if (ip == binary.size()) {
        // throw std::runtime_error("unterminated component name");
        return false;
    }

    if (
        m_node_binaries.find(name) != m_node_binaries.end() ||
        m_primitive_nodes.find(name) != m_primitive_nodes.end()
    ) {
        return false;
        //throw std::runtime_error("node with the same name is already registered: '" + name + "'");
    }

    return m_node_binaries.emplace(std::move(name), binary).second;
}

node_t* compiler_t::create_node(std::string name) {
    node_t* result = nullptr;

    auto it = m_primitive_nodes.find(name);
    if (it != m_primitive_nodes.end()) {
        result = it->second();
    } else {
        result = new node_t;
        result->name(name);
    }

    return result;
}

bool compiler_t::expand_compound_node(std::string name, node_t* node) {
    auto it = m_node_binaries.find(name);
    if (it == m_node_binaries.end()) {
        return false;
    }

    if (binary_to_node(it->second, node)) {
        return true;
    }

    return true;
}

// todo: cleanup on failure
bool compiler_t::binary_to_node(const std::vector<uint8_t>& binary, node_t* out) {
    size_t ip = 0;

    std::string name;
    while (ip < binary.size() && binary[ip] != 0) {
        name.push_back(binary[ip]);
        ++ip;
    }
    if (ip == binary.size()) {
        return false;
        // throw std::runtime_error("unterminated component name in binary");
    }
    ++ip;

    out->name(std::move(name));

    std::vector<node_t*>& nodes = out->m_inner_nodes;

    while (ip < binary.size()) {
        const uint8_t op = binary[ip++];
        switch (op) {
        case OP_CREATE_NODE: {
            std::string node_name;
            while (ip < binary.size() && binary[ip] != 0) {
                node_name.push_back(binary[ip]);
                ++ip;
            }
            if (ip == binary.size()) {
                return false;
                // throw std::runtime_error("unterminated node name in binary");
            }
            assert(binary[ip] == 0);
            ++ip;
            node_t* node = nullptr;
            auto it = m_primitive_nodes.find(node_name);
            if (it != m_primitive_nodes.end()) {
                node = it->second();
            } else {
                node = new node_t;
            }
            assert(node);
            node->name(node_name);
            nodes.emplace_back(node);
        } break ;
        case OP_CONNECT_PORTS: {
            if (binary.size() < ip + 4) {
                return false;
                // throw std::runtime_error("expected 4 bytes for connect ports");
            }
            port_index_t from_node_index = (port_index_t) binary[ip++];
            port_index_t from_port_index = (port_index_t) binary[ip++];
            port_index_t to_node_index = (port_index_t) binary[ip++];
            port_index_t to_port_index = (port_index_t) binary[ip++];

            if (__PORT_SIZE < from_port_index) {
                return false;
                // throw std::runtime_error("from port index out of range: " + std::to_string(from_port_index));
            }

            if (__PORT_SIZE < to_port_index) {
                return false;
                // throw std::runtime_error("to port index out of range: " + std::to_string(to_port_index));
            }

            node_t* from_node = nullptr;
            if (from_node_index == (uint8_t) -1) {
                from_node = out;
            } else if (from_node_index < nodes.size()) {
                from_node = nodes[from_node_index];
            } else {
                return false;
                // throw std::runtime_error("from node index out of range: " + std::to_string(from_node_index));
            }

            node_t* to_node = nullptr;
            if (to_node_index == (uint8_t) -1) {
                to_node = out;
            } else if (to_node_index < nodes.size()) {
                to_node = nodes[to_node_index];
            } else {
                return false;
                // throw std::runtime_error("to node index out of range: " + std::to_string(to_node_index));
            }

            assert(from_node && to_node);
            from_node->connect(to_node, to_port_index, from_port_index);
        } break ;
        default: {
            return false;
            // throw std::runtime_error("unknown opcode: " + std::to_string(op));
        } break ;
        }
    }

    return true;
}

bool compiler_t::binary_to_human_readable(const std::vector<uint8_t>& binary, std::string& out) {
    std::string result;

    std::string result_node_name;
    size_t ip = 0;
    while (ip < binary.size() && binary[ip] != 0) {
        result_node_name.push_back(binary[ip]);
        ++ip;
    }
    if (ip == binary.size()) {
        return false;
        // throw std::runtime_error("unterminated component name in binary");
    }
    assert(binary[ip] == 0);
    ++ip;
    result += result_node_name + ":\n";

    std::unordered_map<uint8_t, std::string> component_names;

    while (ip < binary.size()) {
        uint8_t op = binary[ip++];
        switch (op) {
        case OP_CREATE_NODE: {
            std::string node_name;
            while (ip < binary.size() && binary[ip] != 0) {
                node_name.push_back(binary[ip]);
                ++ip;
            }
            if (ip == binary.size()) {
                return false;
                // throw std::runtime_error("unterminated node name in binary");
            }
            assert(binary[ip] == 0);
            ++ip;
            size_t node_index = component_names.size();
            component_names[node_index] = node_name;
            result += "  " + node_name + " " + std::to_string(node_index) + "\n";
        } break ;
        case OP_CONNECT_PORTS: {
            if (binary.size() < ip + 4) {
                throw std::runtime_error("expected 4 bytes for connect ports");
            }
            uint8_t from_node_index = binary[ip++];
            uint8_t from_port_index = binary[ip++];
            uint8_t to_node_index = binary[ip++];
            uint8_t to_port_index = binary[ip++];
            if (__PORT_SIZE < from_port_index) {
                return false;
                // throw std::runtime_error("from port index out of range: " + std::to_string(from_port_index));
            }
            if (__PORT_SIZE < to_port_index) {
                return false;
                // throw std::runtime_error("to port index out of range: " + std::to_string(to_port_index));
            }
            std::string from_node_name;
            if (from_node_index == (uint8_t) -1) {
                from_node_name = result_node_name;
            } else if (component_names.find(from_node_index) != component_names.end()) {
                from_node_name = component_names[from_node_index];
            } else {
                return false;
                // throw std::runtime_error("from node index out of range: " + std::to_string(from_node_index));
            }
            std::string to_node_name;
            if (to_node_index == (uint8_t) -1) {
                to_node_name = result_node_name;
            } else if (component_names.find(to_node_index) != component_names.end()) {
                to_node_name = component_names[to_node_index];
            } else {
                return false;
                // throw std::runtime_error("to node index out of range: " + std::to_string(to_node_index));
            }
            result += "  " + from_node_name;
            if (from_node_index != (uint8_t) -1) {
                result += " " + std::to_string(from_node_index);
            }
            result += " [" + std::to_string(from_port_index) + "] -> " "[" + std::to_string(to_port_index) + "] " + to_node_name;
            if (to_node_index != (uint8_t) -1) {
                result += " " + std::to_string(to_node_index);
            }
            result += "\n";
        } break ;
        default: {
            return false;
            // throw std::runtime_error("unknown opcode: " + std::to_string(op));
        } break ;
        }
    }

    out = result;

    return true;
}

std::vector<uint8_t> compiler_t::node_to_binary(node_t* node) {
    std::vector<uint8_t> result;

    for (char c : node->name()) {
        result.push_back((uint8_t) c);
    }
    result.push_back(0);

    std::unordered_set<node_t*> unvisited_nodes;

    std::unordered_map<node_t*, uint8_t> node_to_index;

    node_to_index[node] = (uint8_t) -1;
    unvisited_nodes.insert(node);
    std::unordered_set<node_t*> visited_nodes;
    while (!unvisited_nodes.empty()) {
        node_t* unvisited_node = *unvisited_nodes.begin();
        unvisited_nodes.erase(unvisited_node);
        visited_nodes.insert(unvisited_node);

        if (unvisited_node != node) {
            result.push_back(OP_CREATE_NODE);
            for (char c : unvisited_node->name()) {
                result.push_back((uint8_t) c);
            }
            result.push_back(0);
            assert(node_to_index.size() < UINT8_MAX + 1);
            uint8_t node_index = (uint8_t) node_to_index.size() - 1;
            node_to_index[unvisited_node] = node_index;
        }

        for (int i = 0; i < __PORT_SIZE; ++i) {
            node_t* connection = unvisited_node->connection((port_index_t) i);
            if (connection && visited_nodes.find(connection) == visited_nodes.end()) {
                unvisited_nodes.insert(connection);
            }
        }
    }

    while (!visited_nodes.empty()) {
        node_t* from_node = *visited_nodes.begin();
        visited_nodes.erase(from_node);

        for (uint8_t from_port_index = 0; from_port_index < __PORT_SIZE; ++from_port_index) {
            if (from_node->is_connected((port_index_t) from_port_index)) {
                node_t::port_t& from_port = from_node->m_ports[from_port_index];
                node_t* to_node = from_port.m_connection;
                uint8_t to_port_index = from_port.m_connection_port_index;
                assert(to_node);

                auto from_it = node_to_index.find(from_node);
                assert(from_it != node_to_index.end());
                auto to_it = node_to_index.find(to_node);
                assert(to_it != node_to_index.end());

                result.push_back(OP_CONNECT_PORTS);
                result.push_back(from_it->second);
                result.push_back(from_port_index);
                result.push_back(to_it->second);
                result.push_back(to_port_index);                
            }
        }
    }

    return result;
}

// bool compiler_t::are_binaries_equal(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
//     if (a.size() != b.size()) {
//         return false;
//     }

//     std::string a_name;
//     size_t ip_a = 0;
//     while (ip_a < a.size() && a[ip_a] != 0) {
//         a_name.push_back(a[ip_a]);
//         ++ip_a;
//     }
//     if (ip_a == a.size()) {
//         return false;
//     }
//     assert(a[ip_a] == 0);
//     ++ip_a;

//     std::string b_name;
//     size_t ip_b = 0;
//     while (ip_b < b.size() && b[ip_b] != 0) {
//         b_name.push_back(b[ip_b]);
//         ++ip_b;
//     }
//     if (ip_b == b.size()) {
//         return false;
//     }
//     assert(b[ip_b] == 0);

//     if (a_name != b_name) {
//         return false;
//     }

//     std::unordered_set<uint32_t> a_connections;
//     std::unordered_map<std::string, uint8_t> a_node_to_index;
//     while (ip_a < a.size()) {
//     }

//     return true;
// }

bool compiler_t::exists(std::string name) {
    return m_node_binaries.find(name) != m_node_binaries.end() || m_primitive_nodes.find(name) != m_primitive_nodes.end();
}

void compiler_t::clear() {
    m_node_binaries.clear();
    m_primitive_nodes.clear();
}
