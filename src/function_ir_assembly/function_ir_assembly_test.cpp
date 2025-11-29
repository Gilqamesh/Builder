#include <function_ir_assembly.h>

#include <gtest/gtest.h>

#include <chrono>

TEST(FunctionIrAssemblyTest, ProducesAssemblyString) {
    function_ir_t ir {
        .function_id = function_id_t{ .ns = "ns", .name = "root", .creation_time = std::chrono::system_clock::now() },
        .left = 0,
        .right = 10,
        .top = 0,
        .bottom = 10,
        .children = {
            function_ir_t::child_t{ .function_id = function_id_t{ .ns = "ns", .name = "child", .creation_time = std::chrono::system_clock::now() }, .left = 1, .right = 2, .top = 3, .bottom = 4 }
        },
        .connections = {}
    };

    function_ir_assembly_t assembly(ir);
    EXPECT_FALSE(assembly.assembly().empty());
}
