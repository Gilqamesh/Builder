#include "apply.h"

#include <m03gagbhteldyu7ptbgnvootmb_tar/tar.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9tgv0q9lpsj42awj_lisp_tar {


using namespace m03gagbhteldyu7ptbgnvootmb_tar;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 2) {
        throw std::runtime_error(std::format("m03gagbhteldyu7ptbgnvootmb_tar::apply: expected 2 arguments, got {}", args.size()));
    }

    const auto input = path_arg(args[0]);
    const auto output = path_arg(args[1]);
    if (input.extension() == ".tar") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(untar(input, output));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(tar(input, output));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevc9tgv0q9lpsj42awj_lisp_tar::apply(args);
}

} // namespace m03gubnevc9tgv0q9lpsj42awj_lisp_tar
