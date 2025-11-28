#include <function_compound.h>

function_t* function_compound_t::function(typesystem_t& typesystem, function_ir_t function_ir) {
    return new function_t(
        typesystem,
        function_ir,
        [](function_t& function, uint8_t argument_index) {
            function.expand();
            function.send(argument_index);
        }
    );
}

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>

TEST(FunctionCompoundTest, ConstructsFromIr) {
    typesystem_t typesystem;
    function_ir_t ir{
        .function_id = function_id_t{ .ns = "ns", .name = "compound", .creation_time = std::chrono::system_clock::now() },
        .left = 0,
        .right = 1,
        .top = 0,
        .bottom = 1,
        .children = {},
        .connections = {}
    };

    auto fn = function_compound_t::function(typesystem, ir);
    EXPECT_TRUE(fn->children().empty());
}
#endif
