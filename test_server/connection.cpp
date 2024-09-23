#include "connection.h"

connection_t::connection_t(boost::asio::io_context& io_context):
    base(io_context),
    m_debug_sent_bytes(0),
    m_debug_received_bytes(0)
{
}

// todo: queue of messages, for now 1 at a time is sufficient for mutexes

void connection_t::send(size_t mutex_handle, message_mutex_server_t::operation_t operation) {
    message_mutex_server_t message_server;
    message_server.m_mutex_handle = mutex_handle;
    message_server.m_operation = message_mutex_server_t::operation_t::PERMITTED;

    DEBUG_LOG("Server", "Writing message to client...");
    size_t bytes_written = write_some(boost::asio::buffer(&message_server, sizeof(message_server)));
    m_debug_sent_bytes += bytes_written;
    if (bytes_written != sizeof(message_server)) {
        std::runtime_error("Unexpected bytes written from server");
    }
    DEBUG_LOG("Server", "Wrote message to client: " << message_server);
}

void connection_t::send(size_t mutex_handle, message_mutex_client_t::operation_t operation) {
    message_mutex_client_t message_client;
    message_client.m_mutex_handle = mutex_handle;
    message_client.m_process_id = static_cast<size_t>(boost::this_process::get_id());
    message_client.m_operation = operation;

    DEBUG_LOG("Client", "Writing message to server...");
    size_t bytes_written = write_some(boost::asio::buffer(&message_client, sizeof(message_client)));
    m_debug_sent_bytes += bytes_written;
    if (bytes_written != sizeof(message_client)) {
        throw std::runtime_error("Unexpected bytes written from client");
    }
    DEBUG_LOG("Client", "Wrote message to server: " << message_client);
}

void connection_t::receive(size_t expected_handle) {
    DEBUG_LOG("Client", "Reading message from server...");
    message_mutex_server_t message_server;
    std::size_t bytes_read = read_some(boost::asio::buffer(&message_server, sizeof(message_server)));
    m_debug_received_bytes += bytes_read;
    if (bytes_read != sizeof(message_server)) {
        throw std::runtime_error("Unexpected bytes read from server");
    }
    if (message_server.m_mutex_handle != expected_handle) {
        throw std::runtime_error("Unexpected handle value from server (ack is different)");
    }
    DEBUG_LOG("Client", "Read message from server: " << message_server);

    switch (message_server.m_operation) {
    case message_mutex_server_t::operation_t::PERMITTED: {
    } break ;
    case message_mutex_server_t::operation_t::SHOULD_THROW: {
        throw std::runtime_error("Server replied with SHOULD_THROW");
    } break ;
    default: {
        throw std::runtime_error("Unexpected operation from server");
    }
    }
}

void connection_t::receive(std::unordered_map<size_t, mutex_server_t*>& shared_locks) {
    assert(is_open());
    if (std::size_t available_bytes = available()) {
        if (available_bytes < sizeof(message_mutex_client_t)) {
            // maybe client hasn't finished writing yet
        } else {
            DEBUG_LOG("Server", "Reading message from client...");
            message_mutex_client_t message_client;
            size_t bytes_read = read_some(boost::asio::buffer(&message_client, sizeof(message_client)));
            m_debug_received_bytes += bytes_read;
            if (bytes_read != sizeof(message_client)) {
                LOG("Server", "Size not what is expected, maybe need several reads");
            } else {
                auto shared_lock_it = shared_locks.find(message_client.m_mutex_handle);
                if (shared_lock_it == shared_locks.end()) {
                    shared_lock_it = shared_locks.emplace_hint(shared_lock_it, message_client.m_mutex_handle, new mutex_server_t(message_client.m_mutex_handle));
                }
                assert(shared_lock_it != shared_locks.end());
                switch (message_client.m_operation) {
                case message_mutex_client_t::operation_t::WRITE_LOCK: {
                    shared_lock_it->second->write_lock(this);
                } break ;
                case message_mutex_client_t::operation_t::WRITE_UNLOCK: {
                    shared_lock_it->second->write_unlock(this);
                } break ;
                case message_mutex_client_t::operation_t::READ_LOCK: {
                    shared_lock_it->second->read_lock(this);
                } break ;
                case message_mutex_client_t::operation_t::READ_UNLOCK: {
                    shared_lock_it->second->read_unlock(this);
                } break ;
                default: {
                    LOG("Server", "Unknown operation type");
                }
                }
            }
        }
    }
}
