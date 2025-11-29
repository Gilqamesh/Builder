#include <typesystem_call.h>
#include <typesystem.h>

typesystem_call_t::typesystem_call_t(typesystem_t& typesystem, void* function_symbol)
    : call_t(function_symbol), m_typesystem(typesystem) {}

