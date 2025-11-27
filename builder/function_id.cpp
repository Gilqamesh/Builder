#include "function_id.h"

bool function_id_t::operator==(const function_id_t& other) const {
    return ns == other.ns && name == other.name && creation_time == other.creation_time;
}
