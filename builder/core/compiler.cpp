#include "compiler.h"
#include "components.h"

compiler_t COMPILER;

void compiler_t::add(std::string name, const std::vector<unsigned char> instructions) {
    if (
        m_primitive_components.find(name) != m_primitive_components.end() ||
        m_components.find(name) != m_components.end()
    ) {
        throw std::runtime_error("component with the same name is already registered: '" + name + "'");
    }

    m_components.emplace(std::move(name), instructions);
}

component_t* compiler_t::create(std::string name) {
    auto it = m_primitive_components.find(name);
    if (it != m_primitive_components.end()) {
        return it->second();
    }

    auto it2 = m_components.find(name);
    if (it2 == m_components.end()) {
        throw std::runtime_error("unregistered component name: " + name);
    }

    const std::vector<unsigned char>& instructions = it2->second;

    component_t* result = new component_t;
    result->set_name(std::move(name));

    std::vector<wire_t*> wires;
    std::vector<component_t*> components;

    size_t ip = 0;
    const size_t instructions_size = instructions.size();
    while (ip < instructions_size) {
        unsigned char op = instructions[ip++];
        switch (op) {
        case OP_CREATE_WIRE: {
            wires.push_back(new wire_t);
        } break ;
        case OP_EXPOSE_PINS: {
            if (instructions_size < ip + 1) {
                throw std::runtime_error("expected 1 byte for expose pins of resulting component");
            }
            size_t n_exposed_pins = instructions[ip++];
            for (size_t i = 0; i < n_exposed_pins; ++i) {
                result->expose(i);
            }
        } break ;
        case OP_CREATE_COMPONENT: {
            std::string component_name;
            while (ip < instructions_size && instructions[ip] != 0) {
                component_name.push_back(instructions[ip]);
                ++ip;
            }
            if (ip == instructions_size) {
                throw std::runtime_error("unterminated component name");
            }
            assert(instructions[ip] == 0);
            ++ip;
            components.push_back(create(component_name));
        } break ;
        case OP_CREATE_STUB_COMPONENT: {
            std::string component_name;
            while (ip < instructions_size && instructions[ip] != 0) {
                component_name.push_back(instructions[ip]);
                ++ip;
            }
            if (ip == instructions_size) {
                throw std::runtime_error("unterminated component name");
            }
            assert(instructions[ip] == 0);
            ++ip;
            size_t n_exposed_pins = instructions[ip++];
            stub_component_t* stub = new stub_component_t(n_exposed_pins);
            stub->set_name(component_name);
            components.push_back(stub);
        } break ;
        case OP_CONNECT_WIRE_TO_COMPONENT: {
            if (instructions_size < ip + 3) {
                throw std::runtime_error("expected 3 bytes for connect wire to component input");
            }
            uint8_t wire_index = instructions[ip++];
            uint8_t component_index = instructions[ip++];
            uint8_t component_input_index = instructions[ip++];
            if (wires.size() <= wire_index) {
                throw std::runtime_error("wire index is out of bounds");
            }
            if (components.size() <= component_index) {
                throw std::runtime_error("component index is out of bounds");
            }
            components[component_index]->connect(component_input_index, 
            wires[wire_index]);
        } break ;
        case OP_CONNECT_COMPONENT_PIN_TO_RESULT_PIN: {
            if (instructions_size < ip + 3) {
                throw std::runtime_error("expected 3 bytes for connect component pin to resulting component pin");
            }
            uint8_t result_pin_index = instructions[ip++];
            uint8_t component_index = instructions[ip++];
            uint8_t component_pin_index = instructions[ip++];
            if (result->m_exposed_pins.size() <= result_pin_index) {
                throw std::runtime_error("resulting component pin index is out of bounds");
            }
            if (components.size() <= component_index) {
                throw std::runtime_error("component index is out of bounds");
            }
            wire_t* result_wire = result->at(result_pin_index);
            if (!result_wire) {
                throw std::runtime_error("resulting component pin is not exposed");
            }
            components[component_index]->connect(component_pin_index, result_wire);
        } break ;
        default: {
            throw std::runtime_error("unknown opcode: " + std::to_string(op));
        } break ;
        }
    }

    return result;
}

