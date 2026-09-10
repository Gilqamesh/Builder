#include "apply.h"

#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <m03gagbht3svcx3ign454lfup3_cmake/cmake.h>

#include <format>
#include <stdexcept>
#include <string>

namespace m03gubnevc9vhj6hczamfxvzd7_lisp_cmake {


using namespace m03gagbht3svcx3ign454lfup3_cmake;

static std::vector<std::string> string_args(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    std::vector<std::string> result;
    result.reserve(args.size());

    for (const auto& arg : args) {
        if (!m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(arg)) {
            throw std::runtime_error(std::format("m03gagbht3svcx3ign454lfup3_cmake::apply: expected string argument, got '{}'", arg.type_module));
        }
        result.push_back(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(arg));
    }

    return result;
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    std::vector<std::string> command_args {"/usr/bin/cmake"};
    const auto strings = string_args(args);
    command_args.insert(command_args.end(), strings.begin(), strings.end());
    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t { command_args });
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevc9vhj6hczamfxvzd7_lisp_cmake::apply(args);
}

} // namespace m03gubnevc9vhj6hczamfxvzd7_lisp_cmake
