#include "apply.h"

#include <m03gagbhth67irf210vi3byvhk_wget/wget.h>
#include <string_view>

#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>

#include <format>
#include <stdexcept>

namespace m03gubnevca2qkfbkyyx9ab739_lisp_download {


using namespace m03gagbht7wqhtdg9hwdpmfn5o_download;

static void validate_url(const std::string& url) {
    if (url.empty()) {
        throw std::runtime_error("m03gagbht7wqhtdg9hwdpmfn5o_download::fetch: URL must not be empty");
    }
}

static std::string output_filename_from_url(const std::string& url) {
    validate_url(url);

    std::string_view view(url);
    if (const auto fragment = view.find('#'); fragment != std::string_view::npos) {
        view = view.substr(0, fragment);
    }
    if (const auto query = view.find('?'); query != std::string_view::npos) {
        view = view.substr(0, query);
    }

    while (!view.empty() && view.back() == '/') {
        view.remove_suffix(1);
    }

    const auto slash = view.find_last_of('/');
    const auto filename = slash == std::string_view::npos
        ? view
        : view.substr(slash + 1);
    if (filename.empty() || filename == "." || filename == "..") {
        throw std::runtime_error(std::format("m03gagbht7wqhtdg9hwdpmfn5o_download::fetch: could not derive output filename from URL '{}'", url));
    }

    return std::string(filename);
}



static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() < 1 || 3 < args.size()) {
        throw std::runtime_error(std::format("m03gagbht7wqhtdg9hwdpmfn5o_download::apply: expected 1, 2, or 3 arguments, got {}", args.size()));
    }

    if (args.size() == 1) {
        const auto url = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0]);
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhth67irf210vi3byvhk_wget::download(url, m03gagbhsnusi43zogoacgj2ez_filesystem::current_path() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(output_filename_from_url(url))));
    }

    if (args.size() == 2) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhth67irf210vi3byvhk_wget::download(
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0]),
            path_arg(args[1])
        ));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(fetch(
        source_lock_t {
            .url = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0]),
            .sha256 = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[1])
        },
        path_arg(args[2])
    ));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevca2qkfbkyyx9ab739_lisp_download::apply(args);
}

} // namespace m03gubnevca2qkfbkyyx9ab739_lisp_download
