#include "tar.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <tar-path> <output-dir>\n";
        return 1;
    }

    try {
        const auto input_file = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[1]);
        const auto install_path = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[2]);
        if (input_file.extension() == ".tar") {
            tar::untar(input_file, install_path);
        } else {
            tar::tar(input_file, install_path);
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: '{}'", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
