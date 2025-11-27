#include "function.h"

function_t::function_t(typesystem_t* typesystem, function_ir_t* function_ir, function_call_t call):
    m_typesystem(typesystem),
    m_function_ir(function_ir),
    m_call(call),
    m_parent(nullptr),
    m_is_expanded(false),
    m_left(0),
    m_right(0),
    m_top(0),
    m_bottom(0),
    m_coordinate_system_width(0.0f),
    m_coordinate_system_height(0.0f),
    m_is_dimensions_finalized(false)
{
}

function_t::~function_t() {
    for (int i = 0; i < __ARGUMENT_SIZE; ++i) {
        disconnect((argument_index_t) i);
    }
}

function_t* function_t::parent() {
    return m_parent;
}

void function_t::parent(function_t* parent) {
    m_parent = parent;
}

void function_t::argument_name(argument_index_t argument_index, std::string name) {
    assert(argument_index < __ARGUMENT_SIZE);

    m_arguments[argument_index].m_name = std::move(name);
}

const std::string& function_t::argument_name(argument_index_t argument_index) {
    assert(argument_index < __ARGUMENT_SIZE);

    return m_arguments[argument_index].m_name;
}

void function_t::connect(function_t* other, argument_index_t other_argument_index, argument_index_t self_argument_index) {
    assert(self_argument_index < __ARGUMENT_SIZE);
    assert(other_argument_index < __ARGUMENT_SIZE);
    assert(other);

    argument_t& self_argument = m_arguments[self_argument_index];
    self_argument.m_connection = other;
    self_argument.m_connection_argument_index = other_argument_index;

    send(self_argument_index);
}

bool function_t::is_connected(argument_index_t argument_index) {
    assert(argument_index < __ARGUMENT_SIZE);

    return m_arguments[argument_index].m_connection != nullptr;
}

function_t* function_t::connection(argument_index_t argument_index) {
    assert(argument_index < __ARGUMENT_SIZE);

    return m_arguments[argument_index].m_connection;
}

void function_t::disconnect(argument_index_t argument_index) {
    assert(argument_index < __ARGUMENT_SIZE);

    argument_t& self_argument = m_arguments[argument_index];
    self_argument.m_connection = nullptr;
    self_argument.m_connection_argument_index = __ARGUMENT_SIZE;
}

void function_t::call(argument_index_t caller_argument_index) {
    assert(caller_argument_index < __ARGUMENT_SIZE);
    
    assert(m_arguments[caller_argument_index].m_data_type_id != -1);

    expand();
    send(caller_argument_index);
}

void function_t::write(argument_index_t argument_index, void* data, int data_type_id) {
    assert(argument_index < __ARGUMENT_SIZE);

    if (data_type_id == -1) {
        return ;
    }

    argument_t& argument = m_arguments[argument_index];

    if (argument.m_data_type_id != data_type_id) {
        argument.m_data_type_id = data_type_id;
        argument.m_data.resize(m_typesystem->sizeof_type(data_type_id));
    }

    std::memcpy((void*) argument.m_data.data(), data, argument.m_data.size());

    send(argument_index);
}

void function_t::send(argument_index_t argument_index) {
    assert(argument_index < __ARGUMENT_SIZE);

    argument_t& argument = m_arguments[argument_index];
    if (argument.m_data_type_id != -1 && argument.m_connection) {
        argument_t& other_argument = argument.m_connection->m_arguments[argument.m_connection_argument_index];
        if (other_argument.m_data_type_id != argument.m_data_type_id) {
            other_argument.m_data_type_id = argument.m_data_type_id;
            other_argument.m_data.resize(argument.m_data.size());
        }
        assert(other_argument.m_data.size() == argument.m_data.size());
        std::memcpy((void*) other_argument.m_data.data(), (void*) argument.m_data.data(), other_argument.m_data.size());
        argument.m_connection->call(argument.m_connection_argument_index);
    }
}

