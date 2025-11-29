#include <gtest/gtest.h>
#include <function_call_repository.h>
#include <chrono>

namespace {
void noop_call(function_t&, uint8_t) {}
}

TEST(FunctionCallRepositoryTest, StoresFunctionPointers) {
    function_call_repository_t repo;
    const function_id_t id{ .ns = "ns", .name = "call", .creation_time = std::chrono::system_clock::now() };

    repo.save(id, &noop_call);
    EXPECT_EQ(repo.load(id), &noop_call);
    EXPECT_EQ(repo.load(function_id_t{}), nullptr);
}
