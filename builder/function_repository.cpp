#include "function_repository.h"

void function_repository_t::save(function_id_t id, function_t::function_call_t call, function_ir_t ir) {
    m_functions.emplace(
        std::move(id),
        entry_t {
            .call = call,
            .ir = std::move(ir)
        }
    );
}

function_repository_t::entry_t function_repository_t::load(const function_id_t& id) {
    auto it = m_functions.find(id);
    if (it == m_functions.end()) {
        throw std::runtime_error("function not found in repository");
    }

    return it->second;
}
