#include <function_repository.h>
#include <chrono>
#include <stdexcept>

void function_repository_t::save(function_ir_t ir, function_t::function_call_t call) {
    m_functions.emplace(
        ir.function_id,
        entry_t {
            .call = call,
            .ir = ir
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

