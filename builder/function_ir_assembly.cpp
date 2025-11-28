#include "function_ir_assembly.h"

function_ir_assembly_t::function_ir_assembly_t(const function_ir_t& function_ir) {
    function_id_t::to_string(function_ir.function_id);
    m_assembly += ":\n";

    for (size_t i = 0; i < function_ir.children.size(); ++i) {
        const auto& child = function_ir.children[i];
        m_assembly += std::format("  {} ", i);
        m_assembly += function_id_t::to_string(child.function_id);
        m_assembly += std::format(" ({}, {}, {}, {})\n", child.left, child.right, child.top, child.bottom);
    }

    for (const auto& connection : function_ir.connections) {
        m_assembly += "  ";
        function_id_t::to_string(connection.from_function_index == static_cast<uint16_t>(-1) ? function_ir.function_id : function_ir.children[connection.from_function_index].function_id);
        m_assembly += std::format(".{} -> ", connection.from_argument_index);
        function_id_t::to_string(connection.to_function_index == static_cast<uint16_t>(-1) ? function_ir.function_id : function_ir.children[connection.to_function_index].function_id);
        m_assembly += std::format(".{}\n", connection.to_argument_index);
    }

    m_assembly += "\n";
}

const std::string& function_ir_assembly_t::assembly() const {
    return m_assembly;
}

function_ir_t function_ir_assembly_t::function_ir() const {
    throw std::runtime_error("function_ir_assembly_t::function_ir() not implemented");
}

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>

TEST(FunctionIrAssemblyTest, ProducesAssemblyString) {
    function_ir_t ir {
        .function_id = function_id_t{ .ns = "ns", .name = "root", .creation_time = std::chrono::system_clock::now() },
        .left = 0,
        .right = 10,
        .top = 0,
        .bottom = 10,
        .children = {
            function_ir_t::child_t{ .function_id = function_id_t{ .ns = "ns", .name = "child", .creation_time = std::chrono::system_clock::now() }, .left = 1, .right = 2, .top = 3, .bottom = 4 }
        },
        .connections = {}
    };

    function_ir_assembly_t assembly(ir);
    EXPECT_FALSE(assembly.assembly().empty());
}
#endif
