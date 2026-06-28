#include <m03gagbht5685jfnokvj7crv2c_create_module/create_module.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbht2l61mj6qitacwbmea_base36/base36.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhtft23yhjwpp881tfmc_uuidv7/uuidv7.h>

#include <cstddef>
#include <fstream>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace create_module {

static constexpr std::size_t UUIDV7_BASE36_WIDTH = 25;

static bool is_lowercase_slug_char(char c) {
    return ('a' <= c && c <= 'z')
        || ('0' <= c && c <= '9')
        || c == '_';
}

static std::string validate_name(std::string_view name) {
    if (name.empty()) {
        throw std::runtime_error("create_module: name must not be empty");
    }

    for (const char c : name) {
        if (!is_lowercase_slug_char(c)) {
            throw std::runtime_error("create_module: name must contain only lowercase ASCII letters, digits, and underscores");
        }
    }

    if (name.front() == '_' || name.back() == '_') {
        throw std::runtime_error("create_module: name must not start or end with underscore");
    }

    if (name.find("__") != std::string_view::npos) {
        throw std::runtime_error("create_module: name must not contain consecutive underscores");
    }

    return std::string(name);
}

static std::string uuidv7_base36_prefix() {
    auto encoded = base36::encode(uuidv7::uuidv7::generate().bytes());
    if (UUIDV7_BASE36_WIDTH < encoded.size()) {
        throw std::runtime_error(std::format(
            "create_module: UUIDv7 base36 value '{}' is wider than {} characters",
            encoded,
            UUIDV7_BASE36_WIDTH
        ));
    }

    return std::string(UUIDV7_BASE36_WIDTH - encoded.size(), '0') + encoded;
}

static std::string uppercase_identifier(std::string_view value) {
    std::string result;
    result.reserve(value.size());

    for (const char c : value) {
        if ('a' <= c && c <= 'z') {
            result.push_back(static_cast<char>('A' + (c - 'a')));
        } else {
            result.push_back(c);
        }
    }

    return result;
}

static std::string cxx_string_literal(std::string_view value) {
    std::string result("\"");

    for (const char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

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

static void validate_target_workspace(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context_t& invocation_context,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& workspace
) {
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );
    auto* phase_module = workspace_graph.discover_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t("m03gagbhsujjf63n0w3r2w4q6h_build_phases"));

    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_t* target_workspace = nullptr;
    for (const auto* candidate : workspace_graph.workspaces()) {
        if (candidate->relative_path() == workspace) {
            target_workspace = candidate;
            break ;
        }
    }

    if (target_workspace == nullptr) {
        throw std::runtime_error(std::format("create_module: workspace '{}' is not listed in workspaces.json", workspace));
    }

    if (target_workspace->order_position() <= phase_module->workspace().order_position()) {
        throw std::runtime_error(std::format(
            "create_module: workspace '{}' cannot host generated boilerplate because its builder depends on phase from workspace '{}'; choose a later workspace",
            workspace,
            phase_module->workspace().relative_path()
        ));
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

static std::string header_source(std::string_view module_name) {
    const auto guard = std::format("{}_MODULE_H", uppercase_identifier(module_name));

    return std::format(
        "#ifndef {0}\n"
        "# define {0}\n"
        "\n"
        "# include <string>\n"
        "\n"
        "namespace {1} {{\n"
        "\n"
        "std::string message();\n"
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
        "#include <{0}/module.h>\n"
        "\n"
        "namespace {0} {{\n"
        "\n"
        "std::string message() {{\n"
        "    return {1};\n"
        "}}\n"
        "\n"
        "}} // namespace {0}\n",
        module_name,
        cxx_string_literal(module_name)
    );
}

static std::string cli_source(std::string_view module_name) {
    return std::format(
        "#include <{0}/module.h>\n"
        "\n"
        "#include <iostream>\n"
        "\n"
        "int main() {{\n"
        "    std::cout << {0}::message() << std::endl;\n"
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
        "namespace {0} {{\n"
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
        "}}\n"
        "\n"
        "}} // namespace {0}\n",
        module_name
    );
}

std::string make_module_name(std::string_view name) {
    return std::format("m{}_{}", uuidv7_base36_prefix(), validate_name(name));
}

created_module_t create(std::string_view workspace, std::string_view name) {
    const auto module_name = make_module_name(name);

    const auto invocation_context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
    const auto workspace_relative_path = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::string(workspace));
    validate_target_workspace(invocation_context, workspace_relative_path);

    const auto workspace_dir = invocation_context.workspace_root / workspace_relative_path;
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(workspace_dir)) {
        throw std::runtime_error(std::format("create_module: workspace directory '{}' does not exist", workspace_dir));
    }
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(workspace_dir)) {
        throw std::runtime_error(std::format("create_module: workspace path '{}' is not a directory", workspace_dir));
    }

    const auto module_dir = workspace_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(module_name);
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("create_module: module directory '{}' already exists", module_dir));
    }

    m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(module_dir);

    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::MODULE_JSON), deps_json());
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module.h"), header_source(module_name));
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module.cpp"), module_source(module_name));
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::CLI_CPP), cli_source(module_name));
    write_file(module_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::BUILDER_CPP), builder_source(module_name));

    return created_module_t {
        .name = module_name,
        .directory = module_dir
    };
}

} // namespace create_module
