#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbht9a02hx1qrv2qfgnp7_gzip/gzip.h>

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

namespace api = m03gagbht9a02hx1qrv2qfgnp7_gzip;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-gzip-public-api-{}-{}",
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
        throw std::runtime_error("failed to create gzip fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read gzip fixture");
    }
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const std::string contents = "builder gzip public API\nwith a second line\n";
        const filesystem_api::path_t input(temporary_directory.path() / "input.txt");
        const filesystem_api::path_t compressed(
            temporary_directory.path() / "compressed" / "input.txt.gz"
        );
        const filesystem_api::path_t restored(
            temporary_directory.path() / "restored" / "input.txt"
        );
        write_file(input.to_native_path(), contents);

        test::expect(std::equal_to<>(), api::gzip(input, compressed), compressed);
        test::expect(std::identity(), filesystem_api::is_regular_file(input));
        test::expect(std::equal_to<>(), read_file(input.to_native_path()), contents);
        test::expect(std::identity(), filesystem_api::is_regular_file(compressed));
        test::expect(std::identity(), !filesystem_api::exists(compressed + ".input"));
        test::expect(std::identity(), !filesystem_api::exists(compressed + ".input.gz"));

        test::expect(std::equal_to<>(), api::ungzip(compressed, restored), restored);
        test::expect(std::identity(), filesystem_api::is_regular_file(compressed));
        test::expect(std::equal_to<>(), read_file(restored.to_native_path()), contents);
        test::expect(std::identity(), !filesystem_api::exists(restored + ".input.gz"));
        test::expect(std::identity(), !filesystem_api::exists(restored + ".input"));

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::gzip(
                input,
                filesystem_api::path_t(temporary_directory.path() / "wrong.zip")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::gzip(input, compressed);
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::gzip(
                filesystem_api::path_t(temporary_directory.path() / "missing"),
                filesystem_api::path_t(temporary_directory.path() / "missing.gz")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::gzip(
                filesystem_api::path_t(temporary_directory.path()),
                filesystem_api::path_t(temporary_directory.path() / "directory.gz")
            );
        });

        const filesystem_api::path_t collision_output(
            temporary_directory.path() / "collision.gz"
        );
        write_file((collision_output + ".input").to_native_path(), "collision");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::gzip(input, collision_output);
        });
        test::expect(std::identity(), filesystem_api::is_regular_file(collision_output + ".input"));
        test::expect(std::identity(), !filesystem_api::exists(collision_output));

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::ungzip(
                input,
                filesystem_api::path_t(temporary_directory.path() / "wrong-input")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::ungzip(
                filesystem_api::path_t(temporary_directory.path() / "missing.gz"),
                filesystem_api::path_t(temporary_directory.path() / "missing-output")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::ungzip(compressed, restored);
        });

        const filesystem_api::path_t ungzip_collision(
            temporary_directory.path() / "ungzip-collision"
        );
        write_file((ungzip_collision + ".input.gz").to_native_path(), "collision");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::ungzip(compressed, ungzip_collision);
        });
        test::expect(std::identity(), filesystem_api::is_regular_file(ungzip_collision + ".input.gz"));

        const filesystem_api::path_t corrupt(temporary_directory.path() / "corrupt.gz");
        const filesystem_api::path_t corrupt_output(
            temporary_directory.path() / "corrupt-output" / "value"
        );
        write_file(corrupt.to_native_path(), "not a gzip stream");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::ungzip(corrupt, corrupt_output);
        });
        test::expect(std::identity(), !filesystem_api::exists(corrupt_output));
        test::expect(std::identity(), !filesystem_api::exists(corrupt_output + ".input.gz"));
        test::expect(std::identity(), !filesystem_api::exists(corrupt_output + ".input"));
    });
}
