#include <m03gagbht2l61mj6qitacwbmea_base36/base36.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

static std::uint8_t hex_value(char c) {
    if ('0' <= c && c <= '9') {
        return static_cast<std::uint8_t>(c - '0');
    }
    if ('a' <= c && c <= 'f') {
        return static_cast<std::uint8_t>(10 + c - 'a');
    }
    if ('A' <= c && c <= 'F') {
        return static_cast<std::uint8_t>(10 + c - 'A');
    }

    throw std::runtime_error(std::format("hex byte input contains non-hex character '{}'", c));
}

static std::vector<std::uint8_t> parse_hex_bytes(std::string_view input) {
    if (input.starts_with("0x") || input.starts_with("0X")) {
        input.remove_prefix(2);
    }

    if (input.empty()) {
        throw std::runtime_error("hex byte input must not be empty");
    }
    if (input.size() % 2 != 0) {
        throw std::runtime_error("hex byte input must contain an even number of hexadecimal digits");
    }

    std::vector<std::uint8_t> result;
    result.reserve(input.size() / 2);

    for (std::size_t i = 0; i < input.size(); i += 2) {
        result.push_back(static_cast<std::uint8_t>((hex_value(input[i]) << 4) | hex_value(input[i + 1])));
    }

    return result;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <hex-bytes>\n";
        return 1;
    }

    try {
        const auto bytes = parse_hex_bytes(argv[1]);
        std::cout << base36::encode(bytes) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
