#include <function_call_repository.h>
#include <chrono>
#include <utility>

void function_call_repository_t::save(function_id_t id, function_t::function_call_t call) {
    m_function_calls.emplace(std::move(id), std::move(call));
}

function_t::function_call_t function_call_repository_t::load(const function_id_t& id) const {
    auto it = m_function_calls.find(id);
    if (it == m_function_calls.end()) {
        return nullptr;
    }

    return it->second;
}

