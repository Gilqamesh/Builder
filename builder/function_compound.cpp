#include "function_compound.h"

function_t function_compound_t::function(typesystem_t& typesystem, function_ir_t function_ir) {
    return function_t(
        typesystem,
        function_ir,
        [](function_t* function, function_t::argument_index_t argument_index) {
            function->expand();
            function->send(argument_index);
        }
    );
}
