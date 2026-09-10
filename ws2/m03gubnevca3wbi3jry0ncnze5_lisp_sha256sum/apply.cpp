#include "apply.h"

#include <m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>

#include <format>
#include <stdexcept>

namespace m03gubnevca3wbi3jry0ncnze5_lisp_sha256sum {


using namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 2) {
        throw std::runtime_error(std::format("m03gagbhtbusaqidrtw6lnugr4_sha256sum::apply: expected 2 arguments, got {}", args.size()));
    }

    verify(
        path_arg(args[0]),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[1])
    );
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevca3wbi3jry0ncnze5_lisp_sha256sum::apply(args);
}

} // namespace m03gubnevca3wbi3jry0ncnze5_lisp_sha256sum
