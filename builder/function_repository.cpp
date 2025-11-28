#include "function_repository.h"

void function_repository_t::save(function_ir_t ir, function_t::function_call_t call) {
    m_functions.emplace(
        ir.function_id,
        entry_t {
            .call = call,
            .ir = ir
        }
    );
}

function_repository_t::entry_t function_repository_t::load(const function_id_t& id) {
    auto it = m_functions.find(id);
    if (it == m_functions.end()) {
        throw std::runtime_error("function not found in repository");
    }

    return it->second;
}

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>

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
#endif
