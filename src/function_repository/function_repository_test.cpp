#include <function_repository.h>

#include <gtest/gtest.h>

#include <chrono>

namespace {
void noop_call(function_t&, uint8_t) {}
}

TEST(FunctionRepositoryTest, StoresEntries) {
    function_repository_t repo;
    function_ir_t ir{
        .function_id = function_id_t{ .ns = "ns", .name = "id", .creation_time = std::chrono::system_clock::now() },
        .left = 0,
        .right = 0,
        .top = 0,
        .bottom = 0,
        .children = {},
        .connections = {}
    };

    repo.save(ir, &noop_call);
    const auto entry = repo.load(ir.function_id);

    EXPECT_EQ(entry.ir.function_id, ir.function_id);
    EXPECT_EQ(entry.call, &noop_call);
}
