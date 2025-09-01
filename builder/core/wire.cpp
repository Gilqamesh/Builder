#include "wire.h"
#include "compiler.h"
#include "components.h"

wire_t::wire_t():
    m_a(nullptr),
    m_b(nullptr),
    m_data_type_id(-1)
{
}

void wire_t::connect(component_t* component) {
    assert(component != nullptr && "use disconnect");

    if (!m_a) {
        m_a = component;
    } else if (!m_b) {
        m_b = component;
    } else {
        throw std::runtime_error("wire already has two components connected");
    }

    assert(m_a != m_b && "wire cannot connect the same component twice");

    component->call(this);
}

void wire_t::disconnect(component_t* component) {
    assert(component != nullptr);

    if (m_a == component) {
        m_a = nullptr;
    } else if (m_b == component) {
        m_b = nullptr;
    } else {
        assert(false && "component not connected to wire");
    }
}

component_t* wire_t::other(component_t* self) {
    component_t* result = nullptr;

    if (m_a == self) {
        result = m_b;
    } else if (m_b == self) {
        result = m_a;
    }

    return result;
}

void wire_t::write(wire_t& other) {
    write(&other, nullptr);
}

void wire_t::write(wire_t* other) {
    write(other, nullptr);
}

void wire_t::write(wire_t& other, component_t* writer) {
    write(&other, writer);
}

void wire_t::write(wire_t* other, component_t* writer) {
    if (other->m_data_type_id == -1) {
        return ;
    }

    m_data_type_id = other->m_data_type_id;
    m_data = other->m_data;

    if (m_a && m_a != writer) {
        m_a->call(this);
    }

    if (m_b && m_b != writer) {
        m_b->call(this);
    }
}

component_t::component_t()
{
}

component_t::component_t(size_t exposed_pins)
{
    for (size_t i = 0; i < exposed_pins; ++i) {
        expose(i);
    }
}

void component_t::set_name(std::string name) {
    m_name = std::move(name);
}

void component_t::connect(size_t index, wire_t* wire) {
    assert(wire != nullptr && "use disconnect");

    if (
        m_exposed_pins.size() <= index ||
        m_exposed_pins[index] == nullptr
    ) {
        throw std::runtime_error("cannot connect to wire to index " + std::to_string(index) + ", because it is not exposed");
    }

    m_exposed_pins[index]->connect(-1, wire);
}

void component_t::disconnect(size_t index, wire_t* wire) {
    if (
        m_exposed_pins.size() <= index ||
        m_exposed_pins[index] == nullptr
    ) {
        return ;
    }

    m_exposed_pins[index]->disconnect(-1, wire);
}

void component_t::expose(size_t index) {
    assert(m_exposed_pins.size() == m_wires.size());
    if (m_exposed_pins.size() <= index) {
        m_exposed_pins.resize(index + 1, nullptr);
        m_wires.resize(index + 1, nullptr);
    }

    if (m_exposed_pins[index] == nullptr) {
        assert(m_wires[index] == nullptr);
        m_exposed_pins[index] = new pin_component_t();
        m_wires[index] = new wire_t();
        connect(index, m_wires[index]);
    }
}

wire_t* component_t::at(size_t index) {
    if (m_wires.size() <= index) {
        return nullptr;
    }

    return m_wires[index];
}

std::vector<wire_t*> component_t::incoming_wires(size_t index) {
    std::vector<wire_t*> result;

    if (
        m_exposed_pins.size() <= index ||
        m_exposed_pins[index] == nullptr
    ) {
        return result;
    }

    wire_t* self_wire = at(index);
    for (wire_t* wire : m_exposed_pins[index]->m_wires) {
        if (wire != nullptr && wire != self_wire) {
            result.push_back(wire);
        }
    }

    return result;
}

void component_t::mov(size_t from_index, size_t to_index) {
    if (
        m_wires.size() <= from_index ||
        m_wires.size() <= to_index ||
        m_wires[from_index] == nullptr ||
        m_wires[to_index] == nullptr
    ) {
        return ;
    }

    m_wires[to_index]->write(m_wires[from_index], this);
}
