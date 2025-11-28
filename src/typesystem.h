#ifndef TYPESYSTEM_H
# define TYPESYSTEM_H

#include <bit>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

class typesystem_t {
public:
    struct reader_t {
        typesystem_t* self;
        void* from;
        int type_id_from;

        template <typename T>
        operator T() {
            return self->coerce<T>(from, type_id_from);
        }
    };

    struct coercion_t {
        using caller_t = void (*)(void*, void*, void*);

        caller_t caller = nullptr;
        void* context = nullptr;

        constexpr operator bool() const {
            return caller != nullptr;
        }

        void operator()(void* from, void* to) const {
            caller(from, to, context);
        }
    };

public:
    template <typename T>
    void register_type();

    template <typename From, typename To>
    void register_coercion(To (*coercion_procedure)(From));

    template <typename From>
    reader_t coerce(From& from);

    template <typename To>
    To coerce(void* from, int id_from);

    void coerce(void* from, int id_from, void* to, int id_to);

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
    template <typename From, typename To>
    static void coercion_bridge(void* from_raw, void* to_raw, void* context);

private:
    std::unordered_map<void*, int> m_addr_to_typeid;

    std::vector<std::vector<int>> m_type_parents;
    std::vector<std::vector<double>> m_type_distance;
    std::vector<std::vector<size_t>> m_type_calls;
    std::vector<std::vector<coercion_t>> m_coercions;
    std::vector<size_t> m_type_size;
};

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
        m_coercions[i].emplace_back(coercion_t{});
    }
    m_type_parents.emplace_back(std::vector<int>(old_size + 1, -1));
    m_type_distance.emplace_back(std::vector<double>(old_size + 1, INFINITY));
    m_type_calls.emplace_back(std::vector<size_t>(old_size + 1, 0));
    m_coercions.emplace_back(std::vector<coercion_t>(old_size + 1, coercion_t{}));

    m_type_parents[old_size][old_size] = old_size;
    m_type_distance[old_size][old_size] = 0.0;

    m_type_size.emplace_back(sizeof_type<T>());
}

template <typename From, typename To>
void typesystem_t::register_coercion(To (*coercion_procedure)(From)) {
    int id_from = type_id<From>();
    int id_to = type_id<To>();
    assert(static_cast<size_t>(id_from) < m_coercions.size());
    assert(static_cast<size_t>(id_to) < m_coercions[id_from].size());
    if (m_coercions[id_from][id_to]) {
        throw std::runtime_error("coercion is already registered between types");
    }
    m_type_parents[id_from][id_to] = id_from;
    m_type_distance[id_from][id_to] = 0.0;
    m_type_calls[id_from][id_to] = 0;
    m_coercions[id_from][id_to] = coercion_t {
        .caller = &coercion_bridge<From, To>,
        .context = std::bit_cast<void*>(coercion_procedure)
    };
    update_coercion_graph(id_from, id_to);
}

template <typename From>
typesystem_t::reader_t typesystem_t::coerce(From& from) {
    return reader_t {
        .self = this,
        .from = (void*) &from,
        .type_id_from = type_id<From>()
    };
}

template <typename To>
To typesystem_t::coerce(void* from, int id_from) {
    int id_to = type_id<To>();

    if constexpr (std::is_pointer_v<To>) {
        To result{};
        coerce(from, id_from, &result, id_to);
        return result;
    } else {
        std::vector<unsigned char> to(sizeof_type(id_to));
        coerce(from, id_from, to.data(), id_to);
        return *reinterpret_cast<To*>(to.data());
    }
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

template <typename From, typename To>
void typesystem_t::coercion_bridge(void* from_raw, void* to_raw, void* context) {
    auto procedure = std::bit_cast<To (*)(From)>(context);
    *reinterpret_cast<To*>(to_raw) = procedure(*reinterpret_cast<From*>(from_raw));
}

#endif // TYPESYSTEM_H
