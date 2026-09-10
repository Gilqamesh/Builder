#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevca3ihozdheg61t9c1_lisp_compiler {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t compiler_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("compiler"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "cc_path", compiler_capability("cc_path") },
        { "cxx_path", compiler_capability("cxx_path") },
        { "tool_paths", compiler_capability("tool_paths") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__cc_path(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "cc_path");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::cc_compiler());
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__cxx_path(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "cxx_path");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::cxx_compiler());
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__tool_paths(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "tool_paths");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "cc", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::cc_compiler()) },
        { "cxx", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::cxx_compiler()) }
    });
}

} // namespace m03gubnevca3ihozdheg61t9c1_lisp_compiler
