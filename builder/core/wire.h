#ifndef WIRE_H
# define WIRE_H

# include "typesystem.h"

struct component_t;

// bidirectional wire connecting two different components
struct wire_t {
public:
    wire_t();

    void connect(component_t* component);

    void disconnect(component_t* component);

    component_t* other(component_t* self);

    template <typename T>
    void write(T data);

    void write(wire_t& other);
    void write(wire_t* other);

    template <typename T>
    void write(T data, component_t* writer);

    void write(wire_t& other, component_t* writer);
    void write(wire_t* other, component_t* writer);

    template <typename T>
    bool read(T& out);

    component_t* m_a;
    component_t* m_b;
    int m_data_type_id;
    std::vector<unsigned char> m_data;
};

struct pin_component_t;

struct component_t {
public:
    component_t();

    component_t(size_t exposed_pins);

    void set_name(std::string name);

    /**
     * Connect a wire to an exposed pin at the given index.
    */
    virtual void connect(size_t index, wire_t* wire);

    /**
     * Disconnect a wire from an exposed pin at the given index.
    */
    virtual void disconnect(size_t index, wire_t* wire);

    /**
     * Expose a pin at the given index to allow connections.
    */
    void expose(size_t index);

    /**
     * Get the wire connected to the exposed pin at the given index.
    */
    wire_t* at(size_t index);

    virtual std::vector<wire_t*> incoming_wires(size_t index);

    virtual void call(wire_t* caller) {
        (void) caller;
    }

    /**
     * Read data from the wire connected to the exposed pin at the given index.
    */
    template <typename T>
    bool read(size_t index, T& result) {
        if (
            m_wires.size() <= index ||
            m_wires[index] == nullptr
        ) {
            return false;
        }

        return m_wires[index]->read<T>(result);
    }

    /**
     * Write data to the wire connected to the exposed pin at the given index.
    */
    template <typename T>
    void write(size_t index, T data) {
        if (
            m_wires.size() <= index ||
            m_wires[index] == nullptr
        ) {
            return ;
        }

        m_wires[index]->write<T>(std::move(data), this);
    }

    /**
     * Copy data from the wire connected to the exposed pin at from_index to the wire connected to the exposed pin at to_index.
    */
    void mov(size_t from_index, size_t to_index);

    std::string m_name;

    // Todo: make structure { wire_t*, pin_component_t* }
    std::vector<wire_t*> m_wires;
    std::vector<pin_component_t*> m_exposed_pins;
};

template <typename T>
void wire_t::write(T data) {
    write<T>(std::move(data), nullptr);
}

template <typename T>
void wire_t::write(T data, component_t* writer) {
    TYPESYSTEM.register_type<T>();
    const int type_id = TYPESYSTEM.type_id<T>();
    const size_t type_size = TYPESYSTEM.sizeof_type(type_id);
    m_data_type_id = type_id;
    m_data.resize(type_size);
    *reinterpret_cast<T*>(m_data.data()) = std::move(data);

    if (m_a && m_a != writer) {
        m_a->call(this);
    }

    if (m_b && m_b != writer) {
        m_b->call(this);
    }
}

template <typename T>
bool wire_t::read(T& out) {
    if (m_data_type_id == -1) {
        return false;
    }

    return TYPESYSTEM.coerce<T>(m_data.data(), m_data_type_id, &out);
}

#endif // WIRE_H
