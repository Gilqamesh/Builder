#include "test_framework.h"

#include <format>
#include <string>

namespace m03gn97n4iusbtl7uthb01wu9m_test_framework {

assertion_error_t::assertion_error_t(std::string_view message):
    std::runtime_error(std::string(message))
{
}

assertion_error_t::assertion_error_t(std::string_view message, const std::source_location& location):
    std::runtime_error(std::format(
        "{}:{}:{}: {} in {}",
        location.file_name(),
        location.line(),
        location.column(),
        message,
        location.function_name()
    ))
{
}

[[noreturn]] void fail(const std::source_location& location) {
    throw assertion_error_t("failure", location);
}

} // namespace m03gn97n4iusbtl7uthb01wu9m_test_framework