void function_t::clear(argument_index_t argument_index) {
    assert(argument_index < __ARGUMENT_SIZE);

    argument_t& argument = m_arguments[argument_index];
    argument.m_data_type_id = -1;
    argument.m_data.clear();

    if (argument.m_connection) {
        argument_t& other_argument = argument.m_connection->m_arguments[argument.m_connection_argument_index];
        other_argument.m_data_type_id = -1;
        other_argument.m_data.clear();
    }
}

void function_t::copy(argument_index_t from_argument_index, argument_index_t to_argument_index) {
    assert(from_argument_index < __ARGUMENT_SIZE);
    assert(to_argument_index < __ARGUMENT_SIZE);

    argument_t& from_argument = m_arguments[from_argument_index];
    if (from_argument.m_data_type_id == -1) {
        return ;
    }

    write(to_argument_index, (void*) from_argument.m_data.data(), from_argument.m_data_type_id);
}

int function_t::left() {
    return m_left;
}

int function_t::right() {
    return m_right;
}

int function_t::top() {
    return m_top;
}

int function_t::bottom() {
    return m_bottom;
}

std::vector<function_t*>& function_t::children() {
    return m_children;
}

std::array<function_t::argument_t, function_t::__ARGUMENT_SIZE> function_t::arguments() {
    return m_arguments;
}

void function_t::expand(const node_binary_t& binary) {
    // read id of binary
    // determine if we are the same id
    // then check if we are already expanded

    size_t ip = 0;

    {
        std::string name;

        while (ip < binary.bytes.size() && binary.bytes[ip] != 0) {
            name.push_back(binary.bytes[ip]);
            ++ip;
        }
        if (binary.bytes.size() == ip) {
            throw std::runtime_error(std::format("unterminated component name '{}' in binary", name));
        }
        ++ip;

        if (binary.bytes.size() < ip + 8) {
            throw std::runtime_error("expected 8 bytes for creation time");
        }
        uint64_t creation_time_serialized = binary.bytes[ip++] << 56;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++] << 48;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++] << 40;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++] << 32;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++] << 24;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++] << 16;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++] << 8;
        creation_time_serialized |= (uint64_t) binary.bytes[ip++];

        node_id_t node_id = {
            .name = std::move(name),
            .creation_time = node_binary_t::deserialize_creation_time(creation_time_serialized)
        };

        if (id() == node_id) {
            if (m_is_expanded) { 
                return ;
            }
        } else {
            id(std::move(node_id));
            if (m_is_expanded) {
                // todo: collapse
            }
        }
    }

    // todo: cleanup on failure
    while (ip < binary.bytes.size()) {
        const node_binary_t::opcode_t opcode = static_cast<node_binary_t::opcode_t>(binary.bytes[ip++]);
        switch (opcode) {
            case node_binary_t::opcode_t::CREATE_NODE: {
                node_id_t node_id;
                while (ip < binary.bytes.size() && binary.bytes[ip] != 0) {
                    node_id.name.push_back(binary.bytes[ip]);
                    ++ip;
                }
                if (ip == binary.bytes.size()) {
                    throw std::runtime_error("unterminated node name in binary");
                }
                assert(binary.bytes[ip] == 0);
                ++ip;

                if (binary.bytes.size() < ip + 8) {
                    throw std::runtime_error("expected 8 bytes for node dimensions");
                }

                int left = (int16_t) binary.bytes[ip++] << 8;
                left |= (int16_t) binary.bytes[ip++];
                int right = (int16_t) binary.bytes[ip++] << 8;
                right |= (int16_t) binary.bytes[ip++];
                int top = (int16_t) binary.bytes[ip++] << 8;
                top |= (int16_t) binary.bytes[ip++];
                int bottom = (int16_t) binary.bytes[ip++] << 8;
                bottom |= (int16_t) binary.bytes[ip++];

                function_t* node = m_typesystem->coerce(node_id);
                assert(node);
                node->left(left);
                node->right(right);
                node->top(top);
                node->bottom(bottom);
                node->finalize_dimensions();
                m_children.emplace_back(node);
            } break ;
            case node_binary_t::opcode_t::CONNECT_argumentS: {
                if (binary.bytes.size() < ip + 4) {
                    throw std::runtime_error("expected 4 bytes for connect arguments");
                }
                argument_index_t from_node_index = (argument_index_t) binary.bytes[ip++];
                argument_index_t from_argument_index = (argument_index_t) binary.bytes[ip++];
                argument_index_t to_node_index = (argument_index_t) binary.bytes[ip++];
                argument_index_t to_argument_index = (argument_index_t) binary.bytes[ip++];

                if (__ARGUMENT_SIZE < from_argument_index) {
                    throw std::runtime_error("from argument index out of range: " + std::to_string(from_argument_index));
                }

                if (__ARGUMENT_SIZE < to_argument_index) {
                    throw std::runtime_error("to argument index out of range: " + std::to_string(to_argument_index));
                }

                function_t* from_node = nullptr;
                if (from_node_index == (uint8_t) -1) {
                    from_node = this;
                } else if (from_node_index < m_children.size()) {
                    from_node = m_children[from_node_index];
                } else {
                    throw std::runtime_error("from node index out of range: " + std::to_string(from_node_index));
                }

                function_t* to_node = nullptr;
                if (to_node_index == (uint8_t) -1) {
                    to_node = this;
                } else if (to_node_index < m_children.size()) {
                    to_node = m_children[to_node_index];
                } else {
                    throw std::runtime_error("to node index out of range: " + std::to_string(to_node_index));
                }

                assert(from_node && to_node);
                from_node->connect(to_node, to_argument_index, from_argument_index);
            } break ;
            default: {
                throw std::runtime_error("unknown opcode: " + std::to_string(op));
            } break ;
        }
    }
    m_is_expanded = true;
}

