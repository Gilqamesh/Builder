#include "component.h"

void component_t::set_name(std::string name) {
    m_name = std::move(name);
}

void component_t::add_component(component_t* component) {
}

void component_t::call() {
    call_impl();
}
