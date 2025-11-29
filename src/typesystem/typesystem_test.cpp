#include <typesystem.h>

#include <gtest/gtest.h>

TEST(TypesystemTest, RegistersAndCoercesTypes) {
    typesystem_t typesystem;
    typesystem.register_type<int>();
    typesystem.register_type<double>();

    typesystem.register_coercion<int, double>([](int value) { return static_cast<double>(value); });

    int id_int = typesystem.type_id<int>();
    EXPECT_EQ(typesystem.sizeof_type(id_int), sizeof(int));

    int value = 7;
    auto reader = typesystem.coerce(value);
    EXPECT_DOUBLE_EQ(static_cast<double>(reader), 7.0);
}