std::string compiler_t::disassemble(const std::vector<unsigned char> instructions) {
    std::string result;

    size_t wire_index = 0;
    size_t component_index = 0;
    std::unordered_map<size_t, std::string> component_names;

    size_t ip = 0;
    const size_t instructions_size = instructions.size();
    while (ip < instructions_size) {
        unsigned char op = instructions[ip++];
        switch (op) {
        case OP_CREATE_WIRE: {
            result += "w" + std::to_string(wire_index++) + " = create wire\n";
        } break ;
        case OP_EXPOSE_PINS: {
            if (instructions_size < ip + 1) {
                throw std::runtime_error("expected 1 byte for expose pins of resulting component");
            }
            size_t n_exposed_pins = instructions[ip++];
            result += "expose " + std::to_string(n_exposed_pins) + " pins\n";
        } break ;
        case OP_CREATE_COMPONENT: {
            std::string component_name;
            while (ip < instructions_size && instructions[ip] != 0) {
                component_name.push_back(instructions[ip]);
                ++ip;
            }
            if (ip == instructions_size) {
                throw std::runtime_error("unterminated component name");
            }
            assert(instructions[ip] == 0);
            ++ip;
            component_names[component_index] = component_name;
            result += "c" + std::to_string(component_index++) + " = component " + component_name + "\n";
        } break ;
        case OP_CREATE_STUB_COMPONENT: {
            std::string component_name;
            while (ip < instructions_size && instructions[ip] != 0) {
                component_name.push_back(instructions[ip]);
                ++ip;
            }
            if (ip == instructions_size) {
                throw std::runtime_error("unterminated component name");
            }
            assert(instructions[ip] == 0);
            ++ip;
            size_t n_exposed_pins = instructions[ip++];
            component_names[component_index] = component_name;
            result += "c" + std::to_string(component_index++) + " = stub( " + std::to_string(n_exposed_pins) + ") " + component_name + "\n";
        } break ;
        case OP_CONNECT_WIRE_TO_COMPONENT: {
            if (instructions_size < ip + 3) {
                throw std::runtime_error("expected 3 bytes for connect wire to component");
            }
            uint8_t wire_index = instructions[ip++];
            uint8_t component_index = instructions[ip++];
            uint8_t component_input_index = instructions[ip++];
            result += "w" + std::to_string(wire_index) + " -> c" + std::to_string(component_index) + "[" + std::to_string(component_input_index) + "]\n";
        } break ;
        case OP_CONNECT_COMPONENT_PIN_TO_RESULT_PIN: {
            if (instructions_size < ip + 3) {
                throw std::runtime_error("expected 3 bytes for connect component pin to resulting component pin");
            }
            uint8_t result_pin_index = instructions[ip++];
            uint8_t component_index = instructions[ip++];
            uint8_t component_pin_index = instructions[ip++];
            result += "result[" + std::to_string(result_pin_index) + "] -> c" + std::to_string(component_index) + "[" + std::to_string(component_pin_index) + "]\n";
        } break ;
        default: {
            throw std::runtime_error("unknown opcode: " + std::to_string(op));
        } break ;
        }
    }

    return result;
}

bool compiler_t::find(std::string name) {
    return m_primitive_components.find(name) != m_primitive_components.end() || m_components.find(name) != m_components.end();
}

std::vector<unsigned char> compiler_t::assemble(
    unsigned char n_exposed_pins,
    std::vector<component_t*> components_connected_to_exposed_pins,
    std::vector<unsigned char> component_pin_indices,
    std::vector<wire_t*> wires,
    std::vector<component_t*> components
) {
    std::vector<unsigned char> result;

    std::unordered_map<wire_t*, unsigned char> wire_indices;
    std::unordered_map<component_t*, unsigned char> component_indices;

    for (size_t i = 0; i < wires.size(); ++i) {
        wire_t* wire = wires[i];
        result.push_back(OP_CREATE_WIRE);
        unsigned char wire_index = (unsigned char) wire_indices.size();
        wire_indices[wire] = wire_index;
    }

    result.push_back(OP_EXPOSE_PINS);
    result.push_back(n_exposed_pins);

    if ((unsigned char) -1 <= components.size()) {
        throw std::runtime_error("too many inner components, max: " + std::to_string((unsigned char) -1));
    }
    for (size_t component_index = 0; component_index < components.size(); ++component_index) {
        component_t* component = components[component_index];
        component_indices[component] = (unsigned char) component_index;
        if (find(component->m_name)) {
            result.push_back(OP_CREATE_COMPONENT);
        } else {
            result.push_back(OP_CREATE_STUB_COMPONENT);
            assert(component->m_exposed_pins.size() <= (size_t) -1);
            result.push_back((unsigned char) component->m_exposed_pins.size());
        }
        for (unsigned char c : component->m_name) {
            result.push_back(c);
        }
        result.push_back(0);

        for (size_t component_wire_index = 0; component_wire_index < component->m_wires.size(); ++component_wire_index) {
            wire_t* wire = component->m_wires[component_wire_index];
            if (!wire) {
                continue ;
            }

            auto it = wire_indices.find(wire);
            assert(it != wire_indices.end());
            unsigned char wire_index = it->second;
            result.push_back(OP_CONNECT_WIRE_TO_COMPONENT);
            result.push_back(wire_index);
            result.push_back((unsigned char) component_index);
            result.push_back(component_wire_index);
        }
    }

    if (n_exposed_pins != components_connected_to_exposed_pins.size()) {
        throw std::runtime_error("number of exposed pins does not match number of components connected to exposed pins");
    }
    if (n_exposed_pins != component_pin_indices.size()) {
        throw std::runtime_error("number of exposed pins does not match number of component pin indices");
    }
    for (size_t i = 0; i < components_connected_to_exposed_pins.size(); ++i) {
        component_t* component = components_connected_to_exposed_pins[i];
        if (component == nullptr) {
            continue ;
        }
        auto it = component_indices.find(component);
        if (it == component_indices.end()) {
            throw std::runtime_error("component connected to exposed pin is not in the list of inner components");
        }
        result.push_back(OP_CONNECT_COMPONENT_PIN_TO_RESULT_PIN);
        result.push_back((unsigned char) i);
        result.push_back(it->second);
        result.push_back(component_pin_indices[i]);
    }

    return result;
}
