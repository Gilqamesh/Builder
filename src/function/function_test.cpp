#include <gtest/gtest.h>
#include <function.h>

namespace {
void noop_function(function_t&, uint8_t) {}
}

TEST(FunctionTest, InitializesWithNoParent) {
    typesystem_t typesystem;
    function_ir_t ir{};

    function_t fn(typesystem, ir, &noop_function);
    EXPECT_EQ(fn.parent(), nullptr);

    function_t parent(typesystem, ir, &noop_function);
    fn.parent(&parent);
    EXPECT_EQ(fn.parent(), &parent);
}
