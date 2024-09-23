#ifndef CONNECTION_H
# define CONNECTION_H

# include <boost/asio.hpp>

# include "mutex.h"
# include "common.h"

struct connection_t : public boost::asio::ip::tcp::socket {
    using base = boost::asio::ip::tcp::socket;

    // todo: queue of messages, for now 1 at a time is sufficient for mutexes

    connection_t(boost::asio::io_context& io_context);

    void send(size_t mutex_handle, message_mutex_server_t::operation_t operation);

    void send(size_t mutex_handle, message_mutex_client_t::operation_t operation);

    void receive(size_t expected_handle);

    void receive(std::unordered_map<size_t, mutex_server_t*>& shared_locks);

    size_t m_debug_sent_bytes;
    size_t m_debug_received_bytes;
};

struct network_connection_t {
};

struct process_connection_t {
};

#endif // CONNECTION_H
