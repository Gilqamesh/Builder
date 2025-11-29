#include <gtest/gtest.h>
#include <function_ir.h>

TEST(FunctionIrTest, InitializesEmptyVectors) {
    function_ir_t ir{};

    EXPECT_TRUE(ir.children.empty());
    EXPECT_TRUE(ir.connections.empty());
}
