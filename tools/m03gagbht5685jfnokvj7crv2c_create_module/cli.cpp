#include <m03gagbht5685jfnokvj7crv2c_create_module/create_module.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <workspace> <name>\n";
        return 1;
    }

    try {
        const auto created = create_module::create(argv[1], argv[2]);
        std::cout << created.name << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
