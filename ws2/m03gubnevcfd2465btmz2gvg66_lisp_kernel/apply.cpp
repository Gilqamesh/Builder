#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>

namespace m03gubnevcfd2465btmz2gvg66_lisp_kernel {


static void require_count(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (!args.empty()) {
        throw std::runtime_error(std::format("kernel capability expected 0 arguments, got {}", args.size()));
    }
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_count(args);
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        {"workspace_root", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("kernel"), "workspace_root")},
        {"artifact_root", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("kernel"), "artifact_root")}
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__workspace_root(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_count(args);
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context().workspace_root);
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__artifact_root(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_count(args);
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context().artifact_root);
}


} // namespace m03gubnevcfd2465btmz2gvg66_lisp_kernel
