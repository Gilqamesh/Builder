#include <gtest/gtest.h>

#include "function_primitive_cpp.h"

TEST(FunctionPrimitiveCppTest, WrapsCallable) {
    typesystem_t typesystem;
    function_primitive_cpp_t<int(int, int)> primitive;
    auto fn = primitive.function(typesystem, "cpp", "adder", [](int a, int b) { return a + b; });

    EXPECT_TRUE(fn.children().empty());
    EXPECT_EQ(fn.arguments().size(), 0u);
}
