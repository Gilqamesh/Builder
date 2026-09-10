#ifndef M03GN97N4IUSBTL7UTHB01WU9M_TEST_FRAMEWORK_TEST_FRAMEWORK_H
# define M03GN97N4IUSBTL7UTHB01WU9M_TEST_FRAMEWORK_TEST_FRAMEWORK_H

# include <exception>
# include <format>
# include <iostream>
# include <iterator>
# include <source_location>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <typeinfo>
# include <utility>

namespace m03gn97n4iusbtl7uthb01wu9m_test_framework {

/**
 * @brief Carries a failed assertion's diagnostic message and optional source location.
 *
 * Both constructors copy the message into the exception; the location overload
 * includes the file, line, column, and function in what().
 */
class assertion_error_t : public std::runtime_error {
public:
    explicit assertion_error_t(std::string_view message);

    assertion_error_t(std::string_view message, const std::source_location& location);
};

/**
 * @brief Throws assertion_error_t at the supplied or calling source location.
 */
[[noreturn]] void fail(const std::source_location& location = std::source_location::current());

/**
 * @brief Requires predicate(value) to be true.
 *
 * Predicate comes first; value must support std::format("{}", value).
 * @see expect_at for borrowing, predicate requirements, and failure behavior.
 */
template <class predicate_t, class value_t>
void expect(const predicate_t& predicate, const value_t& value, const std::source_location& location = std::source_location::current());

/**
 * @brief Requires predicate(left, right) to be true.
 *
 * Predicate comes first; both values must support default std::format formatting.
 * @see expect_at for borrowing, predicate requirements, and failure behavior.
 */
template <class predicate_t, class left_t, class right_t>
void expect(const predicate_t& predicate, const left_t& left, const right_t& right, const std::source_location& location = std::source_location::current());

/**
 * @brief Requires predicate(first, second, third) to be true.
 *
 * Predicate comes first; all values must support default std::format formatting.
 * @see expect_at for borrowing, predicate requirements, and failure behavior.
 */
template <class predicate_t, class first_t, class second_t, class third_t>
void expect(const predicate_t& predicate, const first_t& first, const second_t& second, const third_t& third, const std::source_location& location = std::source_location::current());

/**
 * @brief Checks predicate(arguments...) and attributes a failure to an explicit location.
 *
 * Calls the const predicate once with const references to the arguments; its
 * result must be convertible to bool. Neither predicate nor arguments are
 * retained. Every argument needs a usable std::formatter for default formatting,
 * even when the predicate succeeds, because the failure diagnostic is compiled.
 * A false result throws assertion_error_t with the formatted arguments and
 * location. Exceptions from the predicate or diagnostic formatting propagate.
 */
template <class predicate_t, class... argument_ts>
void expect_at(const std::source_location& location, const predicate_t& predicate, const argument_ts&... arguments);

/**
 * @brief Requires fn to complete without throwing.
 *
 * Invokes fn once without retaining it. Any exception becomes assertion_error_t
 * at location, including what() for exceptions derived from std::exception.
 */
template <class fn_t>
void expect_no_throw(fn_t&& fn, const std::source_location& location = std::source_location::current());

/**
 * @brief Requires one invocation of fn to throw an exception catchable as const exception_t&.
 *
 * Derived exceptions match; the default accepts any std::exception. Returning
 * normally or throwing a different type raises assertion_error_t at location.
 * The callable is not retained.
 */
template <class exception_t = std::exception, class fn_t>
void expect_throws(fn_t&& fn, const std::source_location& location = std::source_location::current());

/**
 * @brief Runs a test body once and converts uncaught exceptions to a failing exit status.
 *
 * Returns 0 when fn returns normally and 1 when it throws. Writes "test failed: "
 * and what() to std::cerr for std::exception, or an unknown-exception diagnostic
 * for other thrown types. The callable's return value is ignored; no later checks
 * in the body execute after an uncaught failure. The callable is not retained.
 * If writing the diagnostic to std::cerr throws, that exception propagates.
 *
 * @code{.cpp}
 * #include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
 *
 * #include <functional>
 * #include <stdexcept>
 *
 * namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;
 *
 * int main() {
 *     return test::run([] {
 *         test::expect(std::equal_to<>(), 2 + 2, 4);
 *         test::expect(std::identity(), true);
 *         test::expect_throws<std::invalid_argument>([] {
 *             throw std::invalid_argument("invalid input");
 *         });
 *     });
 * }
 * @endcode
 */
template <class fn_t>
int run(fn_t&& fn);

} // namespace m03gn97n4iusbtl7uthb01wu9m_test_framework

