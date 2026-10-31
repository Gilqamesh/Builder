#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <netinet/in.h>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <utility>

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

namespace m03gagbht7wqhtdg9hwdpmfn5o_download {

class http_server_t {
public:
    explicit http_server_t(std::string contents);
    ~http_server_t();

    http_server_t(const http_server_t&) = delete;
    http_server_t& operator=(const http_server_t&) = delete;

    const std::string& url() const;

private:
    void serve(std::stop_token stop_token) const;

    int m_socket;
    std::string m_contents;
    std::string m_url;
    std::jthread m_thread;
};

http_server_t::http_server_t(std::string contents):
    m_socket(::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)),
    m_contents(std::move(contents))
{
    if (m_socket < 0) {
        throw std::system_error(errno, std::generic_category(), "download test: create HTTP fixture socket");
    }
    try {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::bind(m_socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
            throw std::system_error(errno, std::generic_category(), "download test: bind HTTP fixture socket");
        }
        if (::listen(m_socket, 1) < 0) {
            throw std::system_error(errno, std::generic_category(), "download test: listen on HTTP fixture socket");
        }
        socklen_t address_size = sizeof(address);
        if (::getsockname(m_socket, reinterpret_cast<sockaddr*>(&address), &address_size) < 0) {
            throw std::system_error(errno, std::generic_category(), "download test: read HTTP fixture port");
        }
        m_url = std::format("http://127.0.0.1:{}/fixture", ntohs(address.sin_port));
        m_thread = std::jthread([this](std::stop_token stop_token) { serve(stop_token); });
    } catch (...) {
        ::close(m_socket);
        throw;
    }
}

http_server_t::~http_server_t() {
    m_thread.request_stop();
    ::shutdown(m_socket, SHUT_RDWR);
    m_thread.join();
    ::close(m_socket);
}

const std::string& http_server_t::url() const {
    return m_url;
}

void http_server_t::serve(std::stop_token stop_token) const {
    const auto response = std::format("HTTP/1.0 200 OK\r\nContent-Length: {}\r\nConnection: close\r\n\r\n", m_contents.size()) + m_contents;
    while (!stop_token.stop_requested()) {
        const int client_socket = ::accept4(m_socket, nullptr, nullptr, SOCK_CLOEXEC);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        std::string request;
        std::array<char, 1024> buffer;
        while (request.find("\r\n\r\n") == std::string::npos && request.size() < 8192) {
            const auto received = ::recv(client_socket, buffer.data(), buffer.size(), 0);
            if (received <= 0) {
                break;
            }
            request.append(buffer.data(), static_cast<std::size_t>(received));
        }
        if (request.find("\r\n\r\n") != std::string::npos) {
            std::size_t sent = 0;
            while (sent < response.size()) {
                const auto written = ::send(client_socket, response.data() + sent, response.size() - sent, MSG_NOSIGNAL);
                if (written <= 0) {
                    break;
                }
                sent += static_cast<std::size_t>(written);
            }
        }
        ::close(client_socket);
    }
}

