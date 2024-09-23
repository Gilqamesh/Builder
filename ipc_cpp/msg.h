#ifndef IPC_MSG_H
# define IPC_MSG_H

# include "mem.h"

# include <iostream>

template <typename T>
struct message_header_t {
    message_header_t();

    T m_id;
    uint32_t m_size;
};

template <typename T>
struct message_t {
    message_t();

    template <typename... Args>
    message_t(const T& id, Args&&... args);

    message_header_t<T> m_message_header;
    ipc_mem::shared_vector_t<uint8_t> m_body;

    size_t size() const;

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const message_t<T>& message);

    template <typename data_t>
    message_t& operator<<(const data_t& data);

    template <typename data_t>
    message_t& operator>>(data_t& data);
};

# include "msg_impl.h"

#endif // IPC_MSG_H
