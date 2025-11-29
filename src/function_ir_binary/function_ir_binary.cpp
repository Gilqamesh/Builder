#include <function_ir_binary.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

enum class opcode_t : uint8_t {
    CREATE_FUNCTION,
    CONNECT_ARGUMENTS
};

static std::chrono::system_clock::time_point deserialize_creation_time(uint64_t serialized_creation_time) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(serialized_creation_time));
}

static uint64_t serialize_creation_time(const std::chrono::system_clock::time_point& creation_time) {
    return std::chrono::duration_cast<std::chrono::seconds>(creation_time.time_since_epoch()).count();
}

function_ir_binary_t::function_ir_binary_t(std::vector<uint8_t> bytes):
    m_bytes(std::move(bytes))
{
}

function_ir_binary_t::function_ir_binary_t(const function_ir_t& ir) {
    serialize_function_id(ir.function_id);

    for (const auto& child : ir.children) {
        m_bytes.emplace_back(static_cast<uint8_t>(opcode_t::CREATE_FUNCTION));

        serialize_function_id(child.function_id);

        const int16_t left = (int16_t) child.left;
        m_bytes.emplace_back((uint8_t) (left >> 8));
        m_bytes.emplace_back((uint8_t) (left & 0xFF));
        const int16_t right = (int16_t) child.right;
        m_bytes.emplace_back((uint8_t) (right >> 8));
        m_bytes.emplace_back((uint8_t) (right & 0xFF));
        const int16_t top = (int16_t) child.top;
        m_bytes.emplace_back((uint8_t) (top >> 8));
        m_bytes.emplace_back((uint8_t) (top & 0xFF));
        const int16_t bottom = (int16_t) child.bottom;
        m_bytes.emplace_back((uint8_t) (bottom >> 8));
        m_bytes.emplace_back((uint8_t) (bottom & 0xFF));
    }

    for (const auto& connection : ir.connections) {
        m_bytes.emplace_back(static_cast<uint8_t>(opcode_t::CONNECT_ARGUMENTS));

        assert(connection.from_function_index <= UINT8_MAX);
        m_bytes.emplace_back((uint8_t) connection.from_function_index);
        m_bytes.emplace_back(connection.from_argument_index);
        assert(connection.to_function_index <= UINT8_MAX);
        m_bytes.emplace_back((uint8_t) connection.to_function_index);
        m_bytes.emplace_back(connection.to_argument_index);
    }
}

const std::vector<uint8_t>& function_ir_binary_t::bytes() const {
    return m_bytes;
}

function_ir_t function_ir_binary_t::function_ir() const {
    function_ir_t result;

    size_t offset = 0;
    result.function_id = deserialize_function_id(offset);
    while (offset < m_bytes.size()) {
        opcode_t op = static_cast<opcode_t>(m_bytes[offset++]);
        switch (op) {
            case opcode_t::CREATE_FUNCTION: {
                if (m_bytes.size() < offset + 8) {
                    throw std::runtime_error("failed to deserialize function_ir: unexpected end of data while reading CREATE_FUNCTION");
                }

                function_ir_t::child_t child;

                child.function_id = deserialize_function_id(offset);
                child.left = ((int16_t) m_bytes[offset] << 8) | (int16_t) m_bytes[offset + 1];
                offset += 2;
                child.right = ((int16_t) m_bytes[offset] << 8) | (int16_t) m_bytes[offset + 1];
                offset += 2;
                child.top = ((int16_t) m_bytes[offset] << 8) | (int16_t) m_bytes[offset + 1];
                offset += 2;
                child.bottom = ((int16_t) m_bytes[offset] << 8) | (int16_t) m_bytes[offset + 1];
                offset += 2;

                result.children.emplace_back(std::move(child));
            } break;
            case opcode_t::CONNECT_ARGUMENTS: {
                if (m_bytes.size() < offset + 4) {
                    throw std::runtime_error("failed to deserialize function_ir: unexpected end of data while reading CONNECT_ARGUMENTS");
                }

                function_ir_t::connection_info_t connection;

                connection.from_function_index = m_bytes[offset];
                ++offset;
                connection.from_argument_index = m_bytes[offset];
                ++offset;
                connection.to_function_index = m_bytes[offset];
                ++offset;
                connection.to_argument_index = m_bytes[offset];
                ++offset;

                result.connections.emplace_back(std::move(connection));
            } break;
            default: {
                throw std::runtime_error(std::format("failed to deserialize function_ir: unknown opcode {}", static_cast<uint8_t>(op)));
            } break ;
        }
    }

    return result;
}

void function_ir_binary_t::serialize_function_id(const function_id_t& function_id) {
    for (uint8_t c : function_id.ns) {
        m_bytes.emplace_back(c);
    }
    m_bytes.emplace_back(0);

    for (uint8_t c : function_id.name) {
        m_bytes.emplace_back(c);
    }
    m_bytes.emplace_back(0);

    uint64_t creation_time_serialized = serialize_creation_time(function_id.creation_time);
    for (size_t i = 0; i < 8; ++i) {
        m_bytes.emplace_back((uint8_t) (creation_time_serialized >> (56 - i * 8)));
    }
}

function_id_t function_ir_binary_t::deserialize_function_id(size_t& offset) const {
    function_id_t result;

    while (offset < m_bytes.size() && m_bytes[offset] != 0) {
        result.ns.push_back((char) m_bytes[offset]);
        ++offset;
    }

    if (offset == m_bytes.size() || m_bytes[offset] != 0) {
        throw std::runtime_error("failed to deserialize function_id: unexpected end of data while reading namespace");
    }
    ++offset;

    while (offset < m_bytes.size() && m_bytes[offset] != 0) {
        result.name.push_back((char) m_bytes[offset]);
        ++offset;
    }

    if (offset == m_bytes.size() || m_bytes[offset] != 0) {
        throw std::runtime_error("failed to deserialize function_id: unexpected end of data while reading name");
    }
    ++offset;

    if (m_bytes.size() < offset + 8) {
        throw std::runtime_error("failed to deserialize function_id: unexpected end of data while reading creation time");
    }
    uint64_t creation_time_serialized = 0;
    for (size_t i = 0; i < 8; ++i) {
        creation_time_serialized |= ((uint64_t) m_bytes[offset + i]) << (56 - i * 8);
    }
    result.creation_time = deserialize_creation_time(creation_time_serialized);
    offset += 8;

    return result;
}

