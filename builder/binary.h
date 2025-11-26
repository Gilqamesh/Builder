#ifndef NODE_BINARY_H
# define NODE_BINARY_H

# include "libc.h"

/**
 * Binary node representation.
 * Format of binary:
 *   node name <null-terminated string>
 *   creation time <uint64_t>
 *   [opcodes...]*
 * 
 * opcodes:
 *   CREATE_NODE:
 *       op <u8>,
 *       left <i16>, right <i16>, top <i16>, bottom <i16>,
 *       name <null-terminated string>,
 *       creation time <uint64_t>
 * 
 *   CONNECT_PORTS:
 *       op <u8>,
 *       from_node_index <u8>, from_node_port_index <u8>,
 *       to_node_index <u8>,   to_node_port_index <u8>
 * 
 *   (node_index == (uint8_t)-1 refers to the parent node)
*/
struct node_binary_t {
    enum class opcode_t : uint8_t {
        CREATE_NODE,
        CONNECT_PORTS
    };

    std::vector<uint8_t> bytes;

    template <typename T>
    void serialize(const T& value) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            size_t current_size = bytes.size();
            bytes.resize(current_size + sizeof(T));
            std::memcpy(static_cast<void*>(bytes.data()) + current_size, static_cast<void*>(&value), sizeof(T));
        } else {
            static_assert(
                requires(node_binary_t& a, const T& b) { T::serialize(a, b); },
                "Non-trivial types must implement: static void T::serialize(node_binary_t&, const T&)"
            );
            T::serialize(*this, value);
        }
    }

    template <typename T>
    void deserialize(T& value, size_t& pos) {
        if (bytes.size() < pos + sizeof(T)) {
            throw std::runtime_error(std::format("not enough bytes to deserialize type {} at position {}", typeid(T).name(), pos));
        }

        if constexpr (std::is_trivially_copyable_v<T>) {
            value = *reinterpret_cast<const T*>(bytes.data() + pos);
            pos += sizeof(T);
        } else {
            static_assert(
                requires(const node_binary_t& a, size_t& b, T& c) { T::deserialize(a, b, c); },
                "Non-trivial types must implement: static void T::deserialize(const node_binary_t&, size_t&, T&)"
            );
            T::deserialize(*this, pos, value);
        }
    }
};

#endif // NODE_BINARY_H
