#include "function_repository.h"

void function_repository_t::save(function_id_t id, function_ir_t ir) {
    m_functions.emplace(
        std::move(id),
        entry_t {
            .call = [](function_t* function, function_t::argument_index_t argument_index) {
                function->expand();
                function->send(argument_index);
            },
            .ir = std::move(ir)
        }
    );
}

void function_repository_t::save(function_id_t id, function_t::function_call_t call) {
    m_functions.emplace(
        std::move(id),
        entry_t {
            .call = std::move(call),
            .ir = function_ir_t{}
        }
    );
}
