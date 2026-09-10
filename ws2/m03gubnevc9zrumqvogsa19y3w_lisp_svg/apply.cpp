#include "apply.h"

#include <m03gagbhtd850j7g3aa1znukwq_svg/svg.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9zrumqvogsa19y3w_lisp_svg {


using namespace m03gagbhtd850j7g3aa1znukwq_svg;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 2) {
        throw std::runtime_error(std::format("m03gagbhtd850j7g3aa1znukwq_svg::apply: expected 2 arguments, got {}", args.size()));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(render_png(
        path_arg(args[0]),
        path_arg(args[1])
    ));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevc9zrumqvogsa19y3w_lisp_svg::apply(args);
}

} // namespace m03gubnevc9zrumqvogsa19y3w_lisp_svg
