#include "download.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " <url> <sha256> <output-path>\n";
        return 1;
    }

    try {
        const auto output = m03gagbht7wqhtdg9hwdpmfn5o_download::fetch(
            m03gagbht7wqhtdg9hwdpmfn5o_download::source_lock_t {
                .url = argv[1],
                .sha256 = argv[2]
            },
            m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[3])
        );
        std::cout << std::format("{}", output) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
