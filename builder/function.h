#ifndef FUNCTION_H
# define FUNCTION_H

# include "function_ir.h"
# include "typesystem.h"

class function_t {
public:
    enum argument_index_t {
        ARGUMENT_0,  ARGUMENT_1,  ARGUMENT_2,  ARGUMENT_3,
        ARGUMENT_4,  ARGUMENT_5,  ARGUMENT_6,  ARGUMENT_7,
        ARGUMENT_8,  ARGUMENT_9,  ARGUMENT_10, ARGUMENT_11,
        ARGUMENT_12, ARGUMENT_13, ARGUMENT_14, ARGUMENT_15,

        __ARGUMENT_SIZE
    };

    struct argument_t {
        argument_t();

        function_t* m_connection;
        argument_index_t m_connection_argument_index;
        std::string m_name;
        int m_data_type_id;
        std::vector<uint8_t> m_data;
    };

public:
    typedef void (*function_call_t)(function_t&, uint32_t);
    function_t(typesystem_t* typesystem, function_ir_t ir, function_call_t);

    virtual ~function_t();

    function_t* parent();
    void parent(function_t* parent);

    void argument_name(argument_index_t argument_index, std::string name);
    const std::string& argument_name(argument_index_t argument_index);

    /**
     * Connects on `self_argument_index` to `other` on `other_argument_index`.
     * If there was data on `self_argument_index`, it copies it to `other` and calls it.
    */
    void connect(function_t* other, argument_index_t other_argument_index, argument_index_t self_argument_index);

    /**
     * Returns true if `argument_index` is connected to another node.
    */
    bool is_connected(argument_index_t argument_index);

    /**
     * Returns the connected node on `argument_index`, or `nullptr` if not connected.
    */
    function_t* connection(argument_index_t argument_index);

    /**
     * Disconnects the connected node on `argument_index` if any.
    */
    void disconnect(argument_index_t argument_index);

    /**
     * Expands into the defined (by `name`) combination of nodes.
     * `caller_argument_index` is the argument index on which this node was called.
    */
    void call(argument_index_t caller_argument_index);

    /**
     * Reads data of type `T` from `argument_index`.
     * Returns false if no data is present or if the data cannot be coerced to type `T`.
     * Returns true and sets `out` otherwise.
    */
    template <typename T>
    bool read(argument_index_t argument_index, T& out);

    /**
     * Writes data of type `T` to the `argument_index`.
     * If the argument is connected, it then copies the data to the connection and calls it.
    */
    template <typename T>
    void write(argument_index_t argument_index, T data);

    void write(argument_index_t argument_index, void* data, int data_type_id);

    /**
     * If connected, copies data on `argument_index` to the connected argument and calls it.
    */
    void send(argument_index_t argument_index);

    /**
     * Clears data on `argument_index`.
     * If connected, it also clears the connected argument.
    */
    void clear(argument_index_t argument_index);

    /**
     * Calls `write` operation on `to_argument_index` with the data from `from_argument_index`.
     * Implies that if `to_argument_index` is connected, it will also copies the data to the connection and call it.
    */
    void copy(argument_index_t from_argument_index, argument_index_t to_argument_index);

    std::vector<function_t*>& children();

    std::array<argument_t, __ARGUMENT_SIZE> arguments();

    void expand();

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

    std::array<argument_t, __ARGUMENT_SIZE> m_arguments;
};

template <typename T>
bool function_t::read(argument_index_t argument_index, T& out) {
    assert(argument_index < __ARGUMENT_SIZE);

    argument_t& argument = m_arguments[argument_index];
    if (argument.m_data_type_id == -1) {
        return false;
    }
    return TYPESYSTEM.coerce<T>((void*) argument.m_data.data(), argument.m_data_type_id, &out);
}

template <typename T>
void function_t::write(argument_index_t argument_index, T data) {
    assert(argument_index < __ARGUMENT_SIZE);

    TYPESYSTEM.register_type<T>();

    write(argument_index, (void*) &data, TYPESYSTEM.type_id<T>());
}

struct B {
    template <typename RegistererType, typename Func>
    void register_primitive(RegistererType* registerer, Func f, void (*call)(function_t*, function_t::argument_index_t)) {
    }
};

struct A {
    void register_primitives(B* b) {
        b->register_primitive(this, f, +[this](function_t* self, function_t::argument_index_t index) {
            double in;
            if (read(index, in)) {
                self->write<int>(function_t::argument_index_t::ARGUMENT_1, f(in));
            }
        });
    }

    int f(double d) {
    }
};

#endif // FUNCTION_H