void function_t::morph(typesystem_t* typesystem, function_ir_t* function_ir, function_call_t call) {
}

void function_t::expand() {
    if (m_is_expanded) {
        return ;
    }
    assert(m_function_ir);
    assert(m_children.empty());

    for (const auto& child : m_function_ir->children) {
        function_t* node = m_typesystem->coerce(child.id);
        assert(node);
        node->left(child.left);
        node->right(child.right);
        node->top(child.top);
        node->bottom(child.bottom);
        node->finalize_dimensions();
        m_children.emplace_back(node);
    }

    for (const auto& connection : m_function_ir->connections) {
        function_t* from_function = nullptr;
        if (connection.from_function_index == static_cast<uint16_t>(-1)) {
            from_function = this;
        } else if (connection.from_function_index < m_children.size()) {
            from_function = m_children[connection.from_function_index];
        } else {
            throw std::runtime_error(std::format("from function index out of range: {}", connection.from_function_index));
        }

        function_t* to_function = nullptr;
        if (connection.to_function_index == static_cast<uint16_t>(-1)) {
            to_function = this;
        } else if (connection.to_function_index < m_children.size()) {
            to_function = m_children[connection.to_function_index];
        } else {
            throw std::runtime_error(std::format("to function index out of range: {}", connection.to_function_index));
        }

        assert(from_function && to_function);
        from_function->connect(to_function, connection.to_argument_index, connection.from_argument_index);
    }

    m_is_expanded = true;
}

void function_t::left(int left) {
    m_left = left;
}

void function_t::right(int right) {
    m_right = right;
}

void function_t::top(int top) {
    m_top = top;
}

void function_t::bottom(int bottom) {
    m_bottom = bottom;
}

