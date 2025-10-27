#include "builtins.h"

void if_node_t::call(port_index_t caller_port_index) {
    (void) caller_port_index;

    bool condition;
    if (!read(PORT_0, condition)) {
        return ;
    }

    if (condition) {
        copy(PORT_1, PORT_3);
    } else {
        copy(PORT_2, PORT_4);
    }
}

void sub_t::call(port_index_t caller_port_index) {
    (void) caller_port_index;

    int a;
    if (!read(PORT_0, a)) {
        return ;
    }

    int b;
    if (!read(PORT_1, b)) {
        return ;
    }

    write(PORT_2, a - b);
}

void mul_node_t::call(port_index_t caller_port_index) {
    (void) caller_port_index;
    
    int a;
    if (!read(PORT_0, a)) {
        return ;
    }

    int b;
    if (!read(PORT_1, b)) {
        return ;
    }

    write(PORT_2, a * b);
}

void is_zero_t::call(port_index_t caller_port_index) {
    (void) caller_port_index;

    int in;
    if (!read(PORT_0, in)) {
        return ;
    }

    write(PORT_1, in == 0);
}

int_node_t::int_node_t(int val):
    m_val(val)
{
    write(PORT_0, m_val);
}

void int_node_t::call(port_index_t caller_port_index) {
    (void) caller_port_index;

    write(PORT_0, m_val);
}

void logger_node_t::call(port_index_t caller_port_index) {
    (void) caller_port_index;

    int in;
    if (read(PORT_0, in)) {
        std::ostream* os;
        if (read(PORT_1, os)) {
            *os << in << "\n";
        } else {
            std::cout << in << "\n";
        }
    }
}

void pin_node_t::call(port_index_t caller_port_index) {
    for (int i = 0; i < (int) __PORT_SIZE; ++i) {
        const port_index_t to_port_index = (port_index_t) i;
        if (to_port_index != caller_port_index && is_connected(to_port_index)) {
            copy(caller_port_index, to_port_index);
        }
    }
}
