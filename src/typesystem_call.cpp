#include <typesystem_call.h>
#include <typesystem.h>

typesystem_call_t::typesystem_call_t(typesystem_t& typesystem, void* function_symbol)
    : call_t(function_symbol), m_typesystem(typesystem) {}

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>
#include <type_traits>

TEST(TypesystemCallTest, IsDerivedFromCall) {
    static_assert(std::is_base_of_v<call_t, typesystem_call_t>);
    EXPECT_TRUE((std::is_constructible_v<typesystem_call_t, typesystem_t&, void*>));
}
#endif
