#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9sgq8mdc1fl49t5u_lisp_binding {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "binding::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t binding_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("binding"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "key", binding_capability("key") },
        { "make", binding_capability("make") },
        { "value", binding_capability("value") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__make(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 2, "make");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "key", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0])) },
        { "value", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[1])) }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__key(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "key");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field(args[0], "key");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__value(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "value");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field(args[0], "value");
}

} // namespace m03gubnevc9sgq8mdc1fl49t5u_lisp_binding
