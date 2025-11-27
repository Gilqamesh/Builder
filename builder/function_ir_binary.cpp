#include "function_ir_binary.h"

enum class opcode_t : uint8_t {
    CREATE_FUNCTION,
    CONNECT_ARGUMENTS
};

static std::chrono::system_clock::time_point deserialize_creation_time(uint64_t serialized_creation_time) {
    const int year_field_length = 12;
    const int month_field_length = 4;
    const int day_field_length = 5;
    const int hour_field_length = 5;
    const int minute_field_length = 6;
    const int second_field_length = 6;
    const int year = (serialized_creation_time >> (64 - year_field_length)) & ((1 << year_field_length) - 1);
    if (!(0 <= year && year < 4096)) {
        throw std::runtime_error(std::format("invalid year {} in serialized creation time", year));
    }
    const int month = (serialized_creation_time >> (64 - year_field_length - month_field_length)) & ((1 << month_field_length) - 1);
    if (!(1 <= month && month <= 12)) {
        throw std::runtime_error(std::format("invalid month {} in serialized creation time", month));
    }
    const int day = (serialized_creation_time >> (64 - year_field_length - month_field_length - day_field_length)) & ((1 << day_field_length) - 1);
    if (!(1 <= day && day <= 31)) {
        throw std::runtime_error(std::format("invalid day {} in serialized creation time", day));
    }
    const int hour = (serialized_creation_time >> (64 - year_field_length - month_field_length - day_field_length - hour_field_length)) & ((1 << hour_field_length) - 1);
    if (!(0 <= hour && hour < 24)) {
        throw std::runtime_error(std::format("invalid hour {} in serialized creation time", hour));
    }
    const int minute = (serialized_creation_time >> (64 - year_field_length - month_field_length - day_field_length - hour_field_length - minute_field_length)) & ((1 << minute_field_length) - 1);
    if (!(0 <= minute && minute < 60)) {
        throw std::runtime_error(std::format("invalid minute {} in serialized creation time", minute));
    }
    const int second = (serialized_creation_time >> (64 - year_field_length - month_field_length - day_field_length - hour_field_length - minute_field_length - second_field_length)) & ((1 << second_field_length) - 1);
    if (!(0 <= second && second < 60)) {
        throw std::runtime_error(std::format("invalid second {} in serialized creation time", second));
    }
    return std::chrono::system_clock::time_point(
        std::chrono::sys_days{
            std::chrono::year(year) /
            std::chrono::month(month) /
            std::chrono::day(day)
        } +
        std::chrono::hours(hour) +
        std::chrono::minutes(minute) +
        std::chrono::seconds(second)
    );
}

static uint64_t serialize_creation_time(const std::chrono::system_clock::time_point& creation_time) {
    return std::chrono::duration_cast<std::chrono::seconds>(creation_time.time_since_epoch()).count();
}

/**
 * Format:
 *   function name <null-terminated string>
 *   creation time <uint64_t>
 *   [opcodes...]*
 * 
 * opcodes:
 *   CREATE_FUNCTION:
 *       op <u8>,
 *       left <i16>, right <i16>, top <i16>, bottom <i16>,
 *       name <null-terminated string>,
 *       creation time <uint64_t>
 * 
 *   CONNECT_ARGUMENTS:
 *       op <u8>,
 *       from_function_index <u8>, from_function_port_index <u8>,
 *       to_function_index <u8>,   to_function_port_index <u8>
 * 
 *   (function_index == (uint8_t)-1 refers to the parent function)
*/

function_ir_binary_t::function_ir_binary_t(const function_ir_t& ir) {
    serialize_function_id(ir.function_id);

    for (const auto& child : ir.children) {
        m_bytes.emplace_back(static_cast<uint8_t>(opcode_t::CREATE_FUNCTION));

        int16_t left = (int16_t) child.left;
        m_bytes.emplace_back((uint8_t) (left >> 8));
        m_bytes.emplace_back((uint8_t) (left & 0xFF));
        int16_t right = (int16_t) child.right;
        m_bytes.emplace_back((uint8_t) (right >> 8));
        m_bytes.emplace_back((uint8_t) (right & 0xFF));
        int16_t top = (int16_t) child.top;
        m_bytes.emplace_back((uint8_t) (top >> 8));
        m_bytes.emplace_back((uint8_t) (top & 0xFF));
        int16_t bottom = (int16_t) child.bottom;
        m_bytes.emplace_back((uint8_t) (bottom >> 8));
        m_bytes.emplace_back((uint8_t) (bottom & 0xFF));

        assert(child.function_ir);
        serialize_function_id(child.function_ir->function_id);
    }
}

const std::vector<uint8_t>& function_ir_binary_t::bytes() const {
    return m_bytes;
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

    uint64_t creation_time_serialized = function_id.creation_time.transform(serialize_creation_time).value_or(0);
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 56));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 48));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 40));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 32));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 24));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 16));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized >> 8));
    m_bytes.emplace_back((uint8_t) (creation_time_serialized & 0xFF));
}
