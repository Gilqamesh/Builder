#include "fixtures.h"

#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>

#include <array>
#include <format>
#include <forward_list>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

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
        check(std::format("{}", extent), "extent_t {\n  width: 128,\n  height: 256\n}");
        check(std::format("{:}", extent), std::format("{}", extent));
        check(std::format("{:0}", extent), std::format("{}", extent));
        check(std::format("{:1}", extent), "extent_t { width: 128, height: 256 }");
        check(std::format("{:2}", extent), "extent_t { width: 128, height: 256 }");
        check(std::format("{:3}", extent), "extent_t { width: 128, height: 256 }");
        check(std::format("{:2}", formatting::extended_extent_t{128, 256, 4}), "extended_extent_t { width: 128, height: 256, depth: 4 }");
        check(std::format("{:2}", formatting::empty_t{}), "empty_t{}");
        check(std::format("{:2}", formatting::state_t::alias), "ready");
        check(std::format("{:2}", formatting::state_t::done), "done");
        check(std::format("{:2}", static_cast<formatting::state_t>(-3)), "invalid(-3)");
        check(std::format("{:2}", formatting::wide_state_t::high), "high");
        check(std::format("{:2}", static_cast<formatting::wide_state_t>(0xfffffffffffffffeULL)), "invalid(18446744073709551614)");
        check(std::format("{:2}", formatting::record_t{extent, formatting::state_t::ready, "sample", {2, 3}}),
            "record_t { extent: extent_t { width: 128, height: 256 }, state: ready, label: \"sample\", counts: [2, 3] }");
        check(std::format("{:2}", formatting::derived_t{{4}, 5}), "derived_t { base_t { count: 4 }, extra: 5 }");
        formatting::diamond_t diamond;
        diamond.count = 6;
        check(std::format("{:2}", diamond), "diamond_t { left_t { base_t { count: 6 } }, right_t { base_t { count: 6 } } }");
        check(std::format("{:2}", formatting::bits_t{5, 1}), "bits_t { count: 5, active: 1 }");
        check(std::format("{:2}", formatting::borrowed_t{extent}), "borrowed_t { extent: extent_t { width: 128, height: 256 } }");
        check(std::format("{:2}", formatting::custom_record_t{{9}}), "custom_record_t { custom: custom:9 }");
        check(std::format("{:2}", formatting::noncopyable_record_t{formatting::noncopyable_t{8}}), "noncopyable_record_t { noncopyable: noncopyable_t { count: 8 } }");
        check(std::format("{:2}", formatting::box_t<int>{3}), "box_t { item: 3 }");
        const formatting::record_t record{extent, formatting::state_t::ready, "sample", {2, 3}};
        check(std::format("{}", record),
            "record_t {\n  extent: extent_t {\n    width: 128,\n    height: 256\n  },\n"
            "  state: ready,\n  label: \"sample\",\n  counts: [2, 3]\n}");
        check(std::format("{:1}", record),
            "record_t {\n  extent: extent_t { width: 128, height: 256 },\n"
            "  state: ready,\n  label: \"sample\",\n  counts: [2, 3]\n}");
        check(std::format("{}", formatting::derived_t{{4}, 5}),
            "derived_t {\n  base_t {\n    count: 4\n  },\n  extra: 5\n}");
        check(std::format("{}", formatting::custom_record_t{{9}}), "custom_record_t {\n  custom: custom:9\n}");
        check(std::format("{:1}", formatting::custom_record_t{{9}}), "custom_record_t { custom: custom:9 }");
        check(std::format("{:3}", formatting::custom_record_t{{9}}), "custom_record_t { custom: custom:9 }");

        const formatting::box_t<std::array<int, 9>> counts{{0, 1, 2, 3, 4, 5, 6, 7, 8}};
        check(std::format("{}", counts),
            "box_t {\n  item: [\n    0,\n    1,\n    2,\n    3,\n    4,\n    5,\n    6,\n    7,\n    8\n  ]\n}");
        check(std::format("{:1}", counts), std::format("{}", counts));
        check(std::format("{:2}", counts), "box_t { item: [0, 1, 2, 3, 4, 5, 6, 7, 8] }");
        check(std::format("{:3}", counts), "box_t { item: [0, 1, 2, … (+6 items)] }");
        check(std::format("{:3}", formatting::box_t<std::vector<int>>{{}}), "box_t { item: [] }");
        check(std::format("{:3}", formatting::box_t<std::vector<int>>{{1, 2, 3}}), "box_t { item: [1, 2, 3] }");
        const formatting::box_t<std::array<formatting::extent_t, 1>> extents{{extent}};
        check(std::format("{}", extents), "box_t {\n  item: [\n    extent_t {\n      width: 128,\n      height: 256\n    }\n  ]\n}");
        check(std::format("{:1}", extents), "box_t {\n  item: [\n    extent_t { width: 128, height: 256 }\n  ]\n}");
        check(std::format("{:3}", formatting::box_t<std::forward_list<int>>{{1, 2, 3, 4, 5}}),
            "box_t { item: [1, 2, 3, … (+2 items)] }");
        check(std::format("{:3}", formatting::box_t<std::set<int>>{{5, 4, 3, 2, 1}}),
            "box_t { item: {1, 2, 3, … (+2 items)} }");
        check(std::format("{:3}", formatting::box_t<std::vector<bool>>{{true, false, true, false}}),
            "box_t { item: [true, false, true, … (+1 items)] }");
        check(std::format("{:3}", formatting::box_t<formatting::custom_range_t>{{{1, 2, 3, 4}}}),
            "box_t { item: custom-range:4 }");

        const formatting::box_t<double> fraction{3.141592653589793};
        check(std::format("{:2}", fraction), "box_t { item: 3.141592653589793 }");
        check(std::format("{:3}", fraction), "box_t { item: 3.142 }");
        check(std::format("{:3}", formatting::box_t<double>{-0.0}), "box_t { item: -0.000 }");
        check(std::format("{:3}", formatting::box_t<double>{std::numeric_limits<double>::infinity()}), "box_t { item: inf }");
        check(std::format("{:3}", formatting::box_t<double>{std::numeric_limits<double>::quiet_NaN()}), "box_t { item: nan }");
        check(std::format("{:3}", formatting::box_t<std::vector<double>>{{1.23456, 2.34567, 3.45678, 4.56789}}),
            "box_t { item: [1.235, 2.346, 3.457, … (+1 items)] }");
        check(std::format("{:3}", formatting::box_t<std::map<std::string, double>>{{{"pi", fraction.item}}}),
            "box_t { item: {\"pi\": 3.142} }");
        check(std::format("{:3}", formatting::box_t<std::tuple<double, formatting::extent_t>>{{fraction.item, extent}}),
            "box_t { item: (3.142, extent_t { width: 128, height: 256 }) }");

        check(std::format("{:2}", formatting::box_t<std::string>{"a\n\"b\"\\c"}),
            "box_t { item: \"a\\n\\\"b\\\"\\\\c\" }");
        check(std::format("{:2}", formatting::box_t<std::string>{std::string("a\0b", 3)}),
            "box_t { item: \"a\\u{0}b\" }");
        check(std::format("{:2}", formatting::box_t<char>{'\n'}), "box_t { item: '\\n' }");
        check(std::format("{:3}", formatting::box_t<std::string>{""}), "box_t { item: \"\" }");
        check(std::format("{:3}", formatting::box_t<std::string>{std::string(32, 'a')}),
            "box_t { item: \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\" }");
        check(std::format("{:3}", formatting::box_t<std::string>{std::string(33, 'a')}),
            "box_t { item: \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa…\" (length: 33) }");
        const std::string unicode = std::string(31, 'a') + "ő😀";
        check(std::format("{:3}", formatting::box_t<std::string_view>{unicode}),
            "box_t { item: \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaő…\" (length: 33) }");
        check(std::format("{:2}", formatting::box_t<std::string>{unicode}),
            "box_t { item: \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaő😀\" }");
        const std::string invalid_utf8 = std::string(31, 'a') + "\xf0\x9f";
        check(std::format("{:3}", formatting::box_t<std::string>{invalid_utf8}),
            "box_t { item: " + std::format("{:?}", std::string(31, 'a') + "\xf0" + "…") + " (length: 33) }");

        const formatting::summary_t summary{std::string(33, 'a'), fraction.item, {record, record, record, record}, true};
        check(std::format("{:3}", summary),
            "summary_t { label: \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa…\" (length: 33), fraction: 3.142, records: ["
            "record_t { extent: extent_t { width: 128, height: 256 }, state: ready, label: \"sample\", counts: [2, 3] }, "
            "record_t { extent: extent_t { width: 128, height: 256 }, state: ready, label: \"sample\", counts: [2, 3] }, "
            "record_t { extent: extent_t { width: 128, height: 256 }, state: ready, label: \"sample\", counts: [2, 3] }, "
            "… (+1 items)], enabled: true }");
        const formatting::box_t<std::vector<formatting::custom_t>> custom_records{{{7}, {8}, {9}, {10}}};
        check(std::format("{:3}", custom_records), "box_t { item: [custom:7, custom:8, custom:9, … (+1 items)] }");
        check(std::format("{:2}|{:0}|{:3}", extent, extent, extent),
            "extent_t { width: 128, height: 256 }|extent_t {\n  width: 128,\n  height: 256\n}|extent_t { width: 128, height: 256 }");
        require(summary.label.size() == 33 && summary.records.size() == 4 && summary.fraction == fraction.item);

        std::string appended = "prefix ";
        std::format_to(std::back_inserter(appended), "{:2} suffix", extent);
        check(appended, "prefix extent_t { width: 128, height: 256 } suffix");
        std::array<char, 80> buffer{};
        const auto end = std::format_to(buffer.data(), "{:2}", extent);
        check(std::string_view(buffer.data(), end - buffer.data()), "extent_t { width: 128, height: 256 }");
        const auto truncated = std::format_to_n(buffer.data(), 6, "{:2}", extent);
        check(std::string_view(buffer.data(), 6), "extent");
        require(truncated.size == std::formatted_size("{:2}", extent));
        require(extent.width == 128 && extent.height == 256);
        for (const auto specification : {"{:x}", "{:>40}", "{:.2f}", "{:4}", "{:-1}", "{:00}", "{:1x}", "{:2.3}"}) {
            test::expect_throws<std::format_error>([&] { (void)std::vformat(specification, std::make_format_args(extent)); });
        }
    });
}
