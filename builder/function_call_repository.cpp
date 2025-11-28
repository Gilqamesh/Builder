#include <function_call_repository.h>

void function_call_repository_t::save(function_id_t id, function_t::function_call_t call) {
    m_function_calls.emplace(std::move(id), std::move(call));
}

function_t::function_call_t function_call_repository_t::load(const function_id_t& id) const {
    auto it = m_function_calls.find(id);
    if (it == m_function_calls.end()) {
        return nullptr;
    }

    return it->second;
}

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>

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
#endif
