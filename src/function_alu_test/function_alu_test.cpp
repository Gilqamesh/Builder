#include <gtest/gtest.h>
#include <function_alu.h>

TEST(FunctionAluTest, CreatesArithmeticFunctions) {
    typesystem_t typesystem;
    EXPECT_NO_THROW({ auto add = function_alu_t::add(typesystem); (void)add; });
    EXPECT_NO_THROW({ auto cond = function_alu_t::cond(typesystem); (void)cond; });
}
