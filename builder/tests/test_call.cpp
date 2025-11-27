#include <gtest/gtest.h>
#include <type_traits>

#include "call.h"

TEST(CallInterfaceTest, IsConstructible) {
    static_assert(std::is_constructible_v<call_t, void*>);
    EXPECT_TRUE(std::is_copy_constructible_v<call_t>);
}
