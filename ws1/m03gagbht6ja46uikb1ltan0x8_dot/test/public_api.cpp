#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbht6ja46uikb1ltan0x8_dot/dot.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace api = m03gagbht6ja46uikb1ltan0x8_dot;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-dot-public-api-{}-{}",
            std::chrono::steady_clock::now().time_since_epoch().count(),
            reinterpret_cast<std::uintptr_t>(this)
        );
        std::filesystem::create_directory(m_path);
    }

    ~temporary_directory_t() {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    const std::filesystem::path& path() const {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

void write_file(const std::filesystem::path& path, std::string_view contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create DOT fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const filesystem_api::path_t dot_file(temporary_directory.path() / "graph.dot");
        const filesystem_api::path_t output(
            temporary_directory.path() / "rendered" / "graph.svg"
        );
        write_file(dot_file.to_native_path(), "digraph G { a -> b; b -> c; }\n");

        test::expect(std::equal_to<>(), api::render_svg(dot_file, output), output);
        test::expect(std::identity(), filesystem_api::is_regular_file(output));
        test::expect(std::identity(), read_file(output.to_native_path()).find("<svg") != std::string::npos);

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(
                dot_file,
                filesystem_api::path_t(temporary_directory.path() / "wrong.png")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(
                filesystem_api::path_t(temporary_directory.path() / "missing.dot"),
                filesystem_api::path_t(temporary_directory.path() / "missing.svg")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(
                filesystem_api::path_t(temporary_directory.path()),
                filesystem_api::path_t(temporary_directory.path() / "directory.svg")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(dot_file, output);
        });

        const filesystem_api::path_t invalid_dot(temporary_directory.path() / "invalid.dot");
        const filesystem_api::path_t invalid_output(temporary_directory.path() / "invalid.svg");
        write_file(invalid_dot.to_native_path(), "digraph G { this is not valid\n");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(invalid_dot, invalid_output);
        });
        test::expect(std::identity(), !filesystem_api::exists(invalid_output));
    });
}
