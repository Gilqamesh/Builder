#include "msg.h"

#include <type_traits>

template <typename T>
message_header_t<T>::message_header_t():
    m_id{},
    m_size(0) {
}

template <typename T>
message_t<T>::message_t():
    m_message_header{},
    m_body() {
}

template <typename T>
template <typename... Args>
message_t<T>::message_t(const T& id, Args&&... args):
    message_t() {
    m_message_header.m_id = id;
    (*this << ... << args);
}

template <typename T>
size_t message_t<T>::size() const {
    size_t result = sizeof(m_message_header);

    m_body.read([&result](auto& self, auto& body) {
        (void) self;
        result += body.size();
    });

    return result;
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

    m_body.write([&data, this](auto& self, auto& body) {
        (void) self;
        size_t body_size_prev = body.size();
        body.resize(body_size_prev + sizeof(data_t));
        std::memcpy(body.data() + body_size_prev, &data, sizeof(data_t));
        m_message_header.m_size = size();
    });

    return *this;
}

template <typename T>
template <typename data_t>
message_t<T>& message_t<T>::operator>>(data_t& data) {
    static_assert(std::is_standard_layout<data_t>::value, "data_t is not standard layout");

    m_body.write([&data, this](auto& self, auto& body) {
        (void) self;
        assert(sizeof(data_t) <= body.size());
        size_t body_size_new = body.size() - sizeof(data_t);
        std::memcpy(&data, body.data() + body_size_new, sizeof(data_t));
        body.resize(body_size_new);
        m_message_header.m_size = size();
    });

    return *this;
}
