#include "api.h"

#include "executor.h"

namespace module_builder {

int build_module(const std::string& module_name) {
    context_t ctx = make_context_for_module(module_name);
    return execute_build(ctx);
}

}  // namespace module_builder

