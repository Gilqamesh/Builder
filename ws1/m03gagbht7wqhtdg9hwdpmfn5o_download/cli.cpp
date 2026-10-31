#include "download.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <exception>
#include <format>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    const bool use_sha3 = argc == 5 && std::string_view(argv[1]) == "--sha3-256";
    if (argc != 4 && !use_sha3) {
        std::cerr << "usage: " << argv[0] << " [--sha3-256] <url> <checksum> <output-path>\n";
        return 1;
    }

    try {
        const auto first_argument = use_sha3 ? 2 : 1;
        const auto checksum_algorithm = use_sha3
            ? m03gagbht7wqhtdg9hwdpmfn5o_download::checksum_algorithm_t::sha3_256
            : m03gagbht7wqhtdg9hwdpmfn5o_download::checksum_algorithm_t::sha256;
        const m03gagbht7wqhtdg9hwdpmfn5o_download::source_lock_t source_lock{
            .url = argv[first_argument],
            .checksum = {checksum_algorithm, argv[first_argument + 1]}
        };
        const auto output = m03gagbht7wqhtdg9hwdpmfn5o_download::fetch(
            source_lock,
            m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[first_argument + 2])
        );
        std::cout << std::format("{}", output) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
