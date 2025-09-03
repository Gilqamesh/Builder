#include <gtest/gtest.h>

#include "builder.h"

TEST(Compiler, RegisterPrimitiveNode) {
    COMPILER.clear();
    
    std::string node_name("mul");
    ASSERT_EQ(COMPILER.register_node<mul_node_t>(node_name), true);
    node_t* node = COMPILER.create_node(node_name);
    ASSERT_EQ(node->name(), node_name);
}

TEST(Compiler, RegisterBinary) {
    COMPILER.clear();

    std::string mul_node_name("mul");
    std::string logger_node_name("logger");
    ASSERT_EQ(COMPILER.register_node<mul_node_t>(mul_node_name), true);
    ASSERT_EQ(COMPILER.register_node<logger_node_t>(logger_node_name), true);

    std::string result_node_name = "mul-display";
    std::vector<uint8_t> binary;
    for (uint8_t c : result_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t cur_node_index = 0;
    uint8_t result_node_index = (uint8_t) -1;

    uint8_t mul_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : mul_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t logger_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : logger_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    // connect result port 0 to mul port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(result_node_index);
    binary.push_back(PORT_0);
    binary.push_back(mul_node_index);
    binary.push_back(PORT_0);

    // connect result port 1 to mul port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(result_node_index);
    binary.push_back(PORT_1);
    binary.push_back(mul_node_index);
    binary.push_back(PORT_1);

    // connect result port 2 to logger port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(result_node_index);
    binary.push_back(PORT_2);
    binary.push_back(logger_node_index);
    binary.push_back(PORT_1);

    // connect mul port 2 to logger port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(mul_node_index);
    binary.push_back(PORT_2);
    binary.push_back(logger_node_index);
    binary.push_back(PORT_0);

    node_t* result_node = new node_t;
    ASSERT_TRUE(COMPILER.binary_to_node(binary, result_node));
    ASSERT_EQ(result_node->name(), result_node_name);

    std::stringstream ss;
    result_node->write(PORT_2, (std::ostream*) &ss);
    ASSERT_EQ(ss.str(), "");

    int in_a = 3;
    result_node->write(PORT_0, in_a);
    ASSERT_EQ(ss.str(), "");

    int in_b = 4;
    result_node->write(PORT_1, in_b);
    ASSERT_EQ(ss.str(), "12\n");

    std::string human_readable;
    std::cout << "Handcrafted binary:\n";
    ASSERT_TRUE(COMPILER.binary_to_human_readable(binary, human_readable));
    std::cout << human_readable << std::endl;
    std::vector<uint8_t> binary_result = COMPILER.node_to_binary(result_node);
    ASSERT_TRUE(COMPILER.binary_to_human_readable(binary_result, human_readable));
    std::cout << "Generated binary:\n";
    std::cout << human_readable << std::endl;

    // ASSERT_EQ(binary, binary_result);
    // ASSERT_EQ(COMPILER.are_binaries_equal(binary, binary_result), true);
}

TEST(Compiler, FactorialMachine) {
    COMPILER.clear();

    std::string if_node_name("if");
    std::string mul_node_name("mul");
    std::string sub_node_name("sub");
    std::string is_zero_node_name("is_zero");
    std::string int_node_name("int");
    std::string pin_node_name("pin");
    std::string result_node_name("fact");

    COMPILER.register_node<if_node_t>(if_node_name);
    COMPILER.register_node<mul_node_t>(mul_node_name);
    COMPILER.register_node<sub_t>(sub_node_name);
    COMPILER.register_node<is_zero_t>(is_zero_node_name);
    COMPILER.register_node<int_node_t>(int_node_name);
    COMPILER.register_node<pin_node_t>(pin_node_name);

    std::vector<uint8_t> binary;
    for (uint8_t c : result_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    // create components
    uint8_t cur_node_index = 0;
    uint8_t result_node_index = (uint8_t) -1;

    uint8_t pin_0_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : pin_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t pin_1_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : pin_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t pin_2_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : pin_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t one_0_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : int_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t one_1_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : int_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t sub_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : sub_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t is_zero_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : is_zero_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t if_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : if_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t mul_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : mul_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    uint8_t fact_node_index = cur_node_index++;
    binary.push_back(OP_CREATE_NODE);
    for (uint8_t c : result_node_name) {
        binary.push_back(c);
    }
    binary.push_back(0);

    // connect result port 0 to pin_0 port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(result_node_index);
    binary.push_back(PORT_0);
    binary.push_back(pin_0_node_index);
    binary.push_back(PORT_0);

    // connect pin_2 port 2 to result port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(pin_2_node_index);
    binary.push_back(PORT_2);
    binary.push_back(result_node_index);
    binary.push_back(PORT_1);

    // connect pin_0 port 2 to is_zero port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(pin_0_node_index);
    binary.push_back(PORT_2);
    binary.push_back(is_zero_node_index);
    binary.push_back(PORT_0);

    // connect is_zero port 1 to if port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(is_zero_node_index);
    binary.push_back(PORT_1);
    binary.push_back(if_node_index);
    binary.push_back(PORT_0);

    // connect one_0 port 0 to if port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(one_0_node_index);
    binary.push_back(PORT_0);
    binary.push_back(if_node_index);
    binary.push_back(PORT_1);

    // connect if port 3 to pin_2 port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(if_node_index);
    binary.push_back(PORT_3);
    binary.push_back(pin_2_node_index);
    binary.push_back(PORT_0);

    // connect pin_0 port 1 to if port 2
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(pin_0_node_index);
    binary.push_back(PORT_1);
    binary.push_back(if_node_index);
    binary.push_back(PORT_2);

    // connect if port 4 to pin_1 port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(if_node_index);
    binary.push_back(PORT_4);
    binary.push_back(pin_1_node_index);
    binary.push_back(PORT_0);

    // connect pin_1 port 1 to sub port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(pin_1_node_index);
    binary.push_back(PORT_1);
    binary.push_back(sub_node_index);
    binary.push_back(PORT_0);

    // connect one_1 port 0 to sub port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(one_1_node_index);
    binary.push_back(PORT_0);
    binary.push_back(sub_node_index);
    binary.push_back(PORT_1);

    // connect sub port 2 to fact port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(sub_node_index);
    binary.push_back(PORT_2);
    binary.push_back(fact_node_index);
    binary.push_back(PORT_0);

    // connect fact port 1 to mul port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(fact_node_index);
    binary.push_back(PORT_1);
    binary.push_back(mul_node_index);
    binary.push_back(PORT_1);

    // connect pin_1 port 2 to mul port 0
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(pin_1_node_index);
    binary.push_back(PORT_2);
    binary.push_back(mul_node_index);
    binary.push_back(PORT_0);

    // connect mul port 2 to pin_2 port 1
    binary.push_back(OP_CONNECT_PORTS);
    binary.push_back(mul_node_index);
    binary.push_back(PORT_2);
    binary.push_back(pin_2_node_index);
    binary.push_back(PORT_1);

    std::string human_readable;
    ASSERT_TRUE(COMPILER.binary_to_human_readable(binary, human_readable));
    std::cout << "Handcrafted fact binary:\n";
    std::cout << human_readable << std::endl;

    ASSERT_TRUE(COMPILER.register_node(binary));

    node_t* node = COMPILER.create_node(result_node_name);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->name(), result_node_name);

    int_node_t in(5);
    in.connect(node, PORT_0, PORT_0);
    int out_value = 0;
    ASSERT_EQ(node->read(PORT_1, out_value), true);
    ASSERT_EQ(out_value, 120);
}
