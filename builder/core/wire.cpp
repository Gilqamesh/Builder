#include "wire.h"

wire_t::wire_t():
    m_from(nullptr),
    m_to(nullptr),
    m_data_type_id(-1)
{
}

void wire_t::write(wire_t& other) {
    write(&other);
}

void wire_t::write(wire_t* other) {
    if (other->m_data_type_id == -1) {
        return ;
    }

    m_data_type_id = other->m_data_type_id;
    m_data = other->m_data;

    if (m_to) {
        m_to->call();
    }
}
