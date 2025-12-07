#include <gtest/gtest.h>
#include <function_id.h>
#include <chrono>

TEST(FunctionIdTest, RoundTripsToString) {
    function_id_t id {
        .ns = "ns",
        .name = "example",
        .creation_time = std::chrono::system_clock::now()
    };

    const std::string encoded = function_id_t::to_string(id);
    const function_id_t decoded = function_id_t::from_string(encoded);

    EXPECT_EQ(decoded.ns, id.ns);
    EXPECT_EQ(decoded.name, id.name);
}
