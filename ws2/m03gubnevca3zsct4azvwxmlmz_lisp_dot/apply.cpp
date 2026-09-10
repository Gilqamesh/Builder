#include "apply.h"

#include <m03gagbht6ja46uikb1ltan0x8_dot/dot.h>

#include <format>
#include <stdexcept>

namespace m03gubnevca3zsct4azvwxmlmz_lisp_dot {


using namespace m03gagbht6ja46uikb1ltan0x8_dot;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 2) {
        throw std::runtime_error(std::format("m03gagbht6ja46uikb1ltan0x8_dot::apply: expected 2 arguments, got {}", args.size()));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(render_svg(
        path_arg(args[0]),
        path_arg(args[1])
    ));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevca3zsct4azvwxmlmz_lisp_dot::apply(args);
}

} // namespace m03gubnevca3zsct4azvwxmlmz_lisp_dot
