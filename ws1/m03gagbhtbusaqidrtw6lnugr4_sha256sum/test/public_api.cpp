#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace api = m03gagbhtbusaqidrtw6lnugr4_sha256sum;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-sha256sum-public-api-{}-{}",
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
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create SHA-256 fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const filesystem_api::path_t file(temporary_directory.path() / "value.bin");
        const std::string expected =
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad";
        write_file(file.to_native_path(), "abc");

        test::expect_no_throw([&] { api::verify(file, expected); });
        test::expect(std::identity(), filesystem_api::is_regular_file(file));
        test::expect(std::identity(), !filesystem_api::exists(file + ".sha256"));

        test::expect_throws<std::runtime_error>([&] {
            api::verify(file, "short");
        });
        test::expect_throws<std::runtime_error>([&] {
            api::verify(
                file,
                "BA7816BF8F01CFEA414140DE5DAE2223"
                "B00361A396177A9CB410FF61F20015AD"
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            api::verify(
                file,
                "ga7816bf8f01cfea414140de5dae2223"
                "b00361a396177a9cb410ff61f20015ad"
            );
        });

        test::expect_throws<std::runtime_error>([&] {
            api::verify(
                filesystem_api::path_t(temporary_directory.path() / "missing"),
                expected
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            api::verify(filesystem_api::path_t(temporary_directory.path()), expected);
        });

        const auto checksum_path = file + ".sha256";
        write_file(checksum_path.to_native_path(), "preexisting");
        test::expect_throws<std::runtime_error>([&] {
            api::verify(file, expected);
        });
        test::expect(std::identity(), filesystem_api::is_regular_file(checksum_path));
        filesystem_api::remove(checksum_path);

        const std::string wrong =
            "00000000000000000000000000000000"
            "00000000000000000000000000000000";
        test::expect_throws<std::runtime_error>([&] {
            api::verify(file, wrong);
        });
        test::expect(std::identity(), filesystem_api::is_regular_file(file));
        test::expect(std::identity(), !filesystem_api::exists(checksum_path));
    });
}
