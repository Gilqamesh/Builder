#include <function_id.h>
#include <stdexcept>

bool function_id_t::operator==(const function_id_t& other) const {
    return ns == other.ns && name == other.name && creation_time == other.creation_time;
}

function_id_t::operator bool() const {
    return ns != std::string{} && name != std::string{} && creation_time != std::chrono::system_clock::time_point{};
}

std::string function_id_t::to_string(const function_id_t& function_id) {
    return function_id.ns + "::" + function_id.name + "@" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(function_id.creation_time.time_since_epoch()).count());
}

function_id_t function_id_t::from_string(const std::string& str) {
    function_id_t result;

    const size_t ns_end = str.find("::");
    if (ns_end == std::string::npos) {
        throw std::runtime_error("invalid function_id string: " + str);
    }
    result.ns = str.substr(0, ns_end);

    const size_t name_end = str.rfind('@');
    if (name_end == std::string::npos) {
        throw std::runtime_error("invalid function_id string: " + str);
    }
    result.name = str.substr(ns_end + 2, name_end - (ns_end + 2));

    const std::string creation_time_str = str.substr(name_end + 1);
    const uint64_t creation_time_seconds = std::stoull(creation_time_str);
    result.creation_time = std::chrono::system_clock::time_point(std::chrono::seconds(creation_time_seconds));

    return result;
}

