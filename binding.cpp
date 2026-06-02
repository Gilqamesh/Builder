#include "binding.h"

#include <utility>

namespace kernel {

namespace binding {

binding_t::binding_t(std::string key, std::string value):
    m_key(std::move(key)),
    m_value(std::move(value))
{
}

const std::string& binding_t::key() const {
    return m_key;
}

const std::string& binding_t::value() const {
    return m_value;
}

} // namespace binding

} // namespace kernel
