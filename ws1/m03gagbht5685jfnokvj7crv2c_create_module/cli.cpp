#include "create_module.h"

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: " << argv[0] << " <workspace> <friendly_name>\n";
            return 1;
        }

        const auto workspace = std::string_view(argv[1]);
        const auto friendly_name = std::string_view(argv[2]);

        const auto module_dir = m03gagbht5685jfnokvj7crv2c_create_module::create(workspace, friendly_name);
        std::cout << std::format("{}", module_dir) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
