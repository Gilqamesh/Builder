#include "process.h"

static time_type_t t_init;

void set_time_init() {
    t_init = get_time();
}

time_type_t get_time() {
    return std::chrono::high_resolution_clock::now();
}

time_type_t get_time_init() {
    return t_init;
}

process_t::~process_t() {
}

void process_t::define_in(size_t index, std::string name, int type_id) {
    const size_t old_size = m_type_id_in.size();
    if (old_size <= index) {
        const size_t new_size = index + 1;
        const size_t delta_size = new_size - old_size;
        m_type_id_in.insert(m_type_id_in.end(), delta_size, -1);
        m_names_in.insert(m_names_in.end(), delta_size, std::string());
        m_in.insert(m_in.end(), delta_size, nullptr);
    }

    if (m_type_id_in[index] != -1) {
        throw std::runtime_error("input slot is already defined");
    }

    m_type_id_in[index] = type_id;
    m_names_in[index] = std::move(name);
}

void process_t::define_out(size_t index, std::string name, int type_id) {
    const size_t old_size = m_type_id_out.size();
    if (old_size <= index) {
        const size_t new_size = index + 1;
        const size_t delta_size = new_size - old_size;
        m_type_id_out.insert(m_type_id_out.end(), delta_size, -1);
        m_names_out.insert(m_names_out.end(), delta_size, std::string());
        m_out.insert(m_out.end(), delta_size, nullptr);
    }

    if (m_type_id_out[index] != -1) {
        throw std::runtime_error("output slot is already defined");
    }

    m_type_id_out[index] = type_id;
    m_names_out[index] = std::move(name);
}

void process_t::connect_in(size_t index, connection_t* connection) {
    if (m_in.size() <= index) {
        throw std::runtime_error("index is out of bounds");
    }

    if (m_in[index] != nullptr) {
        throw std::runtime_error("slot is already connected");
    }

    const int type_id = m_type_id_in[index];

    m_in[index] = connection;
    connection->to = this;
    connection->message_type_id = type_id;
    connection->message = malloc(TYPESYSTEM.sizeof_type(type_id));
}

void process_t::connect_out(size_t index, connection_t* connection) {
    if (m_out.size() <= index) {
        throw std::runtime_error("index is out of bounds");
    }

    if (m_out[index] != nullptr) {
        throw std::runtime_error("slot is already connected");
    }

    m_out[index] = connection;
    connection->from = this;
}

size_t process_t::size_in() {
    return m_in.size();
}

size_t process_t::size_out() {
    return m_out.size();
}

void process_t::initialize() {
    if (m_is_initialized) {
        return ;
    }

    m_init(this);
    m_is_initialized = true;

    for (auto* in_connection : m_in) {
        in_connection->from->initialize();
        in_connection->to->initialize();
    }

    for (auto* out_connection : m_out) {
        out_connection->from->initialize();
        out_connection->to->initialize();
    }
}

bool process_t::call() {
    return call(get_time());
}

bool process_t::call(time_type_t t_propagate) {
    if (!m_is_initialized) {
        throw std::runtime_error("cannot call uninitialized process");
    }

    if (!call_preamble(t_propagate)) {
        return false;
    }

    if (!call_impl()) {
        return false;
    }

    if (!call_postamble(t_propagate)) {
        return false;
    }

    return true;
}

bool process_t::call_preamble(time_type_t t_propagate) {
    if (t_propagate < m_t_propagate) {
        return false;
    }

    propagate(t_propagate);

    bool has_in = false;
    time_type_t t_most_recent_input;
    for (auto* in_connection : m_in) {
        if (!in_connection || !in_connection->from) {
            continue ;
        }

        has_in = true;
        if (in_connection->from->m_t_propagate == t_propagate) {
            if (in_connection->from->m_t_start < t_propagate) {
                return false;
            }
        }
        if (in_connection->from->m_t_success < in_connection->from->m_t_failure) {
            return false;
        }
        t_most_recent_input = max(t_most_recent_input, in_connection->from->m_t_success);
    }
    if (!has_in) {
        return true;
    }

    if (t_most_recent_input <= m_t_success) {
        return false;
    }

    bool has_out = false;
    time_type_t t_oldest_output = get_time();
    for (auto* out_connection : m_out) {
        if (!out_connection || !out_connection->to) {
            continue ;
        }

        has_out = true;
        t_oldest_output = min(t_oldest_output, out_connection->to->m_t_success);
    }
    if (!has_out) {
        return true;
    }

    if (t_most_recent_input <= t_oldest_output) {
        return false;
    }

    return true;
}

bool process_t::call_impl() {
    m_t_start = get_time();
    if (!m_call(this)) {
        m_t_failure = get_time();
        m_t_finish = m_t_failure;
        return false;
    }
    m_t_success = get_time();
    m_t_finish = m_t_success;

    return true;
}

bool process_t::call_postamble(time_type_t t_propagate) {
    // todo: use queue to incorporate looping constructs, otherwise stack blows up

    for (auto* out_connection : m_out) {
        if (!out_connection || !out_connection->to) {
            continue ;
        }

        if (out_connection->to->m_t_start < m_t_success) {
            (void) out_connection->to->call(t_propagate);
        }
    }

    return true;
}

void process_t::propagate(time_type_t t_propagate) {
    if (t_propagate <= m_t_propagate) {
        return ;
    }
    m_t_propagate = t_propagate;

    for (auto* out_connection : m_out) {
        if (!out_connection || !out_connection->to) {
            continue ;
        }

        out_connection->to->propagate(t_propagate);
    }
}
