#include <builder/tar/tar.h>

#include <iostream>
#include <format>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << std::format("usage: {} <dir to tar | tar file to untar> <install_path>", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto input_file = std::filesystem::path(argv[1]);
        const auto install_path = std::filesystem::path(argv[2]);
        if (input_file.extension() == ".tar") {
            tar_t::untar(input_file, install_path);
        } else {
            tar_t::tar(input_file, install_path);
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: '{}'", argv[0], e.what()) << std::endl;
    }

    return 0;
}
