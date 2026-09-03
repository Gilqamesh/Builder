#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>

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

namespace api = m03gagbht7wqhtdg9hwdpmfn5o_download;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-download-public-api-{}-{}",
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
        throw std::runtime_error("failed to create download fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read download fixture");
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

        const api::source_lock_t empty_lock;
        test::expect(std::identity(), empty_lock.url.empty());
        test::expect(std::identity(), empty_lock.sha256.empty());

        const std::string expected =
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad";
        const std::string unsupported_url = "unsupported://fixture";
        const api::source_lock_t lock {
            .url = unsupported_url,
            .sha256 = expected
        };
        test::expect(std::equal_to<>(), lock.url, unsupported_url);
        test::expect(std::equal_to<>(), lock.sha256, expected);

        const filesystem_api::path_t output(
            temporary_directory.path() / "downloads" / "value.bin"
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::fetch(lock, output);
        });
        test::expect(std::identity(), !filesystem_api::exists(output));
        test::expect(std::identity(), !filesystem_api::exists(output + ".sha256"));

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::fetch(
                api::source_lock_t { .url = "", .sha256 = expected },
                filesystem_api::path_t(temporary_directory.path() / "empty-url")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::fetch(
                api::source_lock_t { .url = "http://127.0.0.1/unused", .sha256 = "" },
                filesystem_api::path_t(temporary_directory.path() / "empty-hash")
            );
        });

        const filesystem_api::path_t preexisting(
            temporary_directory.path() / "preexisting.bin"
        );
        write_file(preexisting.to_native_path(), "keep");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::fetch(lock, preexisting);
        });
        test::expect(std::equal_to<>(), read_file(preexisting.to_native_path()), std::string("keep"));

        const filesystem_api::path_t wrong_hash_output(
            temporary_directory.path() / "wrong-hash.bin"
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::fetch(
                api::source_lock_t {
                    .url = unsupported_url,
                    .sha256 = std::string(64, '0')
                },
                wrong_hash_output
            );
        });
        test::expect(std::identity(), !filesystem_api::exists(wrong_hash_output));
        test::expect(std::identity(), !filesystem_api::exists(wrong_hash_output + ".sha256"));

        const filesystem_api::path_t malformed_hash_output(
            temporary_directory.path() / "malformed-hash.bin"
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::fetch(
                api::source_lock_t {
                    .url = unsupported_url,
                    .sha256 = "not-a-sha256"
                },
                malformed_hash_output
            );
        });
        test::expect(std::identity(), !filesystem_api::exists(malformed_hash_output));
    });
}
