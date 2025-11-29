#include <function_ir_assembly.h>
#include <chrono>
#include <format>
#include <stdexcept>

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

