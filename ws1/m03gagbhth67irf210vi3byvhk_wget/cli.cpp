#include "wget.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <url> <output-path>\n";
        return 1;
    }

    try {
        const auto output = m03gagbhth67irf210vi3byvhk_wget::download(argv[1], m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[2]));
        std::cout << std::format("{}", output) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
