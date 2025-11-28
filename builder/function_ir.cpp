#include "function_ir.h"

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>

TEST(FunctionIrTest, InitializesEmptyVectors) {
    function_ir_t ir{};

    EXPECT_TRUE(ir.children.empty());
    EXPECT_TRUE(ir.connections.empty());
}
#endif
