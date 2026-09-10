#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevca4m5wzzlx42hxkjf_lisp_filesystem {


m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhsnusi43zogoacgj2ez_filesystem::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t filesystem_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("filesystem"),
        name
    );
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "current_path", filesystem_capability("current_path") },
        { "exists", filesystem_capability("exists") },
        { "is_directory", filesystem_capability("is_directory") },
        { "is_regular_file", filesystem_capability("is_regular_file") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__current_path(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "current_path");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhsnusi43zogoacgj2ez_filesystem::current_path());
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__exists(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "exists");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gagbhsnusi43zogoacgj2ez_filesystem::exists(path_arg(args[0])));
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_directory(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "is_directory");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(path_arg(args[0])));
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_regular_file(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "is_regular_file");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(path_arg(args[0])));
}

} // namespace m03gubnevca4m5wzzlx42hxkjf_lisp_filesystem
