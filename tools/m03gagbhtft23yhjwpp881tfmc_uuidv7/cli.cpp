#include <m03gagbht2l61mj6qitacwbmea_base36/base36.h>
#include <m03gagbhtft23yhjwpp881tfmc_uuidv7/uuidv7.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

static std::string uuid_string(std::span<const std::uint8_t> bytes) {
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";

    std::string result;
    result.reserve(36);

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            result.push_back('-');
        }

        result.push_back(HEX_DIGITS[bytes[i] >> 4]);
        result.push_back(HEX_DIGITS[bytes[i] & 0x0f]);
    }

    return result;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        try {
            std::cout << base36::encode(uuidv7::uuidv7::generate().bytes()) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
            return 1;
        }
        return 0;
    }

    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " [--base36|--uuid]\n";
        return 1;
    }

    try {
        const std::string_view option(argv[1]);
        if (option == "--base36") {
            std::cout << base36::encode(uuidv7::uuidv7::generate().bytes()) << std::endl;
        } else if (option == "--uuid") {
            std::cout << uuid_string(uuidv7::uuidv7::generate().bytes()) << std::endl;
        } else {
            std::cerr << "usage: " << argv[0] << " [--base36|--uuid]\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
