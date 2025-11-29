#include <function_primitive_cpp.h>

#include <gtest/gtest.h>

TEST(FunctionPrimitiveCppTest, WrapsCallable) {
    typesystem_t typesystem;
    function_t* fn = function_primitive_cpp_t<>::function(typesystem, "cpp", "adder", [](int a, int b) { return a + b; });

    EXPECT_TRUE(fn->children().empty());
    EXPECT_EQ(fn->arguments().size(), 0u);
}