void function_t::finalize_dimensions() {
    const int width = m_right - m_left;
    const int height = m_bottom - m_top;

    assert(0 < width);
    assert(0 < height);

    if (width < height) {
        m_coordinate_system_width = width / (float) height * std::numeric_limits<int16_t>::max();
        m_coordinate_system_height = std::numeric_limits<int16_t>::max();
    } else {
        m_coordinate_system_width = std::numeric_limits<int16_t>::max();
        m_coordinate_system_height = height / (float) width * std::numeric_limits<int16_t>::max();
    }

    m_is_dimensions_finalized = true;
}

float function_t::coordinate_system_width() {
    assert(m_is_dimensions_finalized);
    return m_coordinate_system_width;
}

float function_t::coordinate_system_height() {
    assert(m_is_dimensions_finalized);
    return m_coordinate_system_height;
}

int function_t::to_child_x(int x) {
    assert(m_left < m_right);
    assert(m_is_dimensions_finalized);
    return (x - m_left) / (float) (m_right - m_left) * m_coordinate_system_width - m_coordinate_system_width / 2.0f;
}

int function_t::to_child_y(int y) {
    assert(m_top < m_bottom);
    assert(m_is_dimensions_finalized);
    return (y - m_top) / (float) (m_bottom - m_top) * m_coordinate_system_height - m_coordinate_system_height / 2.0f;
}

int function_t::from_child_x(int x) {
    assert(m_left < m_right);
    assert(m_is_dimensions_finalized);
    return (x + m_coordinate_system_width / 2.0f) / m_coordinate_system_width * (m_right - m_left) + m_left;
}

int function_t::from_child_y(int y) {
    assert(m_top < m_bottom);
    assert(m_is_dimensions_finalized);
    return (y + m_coordinate_system_height / 2.0f) / m_coordinate_system_height * (m_bottom - m_top) + m_top;
}

function_t::argument_t::argument_t() {
    m_connection = nullptr;
    m_connection_argument_index = __ARGUMENT_SIZE;
    m_data_type_id = -1;
}

static function_t* node_binary_to_node(const node_binary_t& in) {
    return new function_t(in);
}

