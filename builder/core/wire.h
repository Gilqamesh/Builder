#ifndef WIRE_H
# define WIRE_H

# include "typesystem.h"

struct component_t;

struct wire_t {
public:
    wire_t();

    template <typename F>
    void add_action(F f);

    template <typename T>
    void write(T data);

    void write(wire_t& other);
    void write(wire_t* other);

    template <typename T>
    bool read(T& out);

private:
    component_t* m_from;
    component_t* m_to;
    int m_data_type_id;
    std::vector<unsigned char> m_data;
};

template <typename F>
void wire_t::add_action(F f) {
    f();
    m_actions.emplace_back(f);
}

template <typename T>
void wire_t::write(T data) {
    TYPESYSTEM.register_type<T>();
    const int type_id = TYPESYSTEM.type_id<T>();
    const size_t type_size = TYPESYSTEM.sizeof_type(type_id);
    m_data_type_id = type_id;
    m_data.resize(type_size);
    *reinterpret_cast<T*>(m_data.data()) = std::move(data);

    m_to->call();
}

template <typename T>
bool wire_t::read(T& out) {
    if (m_data_type_id == -1) {
        return false;
    }

    return TYPESYSTEM.coerce<T>(m_data.data(), m_data_type_id, &out);
}

struct component_t {
public:
    void set_name(std::string name);

    void add_component(component_t* component);

    void call();

    template <typename T>
    bool read(size_t index, T& result) {
        if (m_ins.size() <= index || !m_ins[index]) {
            return false;
        }

        return m_ins[index]->read<T>(result);
    }

    template <typename T>
    void write(size_t index, T data) {
        if (m_outs.size() <= index || !m_outs[index]) {
            return ;
        }

        m_outs[index]->write<T>(data);
    }

    void write(size_t in_index, size_t out_index) {
        if (m_ins.size() <= in_index || !m_ins[in_index]) {
            return ;
        }

        if (m_outs.size() <= out_index || !m_outs[out_index]) {
            return ;
        }

        m_outs[out_index]->write(m_ins[in_index]);
    }

protected:
    virtual void call_impl() {
    }

protected:
    std::string m_name;

    std::vector<component_t*> m_components;
    std::vector<wire_t*> m_inners;
    std::vector<wire_t*> m_ins;
    std::vector<wire_t*> m_outs;
};

#endif // WIRE_H
