#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhteldyu7ptbgnvootmb_tar/tar.h>

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

namespace api = m03gagbhteldyu7ptbgnvootmb_tar;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-tar-public-api-{}-{}",
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
        throw std::runtime_error("failed to create tar fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read tar fixture");
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
        const filesystem_api::path_t source(temporary_directory.path() / "source");
        std::filesystem::create_directories(source.to_native_path());
        write_file(source.to_native_path() / "root.txt", "root contents\n");
        write_file(source.to_native_path() / "nested" / "value.txt", "nested contents\n");

        const filesystem_api::path_t archive(
            temporary_directory.path() / "archives" / "source.tar"
        );
        test::expect(std::equal_to<>(), api::tar(source, archive), archive);
        test::expect(std::identity(), filesystem_api::is_regular_file(archive));

        const filesystem_api::path_t extracted(
            temporary_directory.path() / "extracted"
        );
        test::expect(std::equal_to<>(), api::untar(archive, extracted), extracted);
        test::expect(std::equal_to<>(), read_file(extracted.to_native_path() / "root.txt"),
            std::string("root contents\n")
        );
        test::expect(std::equal_to<>(), read_file(extracted.to_native_path() / "nested" / "value.txt"),
            std::string("nested contents\n")
        );

        const filesystem_api::path_t existing_install(
            temporary_directory.path() / "existing-install"
        );
        std::filesystem::create_directories(existing_install.to_native_path());
        write_file(existing_install.to_native_path() / "preexisting.txt", "keep\n");
        test::expect(std::equal_to<>(), api::untar(archive, existing_install), existing_install);
        test::expect(std::equal_to<>(), read_file(existing_install.to_native_path() / "preexisting.txt"),
            std::string("keep\n")
        );
        test::expect(std::identity(), filesystem_api::is_regular_file(
            existing_install / filesystem_api::relative_path_t("nested/value.txt")
        ));

        const filesystem_api::path_t inside_archive(
            source.to_native_path() / "inside.tar"
        );
        test::expect(std::equal_to<>(), api::tar(source, inside_archive), inside_archive);
        const filesystem_api::path_t inside_extracted(
            temporary_directory.path() / "inside-extracted"
        );
        api::untar(inside_archive, inside_extracted);
        test::expect(std::identity(), !filesystem_api::exists(
            inside_extracted / filesystem_api::relative_path_t("inside.tar")
        ));
        test::expect(std::identity(), filesystem_api::is_regular_file(
            inside_extracted / filesystem_api::relative_path_t("root.txt")
        ));

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::tar(
                source,
                filesystem_api::path_t(temporary_directory.path() / "wrong.zip")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::tar(source, archive);
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::tar(
                filesystem_api::path_t(temporary_directory.path() / "missing"),
                filesystem_api::path_t(temporary_directory.path() / "missing.tar")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::tar(
                filesystem_api::path_t(source.to_native_path() / "root.txt"),
                filesystem_api::path_t(temporary_directory.path() / "file.tar")
            );
        });

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::untar(
                filesystem_api::path_t(temporary_directory.path() / "wrong.zip"),
                filesystem_api::path_t(temporary_directory.path() / "wrong-output")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::untar(
                filesystem_api::path_t(temporary_directory.path() / "missing.tar"),
                filesystem_api::path_t(temporary_directory.path() / "missing-output")
            );
        });

        const filesystem_api::path_t install_file(
            temporary_directory.path() / "install-file"
        );
        write_file(install_file.to_native_path(), "not a directory");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::untar(archive, install_file);
        });

        const filesystem_api::path_t corrupt(temporary_directory.path() / "corrupt.tar");
        const filesystem_api::path_t corrupt_output(
            temporary_directory.path() / "corrupt-output"
        );
        write_file(corrupt.to_native_path(), "not a tar archive");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::untar(corrupt, corrupt_output);
        });
        test::expect(std::identity(), !filesystem_api::exists(corrupt_output));

        const filesystem_api::path_t existing_corrupt_output(
            temporary_directory.path() / "existing-corrupt-output"
        );
        std::filesystem::create_directory(existing_corrupt_output.to_native_path());
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::untar(corrupt, existing_corrupt_output);
        });
        test::expect(std::identity(), filesystem_api::is_directory(existing_corrupt_output));
    });
}
