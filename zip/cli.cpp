#include <modules/builder/zip/zip.h>

#include <iostream>
#include <format>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << std::format("usage: {} <source dir to zip | zip file to unzip> <install_path>", argv[0]) << std::endl;
        return 1;
    }

    const auto input_file = std::filesystem::path(argv[1]);
    const auto install_path = std::filesystem::path(argv[2]);
    if (input_file.extension() == ".zip") {
        zip_t::unzip(input_file, install_path);
    } else {
        zip_t::zip(input_file, install_path);
    }

    return 0;
}
