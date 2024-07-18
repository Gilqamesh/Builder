#ifndef MESSAGE_H
# define MESSAGE_H

# include <iostream>
# include <vector>

namespace dson {

template <typename T>
struct message_header_t {
    message_header_t();

    T m_id;
    uint32_t m_size;
};

template <typename T>
struct message_t {
    message_t(abs_ptr_t<shared_memory_t> shared_memory);

    template <typename... Args>
    message_t(abs_ptr_t<shared_memory_t> shared_memory, const T& id, Args&&... args);

    message_header_t<T> m_message_header;
    std::vector<uint8_t> m_body;

    size_t size() const;

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const message_t<T>& message);

    template <typename data_t>
    message_t& operator<<(const data_t& data);

    template <typename data_t>
    message_t& operator>>(data_t& data);
};

};

#endif // MESSAGE_H
