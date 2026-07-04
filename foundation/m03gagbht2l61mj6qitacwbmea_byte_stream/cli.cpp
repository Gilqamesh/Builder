#include "byte_stream.h"

#include <exception>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            throw std::runtime_error(std::format("missing option argument"));
        }

        const auto option = std::string_view(argv[1]);
        if (option == "radix") {
            if (argc != 5) {
                throw std::runtime_error(std::format("usage: {} <text-radix> <to-radix> <text> <to-radix>", argv[0]));
            }

            const auto text_radix = std::stoul(argv[2]);
            const auto to_radix = std::stoul(argv[3]);
            const auto text = std::string_view(argv[4]);
            const auto byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix(text, text_radix);
            std::cout << std::format("{}", byte_stream.to_radix(to_radix)) << std::endl;
        } else {
            throw std::runtime_error(std::format("unknown option '{}'", option));
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
