#ifndef TYPESYSTEM_CALL_H
# define TYPESYSTEM_CALL_H

#include <typesystem.h>
#include <call.h>

struct typesystem_call_t : public call_t {
public:
    typesystem_call_t(typesystem_t& typesystem, void* function_symbol);

private:
    typesystem_t& m_typesystem;
};

#endif // TYPESYSTEM_CALL_H
