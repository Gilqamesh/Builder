#include "node.h"

node_t::node_t():
    m_is_expanded(false),
    m_left(0),
    m_right(0),
    m_top(0),
    m_bottom(0),
    m_coordinate_system_width(0.0f),
    m_coordinate_system_height(0.0f),
    m_is_dimensions_finalized(false),
    m_parent(nullptr)
{
}

node_t::~node_t() {
    for (int i = 0; i < __PORT_SIZE; ++i) {
        disconnect((port_index_t) i);
    }
}

void node_t::id(node_id_t id) {
    m_id = std::move(id);
}

const node_id_t& node_t::id() {
    return m_id;
}

node_t* node_t::parent() {
    return m_parent;
}

void node_t::parent(node_t* parent) {
    m_parent = parent;
}

void node_t::port_name(port_index_t port_index, std::string name) {
    assert(port_index < __PORT_SIZE);

    m_ports[port_index].m_name = std::move(name);
}

const std::string& node_t::port_name(port_index_t port_index) {
    assert(port_index < __PORT_SIZE);

    return m_ports[port_index].m_name;
}

void node_t::connect(node_t* other, port_index_t other_port_index, port_index_t self_port_index) {
    assert(self_port_index < __PORT_SIZE);
    assert(other_port_index < __PORT_SIZE);
    assert(other);

    port_t& self_port = m_ports[self_port_index];
    self_port.m_connection = other;
    self_port.m_connection_port_index = other_port_index;

    send(self_port_index);
}

bool node_t::is_connected(port_index_t port_index) {
    assert(port_index < __PORT_SIZE);

    return m_ports[port_index].m_connection != nullptr;
}

node_t* node_t::connection(port_index_t port_index) {
    assert(port_index < __PORT_SIZE);

    return m_ports[port_index].m_connection;
}

void node_t::disconnect(port_index_t port_index) {
    assert(port_index < __PORT_SIZE);

    port_t& self_port = m_ports[port_index];
    self_port.m_connection = nullptr;
    self_port.m_connection_port_index = __PORT_SIZE;
}

void node_t::call(port_index_t caller_port_index) {
    assert(caller_port_index < __PORT_SIZE);
    
    assert(m_ports[caller_port_index].m_data_type_id != -1);

    if (!m_is_expanded) {
        m_is_expanded = true;
        coerce();
        if (!COMPILER.expand_compound_node(id(), this)) {
            m_is_expanded = false;
        }
    }

    send(caller_port_index);
}

void node_t::write(port_index_t port_index, void* data, int data_type_id) {
    assert(port_index < __PORT_SIZE);

    if (data_type_id == -1) {
        return ;
    }

    port_t& port = m_ports[port_index];

    if (port.m_data_type_id != data_type_id) {
        port.m_data_type_id = data_type_id;
        port.m_data.resize(TYPESYSTEM.sizeof_type(data_type_id));
    }

    std::memcpy((void*) port.m_data.data(), data, port.m_data.size());

    send(port_index);
}

void node_t::send(port_index_t port_index) {
    assert(port_index < __PORT_SIZE);

    port_t& port = m_ports[port_index];
    if (port.m_data_type_id != -1 && port.m_connection) {
        port_t& other_port = port.m_connection->m_ports[port.m_connection_port_index];
        if (other_port.m_data_type_id != port.m_data_type_id) {
            other_port.m_data_type_id = port.m_data_type_id;
            other_port.m_data.resize(port.m_data.size());
        }
        assert(other_port.m_data.size() == port.m_data.size());
        std::memcpy((void*) other_port.m_data.data(), (void*) port.m_data.data(), other_port.m_data.size());
        port.m_connection->call(port.m_connection_port_index);
    }
}

void node_t::clear(port_index_t port_index) {
    assert(port_index < __PORT_SIZE);

    port_t& port = m_ports[port_index];
    port.m_data_type_id = -1;
    port.m_data.clear();

    if (port.m_connection) {
        port_t& other_port = port.m_connection->m_ports[port.m_connection_port_index];
        other_port.m_data_type_id = -1;
        other_port.m_data.clear();
    }
}

void node_t::copy(port_index_t from_port_index, port_index_t to_port_index) {
    assert(from_port_index < __PORT_SIZE);
    assert(to_port_index < __PORT_SIZE);

    port_t& from_port = m_ports[from_port_index];
    if (from_port.m_data_type_id == -1) {
        return ;
    }

    write(to_port_index, (void*) from_port.m_data.data(), from_port.m_data_type_id);
}

int node_t::left() {
    return m_left;
}

int node_t::right() {
    return m_right;
}

int node_t::top() {
    return m_top;
}

int node_t::bottom() {
    return m_bottom;
}

std::vector<node_t*>& node_t::inner_nodes() {
    return m_inner_nodes;
}

std::array<node_t::port_t, __PORT_SIZE> node_t::ports() {
    return m_ports;
}

void node_t::left(int left) {
    m_left = left;
}

void node_t::right(int right) {
    m_right = right;
}

void node_t::top(int top) {
    m_top = top;
}

void node_t::bottom(int bottom) {
    m_bottom = bottom;
}

void node_t::finalize_dimensions() {
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

float node_t::coordinate_system_width() {
    assert(m_is_dimensions_finalized);
    return m_coordinate_system_width;
}

float node_t::coordinate_system_height() {
    assert(m_is_dimensions_finalized);
    return m_coordinate_system_height;
}

int node_t::to_child_x(int x) {
    assert(m_left < m_right);
    assert(m_is_dimensions_finalized);
    return (x - m_left) / (float) (m_right - m_left) * m_coordinate_system_width - m_coordinate_system_width / 2.0f;
}

int node_t::to_child_y(int y) {
    assert(m_top < m_bottom);
    assert(m_is_dimensions_finalized);
    return (y - m_top) / (float) (m_bottom - m_top) * m_coordinate_system_height - m_coordinate_system_height / 2.0f;
}

int node_t::from_child_x(int x) {
    assert(m_left < m_right);
    assert(m_is_dimensions_finalized);
    return (x + m_coordinate_system_width / 2.0f) / m_coordinate_system_width * (m_right - m_left) + m_left;
}

int node_t::from_child_y(int y) {
    assert(m_top < m_bottom);
    assert(m_is_dimensions_finalized);
    return (y + m_coordinate_system_height / 2.0f) / m_coordinate_system_height * (m_bottom - m_top) + m_top;
}

node_t::port_t::port_t() {
    m_connection = nullptr;
    m_connection_port_index = __PORT_SIZE;
    m_data_type_id = -1;
}
