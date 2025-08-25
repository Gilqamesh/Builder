#ifndef TYPESYSTEM_H
# define TYPESYSTEM_H

# include "libc.h"

class typesystem_t {
public:
    template <typename T>
    void register_type();

    template <typename From, typename To>
    void register_coercion(To* (*coercion_procedure)(From*));

    template <typename From, typename To>
    To* coerce(From* from);

private:
    template <typename T>
    void* type_addr();

    template <typename T>
    int type_id();

    int type_id(void* addr);

private:
    std::unordered_map<void*, int> m_addr_to_typeid;

    std::vector<std::vector<void*>> m_type_parents;
    std::vector<std::vector<double>> m_type_distance;
    std::vector<std::vector<size_t>> m_type_calls;
    std::vector<std::vector<void* (*)(void*)>> m_coercions;
};

template <typename T>
void typesystem_t::register_type() {
    const size_t old_size = m_registered_types.size();
    const void* addr = type_addr<T>();
    if (!m_registered_types.emplace(addr, old_size).second) {
        throw std::runtime_error("type already registered");
    }

    for (size_t i = 0; i < old_size; ++i) {
        m_type_parents[i].emplace_back(nullptr);
        m_type_distance.emplace_back(INFINITY);
        m_type_calls.emplace_back(0);
        m_coercions.emplace_back(nullptr);
    }
    m_type_parents.emplace_back(std::vector<void*>(old_size + 1, nullptr));
    m_type_distance.emplace_back(std::vector<double>(old_size + 1, INFINITY));
    m_type_calls.emplace_back(std::vector<size_t>(old_size + 1, 0));
    m_coercions.emplace_back(std::vector<void* (*)(void*)>(old_size + 1, nullptr));

    m_type_parents[old_size][old_size] = addr;
    m_type_distance[old_size][old_size] = 0.0;
}

template <typename From, typename To>
void typesystem_t::register_coercion(To* (*coercion_procedure)(From*)) {
    int id_from = type_id<From>();
    int id_to = type_id<To>();
    assert(id_from < m_coercions.size());
    assert(it_to < m_coercions[id_from].size());
    if (m_coercions[id_from][id_to] != nullptr) {
        throw std::runtime_error("coercion is already registered between types");
    }
    m_coercions[id_from][id_to] = coercion_procedure;
}

template <typename T>
void* typesystem_t::type_addr() {
    static const int addr;
    return &addr;
}

template <typename T>
int typesystem_t::type_id() {
    return type_id(type_addr<T>());
}

int typesystem_t::type_id(void* addr) {
    auto it = m_addr_to_typeid.find(addr);
    if (it == m_addr_to_typeid.end()) {
        throw std::runtime_error("type is not registered");
    }
    return it->second;
}

template <typename From, typename To>
To* typesystem_t::coerce(From* from) {
}

#endif // TYPESYSTEM_H
