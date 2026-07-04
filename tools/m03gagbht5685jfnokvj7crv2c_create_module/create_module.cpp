#include <m03gagbht5685jfnokvj7crv2c_create_module/create_module.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhtft23yhjwpp881tfmc_uuid/uuid.h>

#include <cstddef>
#include <fstream>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace m03gagbht5685jfnokvj7crv2c_create_module {

static void write_file(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, std::string_view contents) {
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(path)) {
        throw std::runtime_error(std::format("create_module: file '{}' already exists", path));
    }

    const auto parent = path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent);
    }

    std::ofstream ofs(path.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("create_module: failed to open file '{}'", path));
    }

    ofs << contents;
    if (!ofs) {
        throw std::runtime_error(std::format("create_module: failed to write file '{}'", path));
    }
}

static std::string deps_json() {
    return
        "{\n"
        "    \"module_dependencies\": [],\n"
        "    \"builder_dependencies\": [\n"
        "        \"m03gagbhsujjf63n0w3r2w4q6h_build_phases\",\n"
        "        \"m03gagbhsnusi43zogoacgj2ez_filesystem\",\n"
        "        \"m03gagbhsp2drqq3gkop8pzfrm_workspace_graph\"\n"
        "    ]\n"
        "}\n";
}

static std::string uppercase_identifier(std::string_view name) {
    std::string result;
    result.reserve(name.size());

    for (const char c : name) {
        if ('a' <= c && c <= 'z') {
            result.push_back(static_cast<char>(c - 'a' + 'A'));
        } else {
            result.push_back(c);
        }
    }

    return result;
}

static std::string header_source(std::string_view module_name) {
    const auto guard = std::format("{}_MODULE_H", uppercase_identifier(module_name));

    return std::format(
        "#ifndef {0}\n"
        "# define {0}\n"
        "\n"
        "namespace {1} {{\n"
        "\n"
        "}} // namespace {1}\n"
        "\n"
        "#endif // {0}\n",
        guard,
        module_name
    );
}

static std::string module_source(std::string_view module_name) {
    return std::format(
        "#include \"module.h\"\n"
        "\n"
        "namespace {0} {{\n"
        "\n"
        "}} // namespace {0}\n",
        module_name
    );
}

static std::string cli_source(std::string_view module_name) {
    return std::format(
        "#include \"module.h\"\n"
        "\n"
        "#include <iostream>\n"
        "\n"
        "int main() {{\n"
        "    std::cout << \"Hello from {0}!\" << std::endl;\n"
        "    return 0;\n"
        "}}\n",
        module_name
    );
}

static std::string builder_source(std::string_view module_name) {
    return std::format(
        "#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>\n"
        "#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>\n"
        "#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>\n"
        "\n"
        "extern \"C\" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {{\n"
        "    phase->install_source_tree();\n"
        "}}\n"
        "\n"
        "extern \"C\" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {{\n"
        "    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();\n"
        "    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(\"module.h\")));\n"
        "}}\n"
        "\n"
        "extern \"C\" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {{\n"
        "    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();\n"
        "    const auto library = phase->build_library(\n"
        "        {{ phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(\"module.cpp\")) }},\n"
        "        {{}}\n"
        "    );\n"
        "    phase->install_library(library);\n"
        "}}\n"
        "\n"
        "extern \"C\" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {{\n"
        "    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();\n"
        "    const auto cli = phase->build_cli(\n"
        "        {{ phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::CLI_CPP)) }},\n"
        "        {{}}\n"
        "    );\n"
        "    phase->install_cli(cli);\n"
        "}}\n",
        module_name
    );
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t create(std::string_view workspace, std::string_view friendly_name) {
    const auto module_name = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t::from_friendly_name(friendly_name);

    const auto invocation_context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
    const auto workspace_relative_path = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::string(workspace));

    const auto workspace_dir = invocation_context.workspace_root / workspace_relative_path;
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(workspace_dir)) {
        throw std::runtime_error(std::format("create_module: workspace directory '{}' does not exist", workspace_dir));
    }
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(workspace_dir)) {
        throw std::runtime_error(std::format("create_module: workspace path '{}' is not a directory", workspace_dir));
    }

    const auto module_dir = workspace_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(module_name.unique_name());
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("create_module: module directory '{}' already exists", module_dir));
    }

    m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(module_dir);

    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::MODULE_JSON), deps_json());
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::CLI_CPP), cli_source(module_name.unique_name()));
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::BUILDER_CPP), builder_source(module_name.unique_name()));
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module.h"), header_source(module_name.unique_name()));
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module.cpp"), module_source(module_name.unique_name()));

    return module_dir;
}

} // namespace m03gagbht5685jfnokvj7crv2c_create_module
