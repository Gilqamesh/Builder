#include <function_primitive_lang.h>

#include <gtest/gtest.h>

namespace {
void noop_lang_function(function_t&, uint8_t) {}
}

TEST(FunctionPrimitiveLangTest, CreatesFunctionWithMetadata) {
    typesystem_t typesystem;
    auto fn = function_primitive_lang_t::function(typesystem, "lang", "noop", &noop_lang_function);

    EXPECT_TRUE(fn->children().empty());
    EXPECT_EQ(fn->arguments().size(), 0u);
}
