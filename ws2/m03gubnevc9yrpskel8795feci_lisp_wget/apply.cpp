#include "apply.h"

#include <m03gagbhth67irf210vi3byvhk_wget/wget.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9yrpskel8795feci_lisp_wget {


using namespace m03gagbhth67irf210vi3byvhk_wget;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 2) {
        throw std::runtime_error(std::format("m03gagbhth67irf210vi3byvhk_wget::apply: expected 2 arguments, got {}", args.size()));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(download(
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0]),
        path_arg(args[1])
    ));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevc9yrpskel8795feci_lisp_wget::apply(args);
}

} // namespace m03gubnevc9yrpskel8795feci_lisp_wget
