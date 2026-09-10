#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

namespace m03gubnevca2sul44vp7stg8em_lisp_process {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhsvr0m5w15urj0o291m_process::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t process_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("process"), name);
}

std::vector<std::string> process_args(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    std::vector<std::string> result;
    for (const auto& arg : m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_list(value)) {
        if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path(arg)) {
            result.push_back(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(arg).string());
        } else {
            result.push_back(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(arg));
        }
    }

    return result;
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "succeeds", process_capability("succeeds") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__succeeds(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "succeeds");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gagbhsvr0m5w15urj0o291m_process::create_and_wait(m03gagbhsvr0m5w15urj0o291m_process::command_t {
        process_args(args[0])
    }) == 0);
}

} // namespace m03gubnevca2sul44vp7stg8em_lisp_process
