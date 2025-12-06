#include <typesystem.h>
#include <cstring>
#include <format>
#include <x86intrin.h>

void typesystem_t::coerce(void* from, int id_from, void* to, int id_to) {
    if (id_from == id_to) {
        memcpy(to, from, sizeof_type(id_from));
        return ;
    }

    int id_subresult = m_type_parents[id_from][id_to];
    if (id_subresult == -1) {
        throw std::runtime_error(std::format("no coercion procedure found between types ({}, {})", id_from, id_to));
    }

    std::vector<unsigned char> subresult(sizeof_type(id_subresult));
    coerce(from, id_from, (void*) subresult.data(), id_subresult);

    auto coercion_procedure = m_coercions[id_subresult][id_to];
    if (!coercion_procedure) {
        throw std::runtime_error(std::format("no coercion procedure found between types ({}, {})", id_subresult, id_to));
    }

    size_t t_start = __rdtsc();
    coercion_procedure(subresult.data(), to);
    size_t t_end = __rdtsc();

    double& average_cost_kcy = m_type_distance[id_subresult][id_to];
    size_t& n_calls = m_type_calls[id_subresult][id_to];
    average_cost_kcy = ((average_cost_kcy * n_calls) + (double)(t_end - t_start) / 1000.0) / (double)(n_calls + 1);
    ++n_calls;
}

int typesystem_t::type_id(void* addr) {
    auto it = m_addr_to_typeid.find(addr);
    if (it == m_addr_to_typeid.end()) {
        throw std::runtime_error("type is not registered");
    }
    return it->second;
}

void typesystem_t::update_coercion_graph() {
    const size_t n = m_type_parents.size();
    for (size_t k = 0; k < n; ++k) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                if (m_type_distance[i][k] + m_type_distance[k][j] < m_type_distance[i][j]) {
                    m_type_distance[i][j] = m_type_distance[i][k] + m_type_distance[k][j];
                    m_type_parents[i][j] = m_type_parents[k][j];
                }
            }
        }
    }
}

void typesystem_t::update_coercion_graph(int id_from, int id_to) {
    // Floyd-Warshall
    const size_t n = m_type_parents.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (m_type_distance[i][id_from] + m_type_distance[id_from][id_to] + m_type_distance[id_to][j] < m_type_distance[i][j]) {
                m_type_distance[i][j] = m_type_distance[i][id_from] + m_type_distance[id_from][id_to] + m_type_distance[id_to][j];
                m_type_parents[i][j] = m_type_parents[id_from][j];
            }
        }
    }
}

size_t typesystem_t::sizeof_type(int type_id) {
    if ((int)m_type_size.size() <= type_id) {
        throw std::runtime_error("type id is out of bounds");
    }

    return m_type_size[type_id];
}
