#include <function_ir_binary.h>

#include <gtest/gtest.h>

#include <chrono>

TEST(FunctionIrBinaryTest, SerializesAndDeserializes) {
    function_ir_t ir {
        .function_id = function_id_t{
            .ns = "ns",
            .name = "name",
            .creation_time = std::chrono::system_clock::now()
        },
        .left = 1,
        .right = 2,
        .top = 3,
        .bottom = 4,
        .children = {
            function_ir_t::child_t{
                .function_id = function_id_t{ .ns = "child", .name = "one", .creation_time = std::chrono::system_clock::now() },
                .left = 5,
                .right = 6,
                .top = 7,
                .bottom = 8
            }
        },
        .connections = {
            function_ir_t::connection_info_t{
                .from_function_index = 0,
                .from_argument_index = 1,
                .to_function_index = 0,
                .to_argument_index = 2
            }
        }
    };

    function_ir_binary_t binary(ir);
    const auto recovered = binary.function_ir();

    ASSERT_EQ(recovered.children.size(), ir.children.size());
    ASSERT_EQ(recovered.connections.size(), ir.connections.size());
    EXPECT_EQ(recovered.children.front().left, ir.children.front().left);
    EXPECT_EQ(recovered.connections.front().from_argument_index, ir.connections.front().from_argument_index);
}
