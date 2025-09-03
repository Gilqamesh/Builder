#include <gtest/gtest.h>

#include "builder.h"

TEST(IfNode, Happy) {
    if_node_t node;
    int port_index = 0;
    int int_result;
    while (port_index < __PORT_SIZE) {
        ASSERT_EQ(node.is_connected((port_index_t) port_index), false);
        ASSERT_EQ(node.connection((port_index_t) port_index), nullptr);
        ASSERT_EQ(node.read((port_index_t) port_index, int_result), false);
        ++port_index;
    }

    bool bool_result;
    ASSERT_EQ(node.read(PORT_0, bool_result), false);
    ASSERT_EQ(node.read(PORT_1, int_result), false);
    ASSERT_EQ(node.read(PORT_2, int_result), false);
    ASSERT_EQ(node.read(PORT_3, int_result), false);
    ASSERT_EQ(node.read(PORT_4, int_result), false);

    const int consequence_value = 10;
    node.write(PORT_1, consequence_value);
    node.call(PORT_1);
    ASSERT_EQ(node.read(PORT_0, bool_result), false);
    ASSERT_EQ(node.read(PORT_1, int_result), true);
    ASSERT_EQ(node.read(PORT_2, int_result), false);
    ASSERT_EQ(node.read(PORT_3, int_result), false);
    ASSERT_EQ(node.read(PORT_4, int_result), false);

    const int alternative_value = 20;
    node.write(PORT_2, alternative_value);
    node.call(PORT_2);
    ASSERT_EQ(node.read(PORT_0, bool_result), false);
    ASSERT_EQ(node.read(PORT_1, int_result), true);
    ASSERT_EQ(node.read(PORT_2, int_result), true);
    ASSERT_EQ(node.read(PORT_3, int_result), false);
    ASSERT_EQ(node.read(PORT_4, int_result), false);

    node.write(PORT_0, true);
    node.call(PORT_0);
    ASSERT_EQ(node.read(PORT_0, bool_result), true);
    ASSERT_EQ(node.read(PORT_1, int_result), true);
    ASSERT_EQ(node.read(PORT_2, int_result), true);
    ASSERT_EQ(node.read(PORT_3, int_result), true);
    ASSERT_EQ(node.read(PORT_4, int_result), false);

    ASSERT_EQ(node.read(PORT_3, int_result), true);
    ASSERT_EQ(int_result, consequence_value);

    ASSERT_EQ(node.read(PORT_0, bool_result), true);
    ASSERT_EQ(bool_result, true);

    node.write(PORT_0, false);
    node.call(PORT_0);
    ASSERT_EQ(node.read(PORT_0, bool_result), true);
    ASSERT_EQ(node.read(PORT_1, int_result), true);
    ASSERT_EQ(node.read(PORT_2, int_result), true);
    ASSERT_EQ(node.read(PORT_3, int_result), true);
    ASSERT_EQ(node.read(PORT_4, int_result), true);

    ASSERT_EQ(node.read(PORT_4, int_result), true);
    ASSERT_EQ(int_result, alternative_value);

    ASSERT_EQ(node.read(PORT_0, bool_result), true);
    ASSERT_EQ(bool_result, false);
}

TEST(MulNode, Happy) {
    mul_node_t node;

    int port_index = 0;
    int int_result;
    while (port_index < __PORT_SIZE) {
        ASSERT_EQ(node.is_connected((port_index_t) port_index), false);
        ASSERT_EQ(node.connection((port_index_t) port_index), nullptr);
        ASSERT_EQ(node.read((port_index_t) port_index, int_result), false);
        ++port_index;
    }

    const int a = 3;
    const int b = 4;
    node.write(PORT_0, a);
    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, int_result), false);
    ASSERT_EQ(node.read(PORT_2, int_result), false);

    node.write(PORT_1, b);
    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, int_result), true);
    ASSERT_EQ(node.read(PORT_2, int_result), false);

    node.call(PORT_0);

    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, int_result), true);
    ASSERT_EQ(node.read(PORT_2, int_result), true);

    ASSERT_EQ(int_result, a * b);
}

TEST(IsZeroNode, Happy) {
    is_zero_t node;

    int port_index = 0;
    int int_result;
    while (port_index < __PORT_SIZE) {
        ASSERT_EQ(node.is_connected((port_index_t) port_index), false);
        ASSERT_EQ(node.connection((port_index_t) port_index), nullptr);
        ASSERT_EQ(node.read((port_index_t) port_index, int_result), false);
        ++port_index;
    }

    bool bool_result;
    ASSERT_EQ(node.read(PORT_0, int_result), false);
    ASSERT_EQ(node.read(PORT_1, bool_result), false);

    const int non_zero_value = 10;
    node.write(PORT_0, non_zero_value);
    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, bool_result), false);

    node.call(PORT_0);
    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, bool_result), true);
    ASSERT_EQ(bool_result, false);

    const int zero_value = 0;
    node.write(PORT_0, zero_value);
    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, bool_result), true);
    ASSERT_EQ(bool_result, false);

    node.call(PORT_0);
    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(node.read(PORT_1, bool_result), true);
    ASSERT_EQ(bool_result, true);
}

TEST(LoggerNode, Happy) {
    logger_node_t node;

    std::stringstream ss;
    node.write(PORT_1, (std::ostream*) &ss);
    node.write(PORT_0, 5);
    node.call(PORT_0);
    ASSERT_EQ(ss.str(), "5\n");
}

TEST(PinNode, Happy) {
    pin_node_t node;

    int port_index = 0;
    int result;
    while (port_index < __PORT_SIZE) {
        ASSERT_EQ(node.is_connected((port_index_t) port_index), false);
        ASSERT_EQ(node.connection((port_index_t) port_index), nullptr);
        ASSERT_EQ(node.read((port_index_t) port_index, result), false);
        ++port_index;
    }

    for (int outer_port_index = 0; outer_port_index < (int) __PORT_SIZE; ++outer_port_index) {
        const int value = outer_port_index * 10;
        node.write((port_index_t) outer_port_index, value);
        node.call((port_index_t) outer_port_index);
        for (int inner_port_index = 0; inner_port_index < (int) __PORT_SIZE; ++inner_port_index) {
            if (inner_port_index <= outer_port_index) {
                result = -1;
                ASSERT_EQ(node.read((port_index_t) inner_port_index, result), true);
                ASSERT_EQ(result, inner_port_index * 10);
            } else {
                result = -1;
                ASSERT_EQ(node.read((port_index_t) inner_port_index, result), false);
                ASSERT_EQ(result, -1);
            }
        }
    }
}
