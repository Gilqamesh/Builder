#ifndef FUNCTION_H
# define FUNCTION_H

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include <function_ir.h>
#include <typesystem.h>

class function_t {
public:
    struct argument_t {
        argument_t();

        function_t* m_connection;
        uint8_t m_connection_argument_index;
        std::string m_name;
        int m_data_type_id;
        std::vector<uint8_t> m_data;
    };

    struct reader_t {
        function_t* self;
        uint8_t index;

        template <typename T>
        operator T() {
            return self->read<T>(index);
        }
    };

    using function_call_t = void (*)(function_t&, uint8_t);

public:
    function_t(typesystem_t& typesystem, function_ir_t function_ir, function_call_t function_call);

    virtual ~function_t();

    function_t* parent();
    void parent(function_t* parent);

    function_ir_t& function_ir();
    function_call_t& function_call();

    void argument_name(uint8_t argument_index, std::string name);
    const std::string& argument_name(uint8_t argument_index);

    /**
     * Connects on `self_argument_index` to `other` on `other_argument_index`.
     * If there was data on `self_argument_index`, it copies it to `other` and calls it.
    */
    void connect(function_t* other, uint8_t other_argument_index, uint8_t self_argument_index);

    /**
     * Returns true if `argument_index` is connected to another node.
    */
    bool is_connected(uint8_t argument_index);

    /**
     * Returns the connected node on `argument_index`, or `nullptr` if not connected.
    */
    function_t* connection(uint8_t argument_index);

    /**
     * Disconnects the connected node on `argument_index` if any.
    */
    void disconnect(uint8_t argument_index);

    /**
     * Expands into the defined (by `name`) combination of nodes.
     * `caller_argument_index` is the argument index on which this node was called.
    */
    void call(uint8_t caller_argument_index);

    /**
     * Returns coerced data of type `T` from the `index` argument.
     * T is deduced from the call site.
    */
    reader_t read(uint8_t index);

    /**
     * Writes data of type `T` to the `argument_index`.
     * If the argument is connected, it then copies the data to the connection and calls it.
    */
    template <typename T>
    void write(uint8_t argument_index, T data);

    void write(uint8_t argument_index, void* data, int data_type_id);

    /**
     * If connected, copies data on `argument_index` to the connected argument and calls it.
    */
    void send(uint8_t argument_index);

    /**
     * Clears data on `argument_index`.
     * If connected, it also clears the connected argument.
    */
    void clear(uint8_t argument_index);

    /**
     * Calls `write` operation on `to_argument_index` with the data from `from_argument_index`.
     * Implies that if `to_argument_index` is connected, it will also copies the data to the connection and call it.
    */
    void copy(uint8_t from_argument_index, uint8_t to_argument_index);

    std::vector<function_t*>& children();

    std::vector<argument_t>& arguments();

    void morph(typesystem_t& typesystem, function_ir_t function_ir, function_call_t call);

    void expand();
    void shrink();

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
    template <typename T>
    T read(uint8_t argument_index);

private:
    typesystem_t& m_typesystem;
    function_ir_t m_function_ir;
    function_call_t m_call;
    std::vector<function_t*> m_children;
    function_t* m_parent;

    bool m_is_expanded;

    int m_left;
    int m_right;
    int m_top;
    int m_bottom;

    float m_coordinate_system_width;
    float m_coordinate_system_height;

    bool m_is_dimensions_finalized;

    std::vector<argument_t> m_arguments;
};

template <typename T>
T function_t::read(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }
    argument_t& argument = m_arguments[argument_index];
    if (argument.m_data_type_id == -1) {
        throw std::runtime_error(std::format("argument {} has no data", argument_index));
    }
    return m_typesystem.coerce<T>((void*) argument.m_data.data(), argument.m_data_type_id);
}

template <typename T>
void function_t::write(uint8_t argument_index, T data) {
    m_typesystem.register_type<T>();

    write(argument_index, (void*) &data, m_typesystem.type_id<T>());
}

#endif // FUNCTION_H
