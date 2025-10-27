#ifndef NODE_H
# define NODE_H

# include "typesystem.h"

enum port_index_t {
    PORT_0,  PORT_1,  PORT_2,  PORT_3,
    PORT_4,  PORT_5,  PORT_6,  PORT_7,
    PORT_8,  PORT_9,  PORT_10, PORT_11,
    PORT_12, PORT_13, PORT_14, PORT_15,

    __PORT_SIZE
};

struct node_id_t {
    std::string name;
};

struct node_binary_t {
    std::vector<uint8_t> binary;
};

struct node_assembly_t {
    std::string assembly;
};

/**
 * Acts as an abstraction for a combination of nodes.
 * Override `call` to implement a primitive node.
*/
class node_t {
public:
    struct port_t {
        port_t();

        node_t* m_connection;
        port_index_t m_connection_port_index;
        std::string m_name;
        int m_data_type_id;
        std::vector<uint8_t> m_data;
    };

public:
    node_t();

    virtual ~node_t();

    void id(node_id_t id);

    const node_id_t& id();

    node_t* parent();
    void parent(node_t* parent);

    /**
     * Give a `name` to `port_index`.
    */
    void port_name(port_index_t port_index, std::string name);

    /**
     * Return `name` of `port_index`.
    */
    const std::string& port_name(port_index_t port_index);

    /**
     * Connects on `self_port_index` to `other` on `other_port_index`.
     * If there was data on `self_port_index`, it copies it to `other` and calls it.
    */
    void connect(node_t* other, port_index_t other_port_index, port_index_t self_port_index);

    /**
     * Returns true if `port_index` is connected to another node.
    */
    bool is_connected(port_index_t port_index);

    /**
     * Returns the connected node on `port_index`, or `nullptr` if not connected.
    */
    node_t* connection(port_index_t port_index);

    /**
     * Disconnects the connected node on `port_index` if any.
    */
    void disconnect(port_index_t port_index);

    /**
     * Expands into the defined (by `name`) combination of nodes.
     * `caller_port_index` is the port index on which this node was called.
    */
    virtual void call(port_index_t caller_port_index);

    /**
     * Reads data of type `T` from `port_index`.
     * Returns false if no data is present or if the data cannot be coerced to type `T`.
     * Returns true and sets `out` otherwise.
    */
    template <typename T>
    bool read(port_index_t port_index, T& out);

    /**
     * Writes data of type `T` to the `port_index`.
     * If the port is connected, it then copies the data to the connection and calls it.
    */
    template <typename T>
    void write(port_index_t port_index, T data);

    void write(port_index_t port_index, void* data, int data_type_id);

    /**
     * If connected, copies data on `port_index` to the connected port and calls it.
    */
    void send(port_index_t port_index);

    /**
     * Clears data on `port_index`.
     * If connected, it also clears the connected port.
    */
    void clear(port_index_t port_index);

    /**
     * Calls `write` operation on `to_port_index` with the data from `from_port_index`.
     * Implies that if `to_port_index` is connected, it will also copies the data to the connection and call it.
    */
    void copy(port_index_t from_port_index, port_index_t to_port_index);

    std::vector<node_t*>& inner_nodes();

    std::array<port_t, __PORT_SIZE> ports();

    int left();
    int right();
    int top();
    int bottom();

    void left(int left);
    void right(int right);
    void top(int top);
    void bottom(int bottom);

    void finalize_dimensions();

    float coordinate_system_width();
    float coordinate_system_height();

    /**
     * Converts `x` from this node's coordinate system to the child's coordinate system.
    */
    int to_child_x(int x);

    /**
     * Converts `y` from this node's coordinate system to the child's coordinate system.
    */
    int to_child_y(int y);

    /**
     * Converts `x` from the child's coordinate system to this node's coordinate system.
    */
    int from_child_x(int x);

    /**
     * Converts `y` from the child's coordinate system to this node's coordinate system.
    */
    int from_child_y(int y);

private:

    node_id_t m_id;

    std::vector<node_t*> m_inner_nodes;

    bool m_is_expanded;

    int m_left;
    int m_right;
    int m_top;
    int m_bottom;

    float m_coordinate_system_width;
    float m_coordinate_system_height;

    bool m_is_dimensions_finalized;

    node_t* m_parent;
    std::array<port_t, __PORT_SIZE> m_ports;
};

template <typename T>
bool node_t::read(port_index_t port_index, T& out) {
    assert(port_index < __PORT_SIZE);

    port_t& port = m_ports[port_index];
    if (port.m_data_type_id == -1) {
        return false;
    }
    return TYPESYSTEM.coerce<T>((void*) port.m_data.data(), port.m_data_type_id, &out);
}


template <typename T>
void node_t::write(port_index_t port_index, T data) {
    assert(port_index < __PORT_SIZE);

    TYPESYSTEM.register_type<T>();

    write(port_index, (void*) &data, TYPESYSTEM.type_id<T>());
}

#endif // NODE_H