void test_downloads(const std::filesystem::path& directory) {
    struct fixture_t {
        std::string contents;
        std::array<std::string, 2> digests;
    };
    const std::array fixtures{
        fixture_t{
            "abc",
            {"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532"}
        },
        fixture_t{
            "",
            {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"}
        },
        fixture_t{
            std::string("a\0b", 3),
            {"59b271ae1bbcb1d31d41929817f4b16fb439eb4f31520b5ad1d5ce98920a7138", "b476fd9cc202c304856e5b838839a737fbaaa96a2f44808f8c28c8cff135db22"}
        }
    };
    const std::array checksum_algorithms{checksum_algorithm_t::sha256, checksum_algorithm_t::sha3_256};
    for (std::size_t algorithm_index = 0; algorithm_index < checksum_algorithms.size(); ++algorithm_index) {
        const auto checksum_algorithm = checksum_algorithms[algorithm_index];
        const std::string suffix = checksum_algorithm == checksum_algorithm_t::sha256 ? ".sha256" : ".sha3-256";
        const filesystem_api::path_t output_path(directory / "download with spaces.bin");
        for (const auto& fixture : fixtures) {
            const http_server_t http_server(fixture.contents);
            const source_lock_t source_lock{.url = http_server.url(), .checksum = {checksum_algorithm, fixture.digests[algorithm_index]}};
            test::expect(std::equal_to<>(), fetch(source_lock, output_path), output_path);
            test::expect(std::equal_to<>(), read_file(output_path.to_native_path()), fixture.contents);
            test::expect(std::identity(), !filesystem_api::exists(output_path + suffix));
            filesystem_api::remove(output_path);
        }

        const http_server_t http_server("abc");
        const auto& expected = fixtures[0].digests[algorithm_index];
        const source_lock_t source_lock{.url = http_server.url(), .checksum = {checksum_algorithm, expected}};
        for (const auto& wrong_digest : {std::string(64, '0'), fixtures[0].digests[1 - algorithm_index]}) {
            test::expect_throws<std::runtime_error>([&] {
                fetch({.url = http_server.url(), .checksum = {checksum_algorithm, wrong_digest}}, output_path);
            });
            test::expect(std::identity(), !filesystem_api::exists(output_path));
            test::expect(std::identity(), !filesystem_api::exists(output_path + suffix));
        }

        write_file(output_path.to_native_path(), "preserve original");
        test::expect_throws<std::runtime_error>([&] { fetch(source_lock, output_path); });
        test::expect(std::equal_to<>(), read_file(output_path.to_native_path()), std::string("preserve original"));
        filesystem_api::remove(output_path);
        const auto checksum_path = output_path + suffix;
        write_file(checksum_path.to_native_path(), "preserve sidecar");
        test::expect_throws<std::runtime_error>([&] { fetch(source_lock, output_path); });
        test::expect(std::identity(), !filesystem_api::exists(output_path));
        test::expect(std::equal_to<>(), read_file(checksum_path.to_native_path()), std::string("preserve sidecar"));
        filesystem_api::remove(checksum_path);

        // The prefix file must not be verified in place of a filename containing a newline.
        const filesystem_api::path_t prefix_path(directory / "prefix");
        write_file(prefix_path.to_native_path(), "");
        for (const std::string filename : {"prefix\nignored", "back\\slash", "carriage\rreturn", "mixed\\n\n\r"}) {
            const filesystem_api::path_t unusual_path(directory / filename);
            test::expect_throws<std::runtime_error>([&] {
                fetch({.url = http_server.url(), .checksum = {checksum_algorithm, fixtures[1].digests[algorithm_index]}}, unusual_path);
            });
            test::expect(std::identity(), !filesystem_api::exists(unusual_path));
            test::expect(std::identity(), !filesystem_api::exists(unusual_path + suffix));
            test::expect(std::equal_to<>(), read_file(prefix_path.to_native_path()), std::string());
            test::expect(std::equal_to<>(), fetch(source_lock, unusual_path), unusual_path);
            test::expect(std::equal_to<>(), read_file(unusual_path.to_native_path()), std::string("abc"));
            test::expect(std::identity(), !filesystem_api::exists(unusual_path + suffix));
            filesystem_api::remove(unusual_path);
        }
    }
}

} // namespace m03gagbht7wqhtdg9hwdpmfn5o_download

static_assert(!std::is_default_constructible_v<api::checksum_t>);
static_assert(!std::is_default_constructible_v<api::source_lock_t>);

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const std::string expected_sha256 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
        const std::string expected_sha3 = "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532";
        const std::string unsupported_url = "unsupported://fixture";

        for (const auto checksum_algorithm : {api::checksum_algorithm_t::sha256, api::checksum_algorithm_t::sha3_256}) {
            const auto& expected = checksum_algorithm == api::checksum_algorithm_t::sha256 ? expected_sha256 : expected_sha3;
            const api::source_lock_t source_lock{
                .url = unsupported_url,
                .checksum = {checksum_algorithm, expected}
            };
            test::expect(std::equal_to<>(), source_lock.url, unsupported_url);
            test::expect(std::equal_to<>(), source_lock.checksum.algorithm(), checksum_algorithm);
            test::expect(std::equal_to<>(), source_lock.checksum.digest(), expected);
            test::expect(std::identity(), !std::format("{}", checksum_algorithm).empty());
            test::expect(std::identity(), std::format("{}", source_lock.checksum).find(expected) != std::string::npos);
            test::expect(std::identity(), std::format("{}", source_lock).find(expected) != std::string::npos);

            for (const std::string malformed : {"", "short", "3A985DA74FE225B2045C172D6BD390BD855F086E3E9D525B46BFE24511431532", "ga985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532", "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe245114315320"}) {
                test::expect_throws<std::runtime_error>([&] {
                    api::checksum_t(checksum_algorithm, malformed);
                });
            }

            const filesystem_api::path_t output(temporary_directory.path() / "downloads" / "value.bin");
            test::expect_throws<std::runtime_error>([&] {
                api::fetch(source_lock, output);
            });
            test::expect(std::identity(), !filesystem_api::exists(output));
            test::expect(std::identity(), !filesystem_api::exists(output + ".sha256"));
            test::expect(std::identity(), !filesystem_api::exists(output + ".sha3-256"));

            test::expect_throws<std::runtime_error>([&] {
                api::fetch({.url = "", .checksum = {checksum_algorithm, expected}}, output);
            });
            test::expect(std::identity(), !filesystem_api::exists(output));

            const filesystem_api::path_t preexisting(temporary_directory.path() / "preexisting.bin");
            write_file(preexisting.to_native_path(), "keep");
            test::expect_throws<std::runtime_error>([&] {
                api::fetch(source_lock, preexisting);
            });
            test::expect(std::equal_to<>(), read_file(preexisting.to_native_path()), std::string("keep"));
        }
        test::expect_throws<std::runtime_error>([&] {
            api::checksum_t(static_cast<api::checksum_algorithm_t>(-1), expected_sha256);
        });
        api::test_downloads(temporary_directory.path());
    });
}
