#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace api = m03gagbht2l61mj6qitacwbmea_byte_stream;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;

int main() {
    return test::run([] {
        using byte_stream_t = api::byte_stream_t;

        byte_stream_t empty;
        test::expect(std::identity(), empty.empty());
        test::expect(std::equal_to<>(), empty.size(), std::size_t(0));
        test::expect(std::identity(), empty.bytes().empty());
        test::expect(std::equal_to<>(), empty.to_radix(10), std::string("0"));
        test::expect(std::equal_to<>(), std::format("{}", empty), std::string("0x"));

        const std::array<std::byte, 3> source {
            std::byte { 0x01 },
            std::byte { 0x23 },
            std::byte { 0xab }
        };
        byte_stream_t from_span { std::span<const std::byte>(source) };
        test::expect(std::equal_to<>(), from_span.size(), source.size());
        test::expect(std::identity(), from_span.bytes()[0] == source[0]);
        test::expect(std::identity(), from_span.bytes()[2] == source[2]);

        byte_stream_t from_vector(std::vector<std::byte>(source.begin(), source.end()));
        test::expect(std::identity(), from_vector.bytes()[1] == source[1]);

        byte_stream_t copied = from_span;
        copied.bytes()[0] = std::byte { 0xff };
        test::expect(std::identity(), from_span.bytes()[0] == std::byte { 0x01 });
        test::expect(std::identity(), copied.bytes()[0] == std::byte { 0xff });

        byte_stream_t moved = std::move(copied);
        test::expect(std::identity(), moved.bytes()[0] == std::byte { 0xff });

        byte_stream_t copy_assigned;
        test::expect(std::identity(), &(copy_assigned = from_span) == &copy_assigned);
        test::expect(std::identity(), copy_assigned.bytes()[2] == std::byte { 0xab });
        test::expect(std::identity(), &(copy_assigned = copy_assigned) == &copy_assigned);
        test::expect(std::equal_to<>(), copy_assigned.size(), std::size_t(3));

        byte_stream_t move_assigned;
        test::expect(std::identity(), &(move_assigned = std::move(moved)) == &move_assigned);
        test::expect(std::identity(), move_assigned.bytes()[0] == std::byte { 0xff });

        byte_stream_t appended;
        appended.push_back(std::byte { 0x10 });
        const std::array<std::byte, 2> suffix { std::byte { 0x20 }, std::byte { 0x30 } };
        appended.append(std::span<const std::byte>(suffix));
        test::expect(std::equal_to<>(), appended.size(), std::size_t(3));
        test::expect(std::identity(), appended.bytes()[0] == std::byte { 0x10 });
        test::expect(std::identity(), appended.bytes()[2] == std::byte { 0x30 });

        byte_stream_t tail(std::vector<std::byte> { std::byte { 0x40 }, std::byte { 0x50 } });
        appended.append(std::move(tail));
        test::expect(std::equal_to<>(), appended.size(), std::size_t(5));
        test::expect(std::identity(), appended.bytes()[3] == std::byte { 0x40 });
        test::expect(std::identity(), appended.bytes()[4] == std::byte { 0x50 });

        byte_stream_t self_appended(std::vector<std::byte> { std::byte { 0x01 }, std::byte { 0x02 } });
        self_appended.append(self_appended.bytes());
        test::expect(std::equal_to<>(), self_appended.size(), std::size_t(4));
        test::expect(std::identity(), self_appended.bytes()[0] == std::byte { 0x01 });
        test::expect(std::identity(), self_appended.bytes()[1] == std::byte { 0x02 });
        test::expect(std::identity(), self_appended.bytes()[2] == std::byte { 0x01 });
        test::expect(std::identity(), self_appended.bytes()[3] == std::byte { 0x02 });

        const byte_stream_t const_appended = appended;
        const std::span<const std::byte> readable = const_appended.bytes();
        test::expect(std::equal_to<>(), readable.size(), std::size_t(5));

        appended.clear();
        test::expect(std::identity(), appended.empty());
        test::expect(std::equal_to<>(), appended.size(), std::size_t(0));

        test::expect(std::identity(), byte_stream_t::from_radix("", 10).empty());
        test::expect(std::identity(), byte_stream_t::from_radix("0", 10).empty());
        test::expect(std::identity(), byte_stream_t::from_radix("0000", 2).empty());
        test::expect(std::equal_to<>(), byte_stream_t::from_radix("255", 10).to_radix(16), std::string("ff"));
        test::expect(std::equal_to<>(), byte_stream_t::from_radix("ff", 16).to_radix(10), std::string("255"));
        test::expect(std::equal_to<>(), byte_stream_t::from_radix("101010", 2).to_radix(10), std::string("42"));
        test::expect(std::equal_to<>(), byte_stream_t::from_radix("z", 36).to_radix(10), std::string("35"));

        const std::string large_hex = "123456789abcdef00112233445566778899aabbccddeeff";
        const auto large = byte_stream_t::from_radix(large_hex, 16);
        test::expect(std::equal_to<>(), large.to_radix(16), large_hex);
        test::expect(std::equal_to<>(), byte_stream_t::from_radix(large.to_radix(36), 36).to_radix(16),
            large_hex
        );

        const byte_stream_t leading_zeroes(std::vector<std::byte> {
            std::byte { 0x00 }, std::byte { 0x00 }, std::byte { 0x2a }
        });
        test::expect(std::equal_to<>(), leading_zeroes.to_radix(10), std::string("42"));
        test::expect(std::equal_to<>(), std::format("{}", leading_zeroes), std::string("0x00002a"));

        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto value = byte_stream_t::from_radix("10", 1);
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto value = byte_stream_t::from_radix("10", 37);
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto value = byte_stream_t::from_radix("A", 16);
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto value = byte_stream_t::from_radix("2", 2);
        });
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] const auto value = from_span.to_radix(0);
        });
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] const auto value = from_span.to_radix(100);
        });

        const auto file_path = std::filesystem::temp_directory_path() / std::format(
            "byte-stream-public-api-{}",
            std::chrono::steady_clock::now().time_since_epoch().count()
        );
        from_span.to_file(file_path);
        const auto from_file = byte_stream_t::from_file(file_path);
        test::expect(std::equal_to<>(), from_file.size(), from_span.size());
        test::expect(std::identity(), std::equal(from_file.bytes().begin(), from_file.bytes().end(), from_span.bytes().begin()));
        test::expect(std::identity(), std::filesystem::remove(file_path));
        test::expect_throws<std::ios_base::failure>([&] {
            [[maybe_unused]] const auto missing = byte_stream_t::from_file(file_path);
        });
        test::expect_throws<std::ios_base::failure>([&] {
            from_span.to_file(std::filesystem::temp_directory_path());
        });
    });
}
