#include "fixtures.h"

#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>

#include <array>
#include <format>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace m03gtrxnmqqa2t7zxpijo222n6_formatting {

noncopyable_t::noncopyable_t(int count) : count(count) {}

} // namespace m03gtrxnmqqa2t7zxpijo222n6_formatting

namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;
namespace formatting = m03gtrxnmqqa2t7zxpijo222n6_formatting;

namespace {

void require(bool condition) {
    if (!condition) {
        throw std::runtime_error("structural formatter contract failed");
    }
}

void check(std::string_view actual, std::string_view expected) {
    if (actual != expected) {
        throw std::runtime_error(std::format("structural output '{}' differs from '{}'", actual, expected));
    }
}

} // namespace

int main() {
    return test::run([] {
        const formatting::extent_t extent{128, 256};
        static_assert(std::formattable<const formatting::extent_t, char>);
        check(std::format("{}", extent), "extent_t{width=128, height=256}");
        check(std::format("{:}", extent), "extent_t{width=128, height=256}");
        check(std::format("{}", formatting::extended_extent_t{128, 256, 4}), "extended_extent_t{width=128, height=256, depth=4}");
        check(std::format("{}", formatting::empty_t{}), "empty_t{}");
        check(std::format("{}", formatting::state_t::alias), "ready");
        check(std::format("{}", formatting::state_t::done), "done");
        check(std::format("{}", static_cast<formatting::state_t>(-3)), "invalid(-3)");
        check(std::format("{}", formatting::wide_state_t::high), "high");
        check(std::format("{}", static_cast<formatting::wide_state_t>(0xfffffffffffffffeULL)), "invalid(18446744073709551614)");
        check(std::format("{}", formatting::record_t{extent, formatting::state_t::ready, "sample", {2, 3}}),
            "record_t{extent=extent_t{width=128, height=256}, state=ready, label=sample, counts=[2, 3]}");
        check(std::format("{}", formatting::derived_t{{4}, 5}), "derived_t{base_t{count=4}, extra=5}");
        formatting::diamond_t diamond;
        diamond.count = 6;
        check(std::format("{}", diamond), "diamond_t{left_t{base_t{count=6}}, right_t{base_t{count=6}}}");
        check(std::format("{}", formatting::bits_t{5, 1}), "bits_t{count=5, active=1}");
        check(std::format("{}", formatting::borrowed_t{extent}), "borrowed_t{extent=extent_t{width=128, height=256}}");
        check(std::format("{}", formatting::custom_record_t{{9}}), "custom_record_t{custom=custom:9}");
        check(std::format("{}", formatting::noncopyable_record_t{formatting::noncopyable_t{8}}), "noncopyable_record_t{noncopyable=noncopyable_t{count=8}}");
        check(std::format("{}", formatting::box_t<int>{3}), "box_t{item=3}");
        std::string appended = "prefix ";
        std::format_to(std::back_inserter(appended), "{} suffix", extent);
        check(appended, "prefix extent_t{width=128, height=256} suffix");
        std::array<char, 80> buffer{};
        const auto end = std::format_to(buffer.data(), "{}", extent);
        check(std::string_view(buffer.data(), end - buffer.data()), "extent_t{width=128, height=256}");
        const auto truncated = std::format_to_n(buffer.data(), 6, "{}", extent);
        check(std::string_view(buffer.data(), 6), "extent");
        require(truncated.size == std::formatted_size("{}", extent));
        require(extent.width == 128 && extent.height == 256);
        for (const auto specification : {"{:x}", "{:>40}", "{:.2f}"}) {
            test::expect_throws<std::format_error>([&] { (void)std::vformat(specification, std::make_format_args(extent)); });
        }
    });
}
