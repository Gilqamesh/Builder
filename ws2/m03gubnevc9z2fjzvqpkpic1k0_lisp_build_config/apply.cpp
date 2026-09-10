#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9z2fjzvqpkpic1k0_lisp_build_config {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "build_config::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t build_config_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("build_config"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "default_phase_order", build_config_capability("default_phase_order") },
        { "library_types", build_config_capability("library_types") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__default_phase_order(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "default_phase_order");

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::list_value({
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("source"),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("interface"),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("library"),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("binary")
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__library_types(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "library_types");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "shared", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("shared") }
    });
}

} // namespace m03gubnevc9z2fjzvqpkpic1k0_lisp_build_config
