#include "mutex.h"

#include "connection.h"

std::ostream& operator<<(std::ostream& os, const message_mutex_client_t& message_client) {
    os << "id: " << message_client.m_process_id << ", handle: " << message_client.m_mutex_handle << ", operation: ";
    switch (message_client.m_operation) {
        case message_mutex_client_t::operation_t::WRITE_LOCK: {
            os << "write lock";
        } break ;
        case message_mutex_client_t::operation_t::WRITE_UNLOCK: {
            os << "write unlock";
        } break ;
        case message_mutex_client_t::operation_t::READ_LOCK: {
            os << "read lock";
        } break ;
        case message_mutex_client_t::operation_t::READ_UNLOCK: {
            os << "read unlock";
        } break ;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const message_mutex_server_t& message_server) {
    os << "handle: " << message_server.m_mutex_handle << ", operation: ";
    switch (message_server.m_operation) {
        case message_mutex_server_t::operation_t::PERMITTED: {
            os << "permitted";
        } break ;
        case message_mutex_server_t::operation_t::SHOULD_THROW: {
            os << "should throw";
        } break ;
    }

    return os;
}

mutex_server_t::mutex_server_t(size_t handle):
    m_handle(handle),
    m_writer(0),
    m_debug_readers(0),
    m_debug_writers(0)
{
}

void mutex_server_t::write_lock(connection_t* connection) {
    if (!m_writer && m_readers.empty()) {
        m_writer = connection;
        ++m_debug_writers;
        // LOG("Server", "Writers: " << m_debug_writers);
        connection->send(m_handle, message_mutex_server_t::operation_t::PERMITTED);
    } else {
        // if they are the writers, allow them this operation
        m_write_requests.push(connection);
        // LOG("Server", "Write requests: " << m_write_requests.size());
    }
}

void mutex_server_t::write_unlock(connection_t* connection) {
    if (m_writer != connection) {
        connection->send(m_handle, message_mutex_server_t::operation_t::SHOULD_THROW);
    } else {
        assert(0 < m_debug_writers);
        --m_debug_writers;
        // LOG("Server", "Writers: " << m_debug_writers);
        m_writer = 0;
        connection->send(m_handle, message_mutex_server_t::operation_t::PERMITTED);
        if (!m_write_requests.empty()) {
            connection_t* write_request = m_write_requests.front();
            m_write_requests.pop();
            write_lock(write_request);
        } else if (!m_read_requests.empty()) {
            connection_t* read_request = m_read_requests.front();
            m_read_requests.pop();
            read_lock(read_request);
        }
    }
}

void mutex_server_t::read_lock(connection_t* connection) {
    if (!m_writer || (m_writer && m_writer == connection)) {
        ++m_debug_readers;
        // LOG("Server", "Readers: " << m_debug_readers);
        ++m_readers[connection];
        connection->send(m_handle, message_mutex_server_t::operation_t::PERMITTED);
    } else {
        assert(m_writer && m_writer != connection);
        m_read_requests.push(connection);
        // LOG("Server", "Read requests: " << m_read_requests.size());
    }
}

void mutex_server_t::read_unlock(connection_t* connection) {
    auto reader_pair_it = m_readers.find(connection);
    if (reader_pair_it == m_readers.end()) {
        connection->send(m_handle, message_mutex_server_t::operation_t::SHOULD_THROW);
    } else {
        assert(0 < reader_pair_it->second);
        --reader_pair_it->second;
        assert(0 < m_debug_readers);
        --m_debug_readers;
        connection->send(m_handle, message_mutex_server_t::operation_t::PERMITTED);
        // LOG("Server", "Readers: " << reader_pair_it->second);
        if (reader_pair_it->second == 0) {
            m_readers.erase(reader_pair_it);
            if (m_readers.empty() && !m_write_requests.empty()) {
                connection_t* write_request = m_write_requests.front();
                m_write_requests.pop();
                write_lock(write_request);
            }
        }
    }
}

static size_t g_handle;

mutex_client_t::mutex_client_t():
    m_handle(++g_handle)
{
}

mutex_client_t::mutex_read_guard::mutex_read_guard(mutex_client_t& mutex_client, connection_t* connection):
    m_mutex_client(mutex_client),
    m_connection(connection),
    m_prev_operation(mutex_client.increment_ownership_count(operation_t::READ))
{
    if (m_prev_operation == operation_t::NONE) {
        m_connection->send(m_mutex_client.m_handle, message_mutex_client_t::operation_t::READ_LOCK);
        m_connection->receive(m_mutex_client.m_handle);
    }
}

mutex_client_t::mutex_read_guard mutex_client_t::mutex_read_guard::scoped_read() {
    return mutex_read_guard(m_mutex_client, m_connection);
}

mutex_client_t::mutex_read_guard::~mutex_read_guard() {
    m_mutex_client.decrement_ownership_count(m_prev_operation);
    if (m_prev_operation == operation_t::NONE) {
        m_connection->send(m_mutex_client.m_handle, message_mutex_client_t::operation_t::READ_UNLOCK);
        m_connection->receive(m_mutex_client.m_handle);
    }
}

mutex_client_t::mutex_write_guard::mutex_write_guard(mutex_client_t& mutex_client, connection_t* connection):
    m_mutex_client(mutex_client),
    m_connection(connection)
{
    m_connection->send(m_mutex_client.m_handle, message_mutex_client_t::operation_t::WRITE_LOCK);
    m_connection->receive(m_mutex_client.m_handle);
}

mutex_client_t::mutex_read_guard mutex_client_t::mutex_write_guard::scoped_read() {
    return mutex_read_guard(m_mutex_client, m_connection);
}

mutex_client_t::mutex_write_guard mutex_client_t::mutex_write_guard::scoped_write() {
    return mutex_write_guard(m_mutex_client, m_connection);
}

mutex_client_t::mutex_write_guard::~mutex_write_guard() {
    m_connection->send(m_mutex_client.m_handle, message_mutex_client_t::operation_t::WRITE_UNLOCK);
    m_connection->receive(m_mutex_client.m_handle);
}

mutex_client_t::mutex_write_guard mutex_client_t::scoped_write(connection_t* connection) {
    return mutex_write_guard(*this, connection);
}

mutex_client_t::mutex_read_guard mutex_client_t::scoped_read(connection_t* connection) {
    return mutex_read_guard(*this, connection);
}
