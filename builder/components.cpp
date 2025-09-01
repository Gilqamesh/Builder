#include "components.h"
#include "core/compiler.h"

if_component_t::if_component_t(): component_t(5) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: condition <bool>
 * 1: consequence_in <any>
 * 2: alternative_in <any>
 * 3: consequence_out <any>
 * 4: alternative_out <any>
*/
void if_component_t::call(wire_t* caller) {
    (void) caller;

    bool condition;
    if (!read<bool>(0, condition)) {
        return ;
    }

    if (condition) {
        mov(1, 3);
    } else {
        mov(2, 4);
    }
}

mul_component_t::mul_component_t(): component_t(3) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: a <int>
 * 1: b <int>
 * 2: a * b <int>
*/
void mul_component_t::call(wire_t* caller) {
    (void) caller;

    int a;
    if (!read<int>(0, a)) {
        return ;
    }

    int b;
    if (!read<int>(1, b)) {
        return ;
    }

    write<int>(2, a * b);
}

is_zero_component_t::is_zero_component_t(): component_t(2) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}


/**
 * 0: in <int>
 * 1: out <bool> (in == 0)
*/
void is_zero_component_t::call(wire_t* caller) {
    (void) caller;

    int in;
    if (!read<int>(0, in)) {
        return ;
    }

    write<bool>(1, in == 0);
}

one_component_t::one_component_t(): component_t(1) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: out <int> (always 1)
*/
void one_component_t::call(wire_t* caller) {
    (void) caller;

    write<int>(0, 1);
}

sub_component_t::sub_component_t(): component_t(3) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: a <int>
 * 1: b <int>
 * 2: a - b <int>
*/
void sub_component_t::call(wire_t* caller) {
    (void) caller;

    int a;
    if (!read<int>(0, a)) {
        return ;
    }

    int b;
    if (!read<int>(1, b)) {
        return ;
    }

    write<int>(2, a - b);
}

add_component_t::add_component_t(): component_t(3) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: a <int>
 * 1: b <int>
 * 2: a + b <int>
*/
void add_component_t::call(wire_t* caller) {
    (void) caller;

    int a;
    if (!read<int>(0, a)) {
        return ;
    }

    int b;
    if (!read<int>(1, b)) {
        return ;
    }

    write<int>(2, a + b);
}

/**
 * 0: in <int>
 * 1: out <int>
 */
fact_component_t::fact_component_t(): component_t(2) {
    wire_t* a = new wire_t();
    wire_t* b = new wire_t();
    wire_t* c = new wire_t();
    wire_t* d = new wire_t();
    wire_t* e = new wire_t();
    wire_t* f = new wire_t();
    wire_t* g = new wire_t();
    wire_t* h = new wire_t();
    wire_t* i = new wire_t();
    wire_t* j = new wire_t();
    wire_t* k = new wire_t();
    wire_t* l = new wire_t();
    is_zero_component_t* is_zero = new is_zero_component_t;
    if_component_t* if_component = new if_component_t;
    one_component_t* one_1 = new one_component_t;
    one_component_t* one_2 = new one_component_t;
    mul_component_t* mul = new mul_component_t;
    sub_component_t* sub = new sub_component_t;
    pin_component_t* pin_1 = new pin_component_t;
    pin_component_t* pin_2 = new pin_component_t;
    pin_component_t* pin_3 = new pin_component_t;
    stub_component_t* fact = new stub_component_t(m_exposed_pins.size());
    fact->set_name("fact");

    if_component->connect(4, f);

    if_component->connect(3, e);

    pin_1->connect(-1, b);
    pin_2->connect(-1, h);
    mul->connect(2, i);
    if_component->connect(2, b);
    pin_3->connect(-1, at(1));
    sub->connect(2, k);

    pin_3->connect(-1, i);
    pin_2->connect(-1, g);
    pin_1->connect(-1, a);
    sub->connect(1, l);
    if_component->connect(1, d);
    is_zero->connect(1, c);
    fact->connect(1, j);
    mul->connect(1, j);

    one_2->connect(0, l);
    one_1->connect(0, d);
    pin_3->connect(-1, e);
    pin_2->connect(-1, f);
    mul->connect(0, g);
    sub->connect(0, h);
    if_component->connect(0, c);
    is_zero->connect(0, a);
    pin_1->connect(-1, at(0));
    fact->connect(0, k);
}

display_component_t::display_component_t(): component_t(1) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: in <int>
*/
void display_component_t::call(wire_t* caller) {
    (void) caller;

    int in;
    if (!read<int>(0, in)) {
        return ;
    }

    std::cout << in << std::endl;
}

mul_two_component_t::mul_two_component_t(): component_t(2) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

/**
 * 0: in <int>
 * 1: out <int>
*/
void mul_two_component_t::call(wire_t* caller) {
    (void) caller;

    int a;
    if (!read<int>(0, a)) {
        return ;
    }

    write<int>(1, a * 2);
}

/**
 * 0: in/out <int>
 * 1: in/out <int>
 * ...
*/
void pin_component_t::call(wire_t* caller) {
    for (wire_t* wire : m_wires) {
        if (wire == caller) {
            continue ;
        }

        wire->write(caller, this);
    }
}

void pin_component_t::connect(size_t index, wire_t* wire) {
    (void) index;
    assert(wire != nullptr);

    m_wires.push_back(wire);
    wire->connect(this);
}

void pin_component_t::disconnect(size_t index, wire_t* wire) {
    (void) index;
    assert(wire != nullptr);

    for (size_t i = 0; i < m_wires.size(); ++i) {
        if (m_wires[i] == wire) {
            wire->disconnect(this);
            for (size_t j = i; j + 1 < m_wires.size(); ++j) {
                m_wires[j] = m_wires[j + 1];
            }
            m_wires.pop_back();
            break ;
        }
    }
}

std::vector<wire_t*> pin_component_t::incoming_wires(size_t index) {
    if (m_wires.size() <= index) {
        return {};
    }

    return { m_wires[index] };
}

stub_component_t::stub_component_t(size_t exposed_pins): component_t(exposed_pins) {
    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        at(i)->connect(this);
    }
}

void stub_component_t::call(wire_t* caller) {
    if (caller->m_data_type_id == -1) {
        return ;
    }

    component_t* copy = COMPILER.create(m_name);
    if (copy == nullptr) {
        return ;
    }

    for (size_t i = 0; i < m_exposed_pins.size(); ++i) {
        pin_component_t* stub_pin = m_exposed_pins[i];
        if (stub_pin == nullptr) {
            continue ;
        }

        wire_t* stub_pin_wire = m_wires[i];
        assert(stub_pin_wire != nullptr);
        for (size_t j = 0; j < stub_pin->m_wires.size(); ++j) {
            wire_t* incoming_wire = stub_pin->m_wires[j];
            if (incoming_wire == stub_pin_wire) {
                continue ;
            }

            stub_pin->disconnect(-1, incoming_wire);
            copy->connect(i, incoming_wire);
        }
    }

    // todo: can delete stub component (self) now
}
