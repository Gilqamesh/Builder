#include "primitive.h"

static std::string make_primitive_key(const primitive_locator_t& locator) {
    return locator.ns + "." + locator.name;
}

void primitive_t::save(const primitive_locator_t& locator, call_t primitive) {
    const auto primitive_key = make_primitive_key(locator);
    auto it = primitives.find(primitive_key);
    if (it != primitives.end()) {
        throw std::runtime_error(std::format("primitive '{}' is already registered", primitive_key));
    }

    ir_t* ir = new ir_t;
    ir->call = primitive;

    primitives.emplace(primitive_key, ir);
}

ir_t* primitive_t::load(const primitive_locator_t& locator) {
    const auto primitive_key = make_primitive_key(locator);
    auto it = primitives.find(primitive_key);
    if (it == primitives.end()) {
        throw std::runtime_error(std::format("primitive '{}' is not registered", primitive_key));
    }

    return it->second;
}
