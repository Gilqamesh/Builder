#include <m03gagbht2l61mj6qitacwbmea_base36/base36.h>

#include <algorithm>
#include <vector>

namespace base36 {

std::string encode(std::span<const std::uint8_t> input) {
    static constexpr char DIGITS[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    std::vector<std::uint8_t> bytes(input.begin(), input.end());

    std::size_t begin = 0;

    while (begin < bytes.size() && bytes[begin] == 0) {
        ++begin;
    }

    if (begin == bytes.size()) {
        return "0";
    }

    std::string result;

    while (begin < bytes.size()) {
        std::uint32_t remainder = 0;

        for (std::size_t i = begin; i < bytes.size(); ++i) {
            std::uint32_t value = (remainder << 8) | bytes[i];

            bytes[i] = static_cast<std::uint8_t>(value / 36);
            remainder = value % 36;
        }

        result.push_back(DIGITS[remainder]);

        while (begin < bytes.size() && bytes[begin] == 0) {
            ++begin;
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

} // namespace base36
