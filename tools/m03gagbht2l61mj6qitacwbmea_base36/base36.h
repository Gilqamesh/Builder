#ifndef M03GAGBHT2L61MJ6QITACWBMEA_BASE36_BASE36_H
# define M03GAGBHT2L61MJ6QITACWBMEA_BASE36_BASE36_H

# include <cstdint>
# include <span>
# include <string>

namespace base36 {

/**
 * Encodes bytes as a big-endian unsigned integer using lowercase base36.
 *
 * Leading zero bytes are ignored. Empty input and all-zero input return "0".
 */
std::string encode(std::span<const std::uint8_t> bytes);

} // namespace base36

#endif // M03GAGBHT2L61MJ6QITACWBMEA_BASE36_BASE36_H
