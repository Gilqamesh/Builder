#ifndef COMPONENTS_H
# define COMPONENTS_H

#include "core/wire.h"

struct if_component_t: public component_t {
    if_component_t();

    // in: [condition <bool>, consequence_in, alternative_in]
    // out: [consequence_out, alternative_out]
    void call(wire_t* caller) override;
};

struct mul_component_t : public component_t {
    mul_component_t();

    // in: [a <int>, b <int>]
    // out: [result <int>]
    void call(wire_t* caller) override;
};

struct is_zero_component_t : public component_t {
    is_zero_component_t();

    // in: [in <int>]
    // out: [out <bool>]
    void call(wire_t* caller) override;
};

struct one_component_t : public component_t {
    one_component_t();

    // out: [out <int>]
    void call(wire_t* caller) override;
};

struct sub_component_t : public component_t {
    sub_component_t();

    // in: [a <int>, b <int>]
    // out: [result <int>]
    void call(wire_t* caller) override;
};

struct add_component_t : public component_t {
    add_component_t();

    // in: [a <int>, b <int>]
    // out: [result <int>]
    void call(wire_t* caller) override;
};

// in: [in <int>]
// out: [out <int>]
struct fact_component_t : public component_t {
    fact_component_t();
};

struct display_component_t : public component_t {
    display_component_t();

    // in: [in <int>]
    void call(wire_t* caller) override;
};

struct mul_two_component_t : public component_t {
    mul_two_component_t();

    // in: [in <int>]
    // out: [out <int>]
    void call(wire_t* caller) override;
};

struct pin_component_t: public component_t {
    // in: [in <T>, ...]
    // out: [out <T>, ...]
    void call(wire_t* caller) override;
    void connect(size_t index, wire_t* wire) override;
    void disconnect(size_t index, wire_t* wire) override;
    std::vector<wire_t*> incoming_wires(size_t index) override;
};

struct stub_component_t: public component_t {
    stub_component_t(size_t exposed_pins);

    void call(wire_t* caller) override;
};

#endif // COMPONENTS_H
