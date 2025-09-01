#ifndef COMPILER_H
# define COMPILER_H

# include "wire.h"

enum op_t {
    OP_CREATE_WIRE, // [op <u8>]
    OP_EXPOSE_PINS, // [op <u8>, n_exposed_pins <u8>]
    OP_CREATE_COMPONENT, // [op <u8>, component_name <null-terminated string>]
    OP_CREATE_STUB_COMPONENT, // [op <u8>, n_exposed_pins <u8>, component_name <null-terminated string>]
    OP_CONNECT_WIRE_TO_COMPONENT, // [op <u8>, wire_index <u8>, component_index <u8>, component_pin_index <u8>]
    OP_CONNECT_COMPONENT_PIN_TO_RESULT_PIN, // [op <u8>, result_pin_index <u8>, component_index <u8>, component_pin_index <u8>]
};

struct compiler_t {
public:
    void add(std::string name, const std::vector<unsigned char> instructions);

    template <typename T>
    void add(std::string name);

    component_t* create(std::string name);

    // disassemble instructions into human-readable format
    std::string disassemble(const std::vector<unsigned char> instructions);

    bool find(std::string name);

    // create a blueprint from existing wires and components
    std::vector<unsigned char> assemble(
        unsigned char n_exposed_pins,
        std::vector<component_t*> components_connected_to_exposed_pins,
        std::vector<unsigned char> component_pin_indices,
        std::vector<wire_t*> wires,
        std::vector<component_t*> components
    );

private:
    std::unordered_map<std::string, std::vector<unsigned char>> m_components;
    std::unordered_map<std::string, std::function<component_t*()>> m_primitive_components;
};

extern compiler_t COMPILER;

template <typename T>
void compiler_t::add(std::string name) {
    if (find(name)) {
        return ;
    }

    m_primitive_components.emplace(std::move(name), [] -> component_t* {
        return new T();
    });
}

// todo: create primitive_component_t as a base for all primitive components
//   - automatically registers itself in the COMPILER
//   - exposes pins based on template parameters
// template <typename T>
// struct primitive_component_t : public component_t {
// }

#endif // COMPILER_H
