#include "sha256sum.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <path> <expected-sha256>\n";
        return 1;
    }

    try {
        sha256sum::verify(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[1]), argv[2]);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