namespace m03gn97n4iusbtl7uthb01wu9m_test_framework {

template <class value_t>
using format_value_t = std::decay_t<value_t>;

template <class value_t>
inline constexpr bool has_std_formatter_v = std::is_default_constructible_v<std::formatter<format_value_t<value_t>, char>>;

template <class value_t>
constexpr void require_std_formatter() {
    static_assert(has_std_formatter_v<value_t>, "test_framework requires compared values to have a std::formatter specialization");
}

template <class... argument_ts>
inline constexpr bool has_std_formatters_v = (has_std_formatter_v<argument_ts> && ...);

template <class... argument_ts>
constexpr void require_std_formatters() {
    (require_std_formatter<argument_ts>(), ...);
}

template <class predicate_t, class... argument_ts>
constexpr void require_predicate() {
    static_assert(
        std::is_invocable_r_v<bool, const predicate_t&, const argument_ts&...>,
        "test_framework requires predicate arguments to be invocable and convertible to bool"
    );
}

template <class... argument_ts>
std::string format_arguments(const argument_ts&... arguments) {
    std::string result;
    std::string_view separator;
    ((std::format_to(std::back_inserter(result), "{}{}", std::exchange(separator, ", "), arguments)), ...);
    return result;
}

template <class predicate_t, class... argument_ts>
void expect_at(const std::source_location& location, const predicate_t& predicate, const argument_ts&... arguments) {
    if constexpr (!std::is_invocable_r_v<bool, const predicate_t&, const argument_ts&...>) {
        require_predicate<predicate_t, argument_ts...>();
    } else if constexpr (!has_std_formatters_v<argument_ts...>) {
        require_std_formatters<argument_ts...>();
    } else {
        if (!predicate(arguments...)) {
            throw assertion_error_t(std::format("predicate failed for {}", format_arguments(arguments...)), location);
        }
    }
}

template <class predicate_t, class value_t>
void expect(const predicate_t& predicate, const value_t& value, const std::source_location& location) {
    expect_at(location, predicate, value);
}

template <class predicate_t, class left_t, class right_t>
void expect(const predicate_t& predicate, const left_t& left, const right_t& right, const std::source_location& location) {
    expect_at(location, predicate, left, right);
}

template <class predicate_t, class first_t, class second_t, class third_t>
void expect(const predicate_t& predicate, const first_t& first, const second_t& second, const third_t& third, const std::source_location& location) {
    expect_at(location, predicate, first, second, third);
}

template <class fn_t>
void expect_no_throw(fn_t&& fn, const std::source_location& location) {
    try {
        std::forward<fn_t>(fn)();
    } catch (const std::exception& e) {
        throw assertion_error_t(std::format("expected no exception, got exception: {}", e.what()), location);
    } catch (...) {
        throw assertion_error_t("expected no exception, got unknown exception", location);
    }
}

template <class exception_t, class fn_t>
void expect_throws(fn_t&& fn, const std::source_location& location) {
    if constexpr (std::is_same_v<std::remove_cv_t<exception_t>, std::exception>) {
        try {
            std::forward<fn_t>(fn)();
        } catch (const std::exception&) {
            return ;
        } catch (...) {
            throw assertion_error_t(std::format("expected exception type {}, got unknown exception", typeid(exception_t).name()), location);
        }
    } else {
        try {
            std::forward<fn_t>(fn)();
        } catch (const exception_t&) {
            return ;
        } catch (const std::exception& e) {
            throw assertion_error_t(std::format("expected exception type {}, got exception: {}", typeid(exception_t).name(), e.what()), location);
        } catch (...) {
            throw assertion_error_t(std::format("expected exception type {}, got unknown exception", typeid(exception_t).name()), location);
        }
    }

    throw assertion_error_t(std::format("expected exception type {}, got no exception", typeid(exception_t).name()), location);
}

template <class fn_t>
int run(fn_t&& fn) {
    try {
        std::forward<fn_t>(fn)();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "test failed: " << e.what() << '\n';
    } catch (...) {
        std::cerr << "test failed: unknown exception\n";
    }

    return 1;
}

} // namespace m03gn97n4iusbtl7uthb01wu9m_test_framework

#endif // M03GN97N4IUSBTL7UTHB01WU9M_TEST_FRAMEWORK_TEST_FRAMEWORK_H
