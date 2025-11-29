#include <function.h>
#include <cassert>
#include <cstring>
#include <limits>
#include <utility>

function_t::function_t(typesystem_t& typesystem, function_ir_t function_ir, function_call_t call):
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
    for (size_t i = 0; i < m_arguments.size(); ++i) {
        disconnect((uint8_t) i);
    }
}

function_t* function_t::parent() {
    return m_parent;
}

void function_t::parent(function_t* parent) {
    m_parent = parent;
}

function_ir_t& function_t::function_ir() {
    return m_function_ir;
}

function_t::function_call_t& function_t::function_call() {
    return m_call;
}

void function_t::argument_name(uint8_t argument_index, std::string name) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }

    m_arguments[argument_index].m_name = std::move(name);
}

const std::string& function_t::argument_name(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }

    return m_arguments[argument_index].m_name;
}

void function_t::connect(function_t* other, uint8_t other_argument_index, uint8_t self_argument_index) {
    assert(other);
    if (m_arguments.size() <= self_argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", self_argument_index));
    }
    if (other->m_arguments.size() <= other_argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", other_argument_index));
    }

    argument_t& self_argument = m_arguments[self_argument_index];
    self_argument.m_connection = other;
    self_argument.m_connection_argument_index = other_argument_index;

    send(self_argument_index);
}

bool function_t::is_connected(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }
    return m_arguments[argument_index].m_connection != nullptr;
}

function_t* function_t::connection(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }
    return m_arguments[argument_index].m_connection;
}

void function_t::disconnect(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }

    argument_t& self_argument = m_arguments[argument_index];
    self_argument.m_connection = nullptr;
    self_argument.m_connection_argument_index = -1;
}

void function_t::call(uint8_t caller_argument_index) {
    m_call(*this, caller_argument_index);
}

function_t::reader_t function_t::read(uint8_t index) {
    return reader_t {
        .self = this,
        .index = index
    };
}

void function_t::write(uint8_t argument_index, void* data, int data_type_id) {
    if (data_type_id == -1) {
        return ;
    }

    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }
    argument_t& argument = m_arguments[argument_index];

    if (argument.m_data_type_id != data_type_id) {
        argument.m_data_type_id = data_type_id;
        argument.m_data.resize(m_typesystem.sizeof_type(data_type_id));
    }

    std::memcpy((void*) argument.m_data.data(), data, argument.m_data.size());

    send(argument_index);
}

void function_t::send(uint8_t argument_index) {
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

void function_t::clear(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }

    argument_t& argument = m_arguments[argument_index];
    argument.m_data_type_id = -1;
    argument.m_data.clear();

    if (argument.m_connection) {
        argument_t& other_argument = argument.m_connection->m_arguments[argument.m_connection_argument_index];
        other_argument.m_data_type_id = -1;
        other_argument.m_data.clear();
    }
}

void function_t::copy(uint8_t from_argument_index, uint8_t to_argument_index) {
    if (m_arguments.size() <= from_argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", from_argument_index));
    }
    if (m_arguments.size() <= to_argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", to_argument_index));
    }

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

std::vector<function_t::argument_t>& function_t::arguments() {
    return m_arguments;
}

void function_t::morph(typesystem_t& typesystem, function_ir_t function_ir, function_call_t call) {
    if (m_is_expanded) {
        if (m_function_ir.function_id != function_ir.function_id) {
            shrink();
        }
    }

    m_typesystem = typesystem;
    m_function_ir = std::move(function_ir);
    m_call = call;

    expand();
}

void function_t::expand() {
    if (m_is_expanded) {
        return ;
    }
    assert(m_children.empty());

    for (const auto& child : m_function_ir.children) {
        function_t* node = m_typesystem.coerce(child.function_id);
        assert(node);
        node->left(child.left);
        node->right(child.right);
        node->top(child.top);
        node->bottom(child.bottom);
        node->finalize_dimensions();
        m_children.emplace_back(node);
    }

    for (const auto& connection : m_function_ir.connections) {
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

void function_t::shrink() {
    if (!m_is_expanded) {
        return ;
    }

    throw std::runtime_error("shrink() is not implemented yet");

    m_is_expanded = false;
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
    m_connection_argument_index = -1;
    m_data_type_id = -1;
}

