#include "function_ir_assembly.h"

function_ir_assembly_t::function_ir_assembly_t(const function_ir_t& ir) {
    serialize_function_id(ir.function_id);
    m_assembly += ":\n";

    for (int i = 0; i < ir.children.size(); ++i) {
        const auto& child = ir.children[i];
        m_assembly += std::format("  {} ", i);
        serialize_function_id(child.function_ir->function_id);
        m_assembly += std::format(" ({}, {}, {}, {})\n", child.left, child.right, child.top, child.bottom);
    }

    for (const auto& connection : ir.connections) {
        m_assembly += "  ";
        serialize_function_id(connection.from_function_index == -1 ? ir.function_id : ir.children[connection.from_function_index].function_ir->function_id);
        m_assembly += std::format(".{} -> ", connection.from_argument_index);
        serialize_function_id(connection.to_function_index == -1 ? ir.function_id : ir.children[connection.to_function_index].function_ir->function_id);
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

void function_ir_assembly_t::serialize_function_id(const function_id_t& function_id) {
    m_assembly += function_id.ns + "::" + function_id.name + function_id.creation_time.transform([](const auto& tp) { return std::format("@{0:%Y-%m-%d}-{0:%H-%M-%S}", tp); }).value_or("");
}
