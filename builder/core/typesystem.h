#ifndef TYPESYSTEM_H
# define TYPESYSTEM_H

# include "libc.h"

class typesystem_t {
public:
    template <typename T>
    void register_type();

    template <typename From, typename To>
    void register_coercion(bool (*coercion_procedure)(From*, To*));

    template <typename From, typename To>
    bool coerce(From* from, To* to);

    template <typename To>
    bool coerce(void* from, int id_from, To* to);

    bool coerce(void* from, int id_from, void* to, int id_to);

    template <typename T>
    T& cast(void* data, int id_data);

    template <typename T>
    void* type_addr();

    template <typename T>
    int type_id();

    int type_id(void* addr);

    void update_coercion_graph();
    void update_coercion_graph(int id_from, int id_to);

    template <typename T>
    size_t sizeof_type();

    size_t sizeof_type(int type_id);

private:
    std::unordered_map<void*, int> m_addr_to_typeid;

    std::vector<std::vector<int>> m_type_parents;
    std::vector<std::vector<double>> m_type_distance;
    std::vector<std::vector<size_t>> m_type_calls;
    std::vector<std::vector<bool (*)(void*, void*)>> m_coercions;
    std::vector<size_t> m_type_size;
};

extern typesystem_t TYPESYSTEM;

template <typename T>
void typesystem_t::register_type() {
    int old_size = (int) m_addr_to_typeid.size();
    void* addr = type_addr<T>();
    if (!m_addr_to_typeid.emplace(addr, old_size).second) {
        return ;
    }

    for (int i = 0; i < old_size; ++i) {
        m_type_parents[i].emplace_back(-1);
        m_type_distance[i].emplace_back(INFINITY);
        m_type_calls[i].emplace_back(0);
        m_coercions[i].emplace_back(nullptr);
    }
    m_type_parents.emplace_back(std::vector<int>(old_size + 1, -1));
    m_type_distance.emplace_back(std::vector<double>(old_size + 1, INFINITY));
    m_type_calls.emplace_back(std::vector<size_t>(old_size + 1, 0));
    m_coercions.emplace_back(std::vector<bool (*)(void*, void*)>(old_size + 1, nullptr));

    m_type_parents[old_size][old_size] = old_size;
    m_type_distance[old_size][old_size] = 0.0;

    m_type_size.emplace_back(sizeof_type<T>());
}

template <typename From, typename To>
void typesystem_t::register_coercion(bool (*coercion_procedure)(From*, To*)) {
    int id_from = type_id<From>();
    int id_to = type_id<To>();
    assert(id_from < m_coercions.size());
    assert(id_to < m_coercions[id_from].size());
    if (m_coercions[id_from][id_to]) {
        throw std::runtime_error("coercion is already registered between types");
    }
    m_type_parents[id_from][id_to] = id_from;
    m_type_distance[id_from][id_to] = 0.0;
    m_type_calls[id_from][id_to] = 0;
    m_coercions[id_from][id_to] = coercion_procedure;
    update_coercion_graph(id_from, id_to);
}

template <typename From, typename To>
bool typesystem_t::coerce(From* from, To* to) {
    return coerce(from, type_id<From>(), to, type_id<To>());
}

template <typename To>
bool typesystem_t::coerce(void* from, int id_from, To* to) {
    return coerce(from, id_from, to, type_id<To>());
}

template <typename T>
T& typesystem_t::cast(void* data, int id_data) {
    if (type_id<T>() != id_data) {
        throw std::runtime_error("cast: unexpected type");
    }

    return *reinterpret_cast<T*>(data);
}

template <typename T>
void* typesystem_t::type_addr() {
    static int addr;
    return reinterpret_cast<void*>(&addr);
}

template <typename T>
int typesystem_t::type_id() {
    return type_id(type_addr<T>());
}

template <typename T>
size_t typesystem_t::sizeof_type() {
    return sizeof(T);
}

#endif // TYPESYSTEM_H
