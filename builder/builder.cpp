#include "builder.h"
#include "builtins/builtins.h"

namespace builder {

enum op_t {
    OP_CREATE_NODE,   // [op <u8>, left <i16: i8 high, i8 low>, right <i16>, top <i16>, bottom <i16>, name <null-terminated string>]
    OP_CONNECT_PORTS, // [op <u8>, from_node_index <u8>, from_node_port_index <u8>, to_node_index <u8>, to_node_port_index <u8>]
                      // - (node_index == (uint8_t) -1) refers to result node
};

// todo: implement with C++'s filesystem
const std::string PROGRAMS_DIR = "programs/";

static std::string to_bin_file_path(const std::string& name) {
    return PROGRAMS_DIR + "/" + name + ".bin";
}

static std::string to_human_readable_file_path(const std::string& name) {
    return PROGRAMS_DIR + "/" + name + ".txt";
}

std::unordered_map<std::string, std::function<node_t*()>> g_builtin;
std::unordered_map<std::string, std::vector<uint8_t>> g_binaries;

template <typename T>
static bool register_builtin(node_id_t id) {
    if (g_builtin.find(id.name) != g_builtin.end()) {
        return false;
    }

    return g_builtin.emplace(id.name, [id] -> node_t* {
        node_t* result = new T();

        result->id(id);

        return result;
    }).second;
}

template <typename From, typename To>
static void register_coercion(bool (*coercion_procedure)(From*, To*)) {
    TYPESYSTEM.register_coercion<From, To>(coercion_procedure);
}

template <typename From, typename To>
static bool coerce(From* from, To* to) {
    return TYPESYSTEM.coerce<From, To>(from, to);
}

// todo: cleanup on failure
static bool node_binary_to_node(node_binary_t* in, node_t* out) {
    size_t ip = 0;

    {
        node_id_t id;

        while (ip < in->binary.size() && in->binary[ip] != 0) {
            id.name.push_back(in->binary[ip]);
            ++ip;
        }
        if (ip == in->binary.size()) {
            return false;
            // throw std::runtime_error("unterminated component name in binary");
        }
        ++ip;

        out->id(std::move(id));
    }

    std::vector<node_t*>& nodes = out->inner_nodes();

    while (ip < in->binary.size()) {
        const uint8_t op = in->binary[ip++];
        switch (op) {
        case OP_CREATE_NODE: {
            node_id_t node_id;
            while (ip < in->binary.size() && in->binary[ip] != 0) {
                node_id.name.push_back(in->binary[ip]);
                ++ip;
            }
            if (ip == in->binary.size()) {
                return false;
                // throw std::runtime_error("unterminated node name in binary");
            }
            assert(in->binary[ip] == 0);
            ++ip;

            if (in->binary.size() < ip + 8) {
                return false;
                // throw std::runtime_error("expected 8 bytes for node dimensions");
            }

            int left = (int16_t) in->binary[ip++] << 8;
            left |= (int16_t) in->binary[ip++];
            int right = (int16_t) in->binary[ip++] << 8;
            right |= (int16_t) in->binary[ip++];
            int top = (int16_t) in->binary[ip++] << 8;
            top |= (int16_t) in->binary[ip++];
            int bottom = (int16_t) in->binary[ip++] << 8;
            bottom |= (int16_t) in->binary[ip++];

            node_t* node;
            if (!coerce(&node_id, &node)) {
                return false;
            }
            assert(node);
            node->left(left);
            node->right(right);
            node->top(top);
            node->bottom(bottom);
            node->finalize_dimensions();
            nodes.emplace_back(node);
        } break ;
        case OP_CONNECT_PORTS: {
            if (in->binary.size() < ip + 4) {
                return false;
                // throw std::runtime_error("expected 4 bytes for connect ports");
            }
            port_index_t from_node_index = (port_index_t) in->binary[ip++];
            port_index_t from_port_index = (port_index_t) in->binary[ip++];
            port_index_t to_node_index = (port_index_t) in->binary[ip++];
            port_index_t to_port_index = (port_index_t) in->binary[ip++];

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

static bool node_binary_to_node_assembly(node_binary_t* in, node_assembly_t* out) {
    std::string result_node_name;
    size_t ip = 0;
    while (ip < in->binary.size() && in->binary[ip] != 0) {
        result_node_name.push_back(in->binary[ip]);
        ++ip;
    }
    if (ip == in->binary.size()) {
        return false;
        // throw std::runtime_error("unterminated component name in binary");
    }
    assert(in->binary[ip] == 0);
    ++ip;
    out->assembly += result_node_name + ":\n";

    std::unordered_map<uint8_t, std::string> component_names;

    while (ip < in->binary.size()) {
        uint8_t op = in->binary[ip++];
        switch (op) {
        case OP_CREATE_NODE: {
            std::string node_name;
            while (ip < in->binary.size() && in->binary[ip] != 0) {
                node_name.push_back(in->binary[ip]);
                ++ip;
            }
            if (ip == in->binary.size()) {
                return false;
                // throw std::runtime_error("unterminated node name in binary");
            }
            assert(in->binary[ip] == 0);
            ++ip;

            if (in->binary.size() < ip + 8) {
                return false;
                // throw std::runtime_error("expected 8 bytes for node dimensions");
            }
            int left = (int16_t) in->binary[ip++] << 8;
            left |= (int16_t) in->binary[ip++];
            int right = (int16_t) in->binary[ip++] << 8;
            right |= (int16_t) in->binary[ip++];
            int top = (int16_t) in->binary[ip++] << 8;
            top |= (int16_t) in->binary[ip++];
            int bottom = (int16_t) in->binary[ip++] << 8;
            bottom |= (int16_t) in->binary[ip++];

            size_t node_index = component_names.size();
            component_names[node_index] = node_name;
            out->assembly += "  " + node_name + " " + std::to_string(node_index) + " (" + std::to_string(left) + ", " + std::to_string(right) + ", " + std::to_string(top) + ", " + std::to_string(bottom) + ")\n";
        } break ;
        case OP_CONNECT_PORTS: {
            if (in->binary.size() < ip + 4) {
                throw std::runtime_error("expected 4 bytes for connect ports");
            }
            uint8_t from_node_index = in->binary[ip++];
            uint8_t from_port_index = in->binary[ip++];
            uint8_t to_node_index = in->binary[ip++];
            uint8_t to_port_index = in->binary[ip++];
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
            out->assembly += "  " + from_node_name;
            if (from_node_index != (uint8_t) -1) {
                out->assembly += " " + std::to_string(from_node_index);
            }
            out->assembly += " [" + std::to_string(from_port_index) + "] -> " "[" + std::to_string(to_port_index) + "] " + to_node_name;
            if (to_node_index != (uint8_t) -1) {
                out->assembly += " " + std::to_string(to_node_index);
            }
            out->assembly += "\n";
        } break ;
        default: {
            return false;
            // throw std::runtime_error("unknown opcode: " + std::to_string(op));
        } break ;
        }
    }

    return true;
}

bool node_to_node_binary(node_t* in, node_binary_t* out) {
    for (char c : in->id().name) {
        out->binary.push_back((uint8_t) c);
    }
    out->binary.push_back(0);

    std::unordered_set<node_t*> unvisited_nodes;

    std::unordered_map<node_t*, uint8_t> node_to_index;

    node_to_index[in] = (uint8_t) -1;
    unvisited_nodes.insert(in);
    std::unordered_set<node_t*> visited_nodes;
    while (!unvisited_nodes.empty()) {
        node_t* unvisited_node = *unvisited_nodes.begin();
        unvisited_nodes.erase(unvisited_node);
        visited_nodes.insert(unvisited_node);

        if (unvisited_node != in) {
            out->binary.push_back(OP_CREATE_NODE);
            for (char c : unvisited_node->id().name) {
                out->binary.push_back((uint8_t) c);
            }
            out->binary.push_back(0);
            int16_t left = (int16_t) unvisited_node->left();
            out->binary.push_back((uint8_t) (left >> 8));
            out->binary.push_back((uint8_t) (left & 0xFF));
            int16_t right = (int16_t) unvisited_node->right();
            out->binary.push_back((uint8_t) (right >> 8));
            out->binary.push_back((uint8_t) (right & 0xFF));
            int16_t top = (int16_t) unvisited_node->top();
            out->binary.push_back((uint8_t) (top >> 8));
            out->binary.push_back((uint8_t) (top & 0xFF));
            int16_t bottom = (int16_t) unvisited_node->bottom();
            out->binary.push_back((uint8_t) (bottom >> 8));
            out->binary.push_back((uint8_t) (bottom & 0xFF));
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
                node_t::port_t& from_port = from_node->ports()[from_port_index];
                node_t* to_node = from_port.m_connection;
                uint8_t to_port_index = from_port.m_connection_port_index;
                assert(to_node);

                auto from_it = node_to_index.find(from_node);
                assert(from_it != node_to_index.end());
                auto to_it = node_to_index.find(to_node);
                assert(to_it != node_to_index.end());

                out->binary.push_back(OP_CONNECT_PORTS);
                out->binary.push_back(from_it->second);
                out->binary.push_back(from_port_index);
                out->binary.push_back(to_it->second);
                out->binary.push_back(to_port_index);
            }
        }
    }

    return true;
}

void init() {
    register_builtin<if_node_t>(node_id_t{"if"});
    register_builtin<sub_t>(node_id_t{"sub"});
    register_builtin<mul_node_t>(node_id_t{"mul"});
    register_builtin<is_zero_t>(node_id_t{"is_zero"});
    register_builtin<int_node_t>(node_id_t{"int"});
    register_builtin<logger_node_t>(node_id_t{"logger"});
    register_builtin<pin_node_t>(node_id_t{"pin"});

    register_coercion<node_binary_t, node_t>(&node_binary_to_node);
    register_coercion<node_binary_t, node_assembly_t>(&node_binary_to_node_assembly);
    register_coercion<node_t, node_binary_t>(&node_to_node_binary);
}

bool persist_save(node_t* node) {
    std::ofstream bin_file(to_bin_file_path(node->id().name), std::ios::binary);
    std::ofstream human_readable_file(to_human_readable_file_path(node->id().name));

    node_binary_t node_binary;
    if (!coerce(&node, &node_binary)) {
        return false;
    }

    bin_file.write((const char*) node_binary.binary.data(), (std::streamsize) node_binary.binary.size());

    node_assembly_t node_assembly;
    if (!coerce(&node_binary, &node_assembly)) {
        return false;
    }
    human_readable_file.write(node_assembly.assembly.c_str(), (std::streamsize) node_assembly.assembly.size());

    for (node_t* child : node->inner_nodes()) {
        persist_save(child);
    }

    return true;
}

node_t* persist_load(const node_id_t& id) {
    const std::string bin_path = to_bin_file_path(id.name);
    std::ifstream bin_file(bin_path);
    if (!bin_file.is_open()) {
        return nullptr;
        // throw std::runtime_error("failed to open binary file ' + bin_path + ');
    }

    node_binary_t node_binary;

    bin_file.seekg(0, std::ios::end);
    size_t size = (size_t) bin_file.tellg();
    bin_file.seekg(0, std::ios::beg);
    node_binary.binary.resize(size);
    bin_file.read((char*) node_binary.binary.data(), (std::streamsize) size);

    node_t* node = new node_t;
    if (!coerce(&node_binary, &node)) {
        delete node;
        return nullptr;
    }

    return node;
}

}
