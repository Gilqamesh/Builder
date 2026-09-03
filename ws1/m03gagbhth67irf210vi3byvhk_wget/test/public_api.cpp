#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhth67irf210vi3byvhk_wget/wget.h>

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

namespace api = m03gagbhth67irf210vi3byvhk_wget;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-wget-public-api-{}-{}",
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
        throw std::runtime_error("failed to create wget fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read wget fixture");
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

        const std::string unsupported_url = "unsupported://fixture";
        const filesystem_api::path_t output(
            temporary_directory.path() / "downloads" / "payload.bin"
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::download("", output);
        });
        test::expect(std::identity(), !filesystem_api::exists(output));
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::download(unsupported_url, output);
        });
        test::expect(std::identity(), !filesystem_api::exists(output));

        const filesystem_api::path_t preexisting(
            temporary_directory.path() / "preexisting.bin"
        );
        write_file(preexisting.to_native_path(), "keep");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::download(unsupported_url, preexisting);
        });
        test::expect(std::equal_to<>(), read_file(preexisting.to_native_path()), std::string("keep"));

        const filesystem_api::path_t failed_output(
            temporary_directory.path() / "failed" / "payload.bin"
        );
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::download(unsupported_url, failed_output);
        });
        test::expect(std::identity(), !filesystem_api::exists(failed_output));
    });
}
