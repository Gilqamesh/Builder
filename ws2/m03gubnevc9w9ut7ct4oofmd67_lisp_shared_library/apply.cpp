#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>
#include <m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9w9ut7ct4oofmd67_lisp_shared_library {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhsx4j5z28bqkac3dhhh_shared_library::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t shared_library_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("shared_library"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "has_symbol", shared_library_capability("has_symbol") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__has_symbol(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 2, "has_symbol");

    try {
        m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t loader(
            path_arg(args[0]),
            m03gagbhsx4j5z28bqkac3dhhh_shared_library::lifetime_t::DTOR,
            m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_resolution_t::LAZY,
            m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_visibility_t::LOCAL
        );
        loader.resolve(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[1]).c_str());
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(true);
    } catch (const std::exception&) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(false);
    }
}

} // namespace m03gubnevc9w9ut7ct4oofmd67_lisp_shared_library
