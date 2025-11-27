#include <gtest/gtest.h>

#include "function.h"

TEST(Node, Happy) {
    node_t node;

    ASSERT_EQ(node.name(), std::string(""));
    std::string node_name("asd");
    node.name(node_name);
    ASSERT_EQ(node.name(), node_name);

    for (int i = 0; i < __PORT_SIZE; ++i) {
        port_index_t port_index = (port_index_t) i;

        ASSERT_EQ(node.port_name(port_index), "");
        std::string port_index_name("port_" + std::to_string(i));
        node.port_name(port_index, port_index_name);
        ASSERT_EQ(node.port_name(port_index), port_index_name);
    }

    int written = 0x1234;
    int int_result = written + 10;

    node_t other;
    ASSERT_EQ(node.is_connected(PORT_0), false);
    ASSERT_EQ(node.is_connected(PORT_3), false);
    ASSERT_EQ(other.is_connected(PORT_0), false);
    ASSERT_EQ(other.is_connected(PORT_3), false);

    ASSERT_EQ(node.connection(PORT_0), nullptr);
    ASSERT_EQ(node.connection(PORT_3), nullptr);
    ASSERT_EQ(other.connection(PORT_0), nullptr);
    ASSERT_EQ(other.connection(PORT_3), nullptr);

    ASSERT_EQ(node.read(PORT_0, int_result), false);
    ASSERT_EQ(node.read(PORT_3, int_result), false);
    ASSERT_EQ(other.read(PORT_0, int_result), false);
    ASSERT_EQ(other.read(PORT_3, int_result), false);

    node.connect(&other, PORT_3, PORT_0);

    ASSERT_EQ(node.is_connected(PORT_0), true);
    ASSERT_EQ(node.is_connected(PORT_3), false);
    ASSERT_EQ(other.is_connected(PORT_0), false);
    ASSERT_EQ(other.is_connected(PORT_3), false);

    ASSERT_EQ(node.connection(PORT_0), &other);
    ASSERT_EQ(node.connection(PORT_3), nullptr);
    ASSERT_EQ(other.connection(PORT_0), nullptr);
    ASSERT_EQ(other.connection(PORT_3), nullptr);

    ASSERT_EQ(node.read(PORT_0, int_result), false);
    ASSERT_EQ(node.read(PORT_3, int_result), false);
    ASSERT_EQ(other.read(PORT_0, int_result), false);
    ASSERT_EQ(other.read(PORT_3, int_result), false);

    node.write(PORT_0, written);

    ASSERT_EQ(node.read(PORT_0, int_result), true);
    ASSERT_EQ(int_result, written);
    ASSERT_EQ(node.read(PORT_3, int_result), false);
    ASSERT_EQ(int_result, written);
    ASSERT_EQ(other.read(PORT_0, int_result), false);
    int_result = written + 10;
    ASSERT_EQ(other.read(PORT_3, int_result), true);
    ASSERT_EQ(int_result, written);
}
