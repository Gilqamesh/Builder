#include "uuid.h"

#include <exception>
#include <format>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            std::cout << std::format("{}", m03gagbhtft23yhjwpp881tfmc_uuid::uuid::generate(7)) << std::endl;
            return 0;
        }

        if (argc != 2) {
            std::cerr << "usage: " << argv[0] << " <uuid version>\n";
            return 1;
        }

        const auto version(argv[1]);
        std::cout << std::format("{}", m03gagbhtft23yhjwpp881tfmc_uuid::uuid::generate(std::stoi(version))) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
