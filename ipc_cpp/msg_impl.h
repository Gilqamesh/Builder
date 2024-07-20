#include "msg.h"

#include <type_traits>

template <typename T>
message_header_t<T>::message_header_t():
    m_id{},
    m_size(0) {
}

template <typename T>
message_t<T>::message_t(abs_ptr_t<shared_memory_t> shared_memory):
    m_message_header{},
    m_body(shared_memory) {
}

template <typename T>
template <typename... Args>
message_t<T>::message_t(abs_ptr_t<shared_memory_t> shared_memory, const T& id, Args&&... args):
    message_t(shared_memory) {
    m_message_header.m_id = id;
    (*this << ... << args);
}

template <typename T>
size_t message_t<T>::size() const {
    return sizeof(m_message_header) + m_body.size();
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const message_t<T>& message) {
    os << "ID: " << static_cast<int>(message.m_message_header.m_id) << ", size: " << message.size() << std::endl;
    return os;
}

template <typename T>
template <typename data_t>
message_t<T>& message_t<T>::operator<<(const data_t& data) {
    static_assert(std::is_standard_layout<data_t>::value, "data_t is not standard layout");

    size_t body_size_prev = m_body.size();
    m_body.resize(body_size_prev + sizeof(data_t));
    std::memcpy(m_body.data() + body_size_prev, &data, sizeof(data_t));
    m_message_header.m_size = size();

    return *this;
}

template <typename T>
template <typename data_t>
message_t<T>& message_t<T>::operator>>(data_t& data) {
    static_assert(std::is_standard_layout<data_t>::value, "data_t is not standard layout");

    assert(sizeof(data_t) <= m_body.size());
    size_t body_size_new = m_body.size() - sizeof(data_t);
    std::memcpy(&data, m_body.data() + body_size_new, sizeof(data_t));
    m_body.resize(body_size_new);
    m_message_header.m_size = size();

    return *this;
}
