#include <function_primitive_lang.h>

function_t* function_primitive_lang_t::function(typesystem_t& typesystem, std::string ns, std::string name, function_t::function_call_t function_call) {
    return new function_t(
        typesystem,
        function_ir_t {
            .function_id = function_id_t {
                .ns = std::move(ns),
                .name = std::move(name),
                .creation_time = std::chrono::system_clock::now()
            },
            .left = {},
            .right = {},
            .top = {},
            .bottom = {},
            .children = {},
            .connections = {}
        },
        std::move(function_call)
    );
}

#ifdef BUILDER_ENABLE_TESTS
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
#endif
