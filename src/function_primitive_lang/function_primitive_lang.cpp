#include <function_primitive_lang.h>
#include <chrono>
#include <utility>

function_t* function_primitive_lang_t::function(typesystem_t& typesystem, std::string ns, std::string name, function_t::function_call_t function_call) {
    return new function_t(
        typesystem,
        function_ir_t {
            .function_id = function_id_t {
                .ns = std::move(ns),
                .name = std::move(name),
                .creation_time = std::chrono::system_clock::now()
            },
            .left = {},
            .right = {},
            .top = {},
            .bottom = {},
            .children = {},
            .connections = {}
        },
        std::move(function_call)
    );
}

