#include "curl.h"

#include <iostream>
#include <format>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << std::format("usage: {} <url> <out_path>", argv[0]) << std::endl;
        return 1;
    }

    try {
        const std::string url = argv[1];
        const std::filesystem::path out_path = argv[2];
        const auto download_path = curl_t::download(url, out_path);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: '{}'", argv[0], e.what()) << std::endl;
    }

    return 0;
}
