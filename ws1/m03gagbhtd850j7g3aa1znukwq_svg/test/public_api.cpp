#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhtd850j7g3aa1znukwq_svg/svg.h>

#include <functional>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace api = m03gagbhtd850j7g3aa1znukwq_svg;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-svg-public-api-{}-{}",
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
        throw std::runtime_error("failed to create SVG fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::array<unsigned char, 8> read_signature(const std::filesystem::path& path) {
    std::array<unsigned char, 8> result {};
    std::ifstream input(path, std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(result.data()), result.size())) {
        throw std::runtime_error("failed to read PNG signature");
    }
    return result;
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const filesystem_api::path_t svg_file(temporary_directory.path() / "image.svg");
        const filesystem_api::path_t output(
            temporary_directory.path() / "rendered" / "image.png"
        );
        write_file(
            svg_file.to_native_path(),
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"6\">"
            "<rect width=\"8\" height=\"6\" fill=\"red\"/>"
            "</svg>\n"
        );

        test::expect(std::equal_to<>(), api::render_png(svg_file, output), output);
        test::expect(std::identity(), filesystem_api::is_regular_file(output));
        test::expect(std::identity(), read_signature(output.to_native_path()) == std::array<unsigned char, 8> { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a }
        );

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_png(
                filesystem_api::path_t(temporary_directory.path() / "wrong.txt"),
                filesystem_api::path_t(temporary_directory.path() / "wrong.png")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_png(
                svg_file,
                filesystem_api::path_t(temporary_directory.path() / "wrong.jpg")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_png(
                filesystem_api::path_t(temporary_directory.path() / "missing.svg"),
                filesystem_api::path_t(temporary_directory.path() / "missing.png")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_png(svg_file, output);
        });

        const filesystem_api::path_t malformed(temporary_directory.path() / "malformed.svg");
        const filesystem_api::path_t malformed_output(temporary_directory.path() / "malformed.png");
        write_file(malformed.to_native_path(), "<svg><not-closed>\n");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_png(malformed, malformed_output);
        });
        test::expect(std::identity(), !filesystem_api::exists(malformed_output));
    });
}
