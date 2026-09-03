#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>

#include <functional>
#include <iostream>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>

namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;

int main() {
    return test::run([] {
        const test::assertion_error_t error("message");
        test::expect(std::equal_to<>(), std::string(error.what()), std::string("message"));

        test::expect_throws<test::assertion_error_t>([] {
            test::fail();
        });

        test::expect(std::identity(), true);
        test::expect_throws<test::assertion_error_t>([] {
            test::expect(std::identity(), false);
        });

        test::expect(std::equal_to<>(), 3, 3);
        test::expect(std::equal_to<>(), 3, 3L);
        test::expect_throws<test::assertion_error_t>([] {
            test::expect(std::equal_to<>(), 3, 4);
        });

        test::expect(std::not_equal_to<>(), 3, 4);
        test::expect_throws<test::assertion_error_t>([] {
            test::expect(std::not_equal_to<>(), 3, 3);
        });

        std::string equal_failure;
        try {
            test::expect(std::equal_to<>(), 3, 4);
        } catch (const test::assertion_error_t& e) {
            equal_failure = e.what();
        }
        test::expect(std::identity(), equal_failure.find("predicate failed for 3, 4") != std::string::npos);
        test::expect(std::identity(), 1 <= 2 && 2 <= 3);
        test::expect_throws<test::assertion_error_t>([] {
            test::expect(std::identity(), 1 <= 4 && 4 <= 3);
        });
        test::expect_at(std::source_location::current(), std::equal_to<>(), 5, 5);
        test::expect_throws<test::assertion_error_t>([] {
            test::expect_at(std::source_location::current(), std::equal_to<>(), 5, 6);
        });

        test::expect_no_throw([] {});
        test::expect_throws<test::assertion_error_t>([] {
            test::expect_no_throw([] {
                throw std::runtime_error("unexpected");
            });
        });

        test::expect_throws<std::runtime_error>([] {
            throw std::runtime_error("expected");
        });
        test::expect_throws<std::exception>([] {
            throw std::runtime_error("derived");
        });
        test::expect_throws<test::assertion_error_t>([] {
            test::expect_throws<std::runtime_error>([] {});
        });
        test::expect_throws<test::assertion_error_t>([] {
            test::expect_throws<std::invalid_argument>([] {
                throw std::runtime_error("wrong type");
            });
        });

        test::expect(std::equal_to<>(), test::run([] {}), 0);

        std::ostringstream captured_errors;
        auto* previous_buffer = std::cerr.rdbuf(captured_errors.rdbuf());
        const int standard_exception_result = test::run([] {
            throw std::runtime_error("nested failure");
        });
        const int unknown_exception_result = test::run([] {
            throw 42;
        });
        std::cerr.rdbuf(previous_buffer);

        test::expect(std::equal_to<>(), standard_exception_result, 1);
        test::expect(std::equal_to<>(), unknown_exception_result, 1);
        test::expect(std::identity(), captured_errors.str().find("nested failure") != std::string::npos
        );
        test::expect(std::identity(), captured_errors.str().find("unknown exception") != std::string::npos
        );
    });
}
