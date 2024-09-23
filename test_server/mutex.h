#ifndef MUTEX_H
# define MUTEX_H

# include <cstdint>
# include <queue>
# include <unordered_set>
# include <iostream>

# include "common.h"
# include "mem.h"

struct connection_t;

struct message_mutex_client_t {
    size_t m_process_id;
    size_t m_mutex_handle;
    enum class operation_t: int32_t {
        WRITE_LOCK,
        WRITE_UNLOCK,
        READ_LOCK,
        READ_UNLOCK
    } m_operation;

    friend std::ostream& operator<<(std::ostream& os, const message_mutex_client_t& message_client);
};

struct message_mutex_server_t {
    size_t m_mutex_handle;
    enum class operation_t: int32_t {
        PERMITTED,
        SHOULD_THROW
    } m_operation;

    friend std::ostream& operator<<(std::ostream& os, const message_mutex_server_t& message_server);
};

struct mutex_server_t {
    mutex_server_t(size_t handle);

    void write_lock(connection_t* connection);

    void write_unlock(connection_t* connection);

    void read_lock(connection_t* connection);

    void read_unlock(connection_t* connection);

    size_t m_handle;
    connection_t* m_writer;
    std::queue<connection_t*> m_read_requests;
    std::unordered_map<connection_t*, int> m_readers;
    std::queue<connection_t*> m_write_requests;

    size_t m_debug_readers;
    size_t m_debug_writers;
};

// note: process shareable
template <typename T>
struct mutex_client_t {
    mutex_client_t(T&& data);

    struct mutex_read_guard {
        mutex_read_guard(mutex_client_t& mutex_client, connection_t* connection);
        ~mutex_read_guard();

        mutex_read_guard scoped_read();

        mutex_client_t& m_mutex_client;
        connection_t* m_connection;
    };

    struct mutex_write_guard {
        mutex_write_guard(mutex_client_t& mutex_client, connection_t* connection);
        ~mutex_write_guard();

        mutex_read_guard scoped_read();
        mutex_write_guard scoped_write();

        mutex_client_t& m_mutex_client;
        connection_t* m_connection;
    };

    mutex_write_guard scoped_write(connection_t* connection);
    mutex_read_guard scoped_read(connection_t* connection);

    T m_prev;
    T m_data;

    size_t m_handle;
};

#endif // MUTEX_H