node_binary_t function_to_node_binary(function_t* in) {
    if (

    node_binary_t result;

    for (char c : in->bytes().name) {
        result.bytes.push_back((uint8_t) c);
    }
    result.bytes.push_back(0);

    std::unordered_set<function_t*> unvisited_nodes;

    std::unordered_map<function_t*, uint8_t> function_to_index;

    function_to_index[in] = (uint8_t) -1;
    unvisited_nodes.insert(in);
    std::unordered_set<function_t*> visited_nodes;
    while (!unvisited_nodes.empty()) {
        function_t* unvisited_node = *unvisited_nodes.begin();
        unvisited_nodes.erase(unvisited_node);
        visited_nodes.insert(unvisited_node);

        if (unvisited_node != in) {
            result.bytes.push_back(OP_CREATE_NODE);
            for (char c : unvisited_node->id().name) {
                result.bytes.push_back((uint8_t) c);
            }
            result.bytes.push_back(0);
            int16_t left = (int16_t) unvisited_node->left();
            result.bytes.push_back((uint8_t) (left >> 8));
            result.bytes.push_back((uint8_t) (left & 0xFF));
            int16_t right = (int16_t) unvisited_node->right();
            result.bytes.push_back((uint8_t) (right >> 8));
            result.bytes.push_back((uint8_t) (right & 0xFF));
            int16_t top = (int16_t) unvisited_node->top();
            result.bytes.push_back((uint8_t) (top >> 8));
            result.bytes.push_back((uint8_t) (top & 0xFF));
            int16_t bottom = (int16_t) unvisited_node->bottom();
            result.bytes.push_back((uint8_t) (bottom >> 8));
            result.bytes.push_back((uint8_t) (bottom & 0xFF));
            assert(function_to_index.size() < UINT8_MAX + 1);
            uint8_t node_index = (uint8_t) function_to_index.size() - 1;
            function_to_index[unvisited_node] = node_index;
        }

        for (int i = 0; i < __ARGUMENT_SIZE; ++i) {
            function_t* connection = unvisited_node->connection((argument_index_t) i);
            if (connection && visited_nodes.find(connection) == visited_nodes.end()) {
                unvisited_nodes.insert(connection);
            }
        }
    }

    while (!visited_nodes.empty()) {
        function_t* from_node = *visited_nodes.begin();
        visited_nodes.erase(from_node);

        for (uint8_t from_argument_index = 0; from_argument_index < __ARGUMENT_SIZE; ++from_argument_index) {
            if (from_node->is_connected((argument_index_t) from_argument_index)) {
                function_t::argument_t& from_argument = from_node->arguments()[from_argument_index];
                function_t* to_node = from_argument.m_connection;
                uint8_t to_argument_index = from_argument.m_connection_argument_index;
                assert(to_node);

                auto from_it = function_to_index.find(from_node);
                assert(from_it != function_to_index.end());
                auto to_it = function_to_index.find(to_node);
                assert(to_it != function_to_index.end());

                result.bytes.push_back(OP_CONNECT_argumentS);
                result.bytes.push_back(from_it->second);
                result.bytes.push_back(from_argument_index);
                result.bytes.push_back(to_it->second);
                result.bytes.push_back(to_argument_index);
            }
        }
    }

    return result;
}

function_t* string_to_node(const std::string& str) {
    static std::unordered_map<std::string, function_t*(*)()> primitive_constructors = {
        {
            "if",
            +[]() {
                return new function_t(
                    +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                        (void) caller_argument_index;

                        bool condition;
                        if (!self->read(argument_0, condition)) {
                            return ;
                        }

                        if (condition) {
                            self->copy(argument_1, argument_3);
                        } else {
                            self->copy(argument_2, argument_3);
                        }
                    }
                );
            }
        },
        {
            "sub",
            +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                (void) caller_argument_index;

                int a;
                if (!self->read(argument_0, a)) {
                    return ;
                }

                int b;
                if (!self->read(argument_1, b)) {
                    return ;
                }

                self->write(argument_2, a - b);
            }
        },
        {
            "mul",
            +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                (void) caller_argument_index;
                
                int a;
                if (!self->read(argument_0, a)) {
                    return ;
                }

                int b;
                if (!self->read(argument_1, b)) {
                    return ;
                }

                self->write(argument_2, a * b);
            }
        },
        {
            "is_zero",
            +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                (void) caller_argument_index;

                int in;
                if (!self->read(argument_0, in)) {
                    return ;
                }

                self->write(argument_1, in == 0);
            }
        },
        {
            "int",
            +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                self->write(argument_0, 0);
            }
        },
        {
            "logger",
            +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                (void) caller_argument_index;

                int in;
                if (self->read(argument_0, in)) {
                    std::ostream* os;
                    if (self->read(argument_1, os)) {
                        *os << in << "\n";
                    } else {
                        std::cout << in << "\n";
                    }
                }
            }
        },
        {
            "pin",
            +[](function_t* self, function_t::argument_index_t caller_argument_index) {
                for (int i = 0; i < (int) __ARGUMENT_SIZE; ++i) {
                    const function_t::argument_index_t to_argument_index = (argument_index_t) i;
                    if (to_argument_index != caller_argument_index && self->is_connected(to_argument_index)) {
                        self->copy(caller_argument_index, to_argument_index);
                    }
                }
            }
        }
    };
    auto it = primitive_constructors.find(str);
    if (it != primitive_constructors.end()) {
        return it->second();
    }

    std::ifstream ifs(to_bin_file_path(str), std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error(std::format("failed to open binary file '{}'", to_bin_file_path(str)));
    }

    node_binary_t node_binary;
    ifs.seekg(0, std::ios::end);
    size_t size = (size_t) ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    node_binary.bytes.resize(size);
    ifs.read((char*) node_binary.bytes.data(), size);

    return coerce(node_binary);
}
