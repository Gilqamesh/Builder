#ifndef PROCESS_H
# define PROCESS_H

# include "typesystem.h"

using clock_type_t = std::chrono::high_resolution_clock;
using time_type_t = std::chrono::time_point<clock_type_t>;

void set_time_init();
time_type_t get_time();
time_type_t get_time_init();

struct process_t;

struct connection_t {
    process_t* from = nullptr;
    process_t* to = nullptr;

    bool is_initialized = false;
    int message_type_id = -1;
    void* message = nullptr;
    // void* coerced_message;
};

class process_t {
public:
    template <typename FInit, typename FCall>
    process_t(std::string name, FInit&& init, FCall&& call);

    ~process_t();

    template <typename T>
    void define_in(size_t index, std::string name);
    void define_in(size_t index, std::string name, int type_id);

    template <typename T>
    void define_out(size_t index, std::string name);
    void define_out(size_t index, std::string name, int type_id);

    void connect_in(size_t index, connection_t* connection);
    void connect_out(size_t index, connection_t* connection);

    template <typename T>
    T receive(size_t index);

    template <typename T>
    void send(size_t index, T out);

    size_t size_in();
    size_t size_out();

    void initialize();
    bool call();

private:
    bool call(time_type_t t_propagate);
    bool call_preamble(time_type_t t_propagate);
    bool call_impl();
    bool call_postamble(time_type_t t_propagate);

    void propagate(time_type_t t_propagate);

private:
    std::string m_name;
    std::function<void(process_t*)> m_init;
    std::function<bool(process_t*)> m_call;

    bool m_is_initialized;

    std::vector<int> m_type_id_in;
    std::vector<std::string> m_names_in;
    std::vector<connection_t*> m_in;

    std::vector<std::string> m_names_out;
    std::vector<int> m_type_id_out;
    std::vector<connection_t*> m_out;

    time_type_t m_t_start;
    time_type_t m_t_success;
    time_type_t m_t_failure;
    time_type_t m_t_finish;
    time_type_t m_t_propagate;
};

template <typename FInit, typename FCall>
process_t::process_t(std::string name, FInit&& init, FCall&& call):
    m_name(std::move(name)),
    m_init(init),
    m_call(call),
    m_is_initialized(false)
{
}

template <typename T>
void process_t::define_in(size_t index, std::string name) {
    TYPESYSTEM.register_type<T>();
    define_in(index, std::move(name), TYPESYSTEM.type_id<T>());
}

template <typename T>
void process_t::define_out(size_t index, std::string name) {
    TYPESYSTEM.register_type<T>();
    define_out(index, std::move(name), TYPESYSTEM.type_id<T>());
}

template <typename T>
T process_t::receive(size_t index) {
    if (m_in.size() <= index) {
        throw std::runtime_error("index is out of bounds");
    }

    connection_t* connection = m_in[index];

    if (!connection) {
        throw std::runtime_error("no connection to receive from");
    }

    if (!connection->is_initialized) {
        throw std::runtime_error("connection is not initialized");
    }

    T result;
    if (!TYPESYSTEM.coerce(connection->message, connection->message_type_id, &result)) {
        throw std::runtime_error("receive: coercion failed");
    }
    return result;
}

template <typename T>
void process_t::send(size_t index, T out) {
    if (m_out.size() <= index) {
        throw std::runtime_error("index is out of bounds");
    }

    connection_t* connection = m_out[index];

    if (!connection) {
        throw std::runtime_error("no connection to sent to");
    }

    if (!connection->message) {
        throw std::runtime_error("connection doesn't have a receiver");
    }

    TYPESYSTEM.cast<T>(connection->message, connection->message_type_id) = out;
    connection->is_initialized = true;
}

#endif // PROCESS_H
