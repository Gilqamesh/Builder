#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevca3x6qmtha8ynavgi_lisp_signal_handler {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t signal_handler_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("signal_handler"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "termination_signals", signal_handler_capability("termination_signals") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__termination_signals(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "termination_signals");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::list_value({
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("SIGINT"),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("SIGTERM"),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("SIGHUP")
    });
}

} // namespace m03gubnevca3x6qmtha8ynavgi_lisp_signal_handler
