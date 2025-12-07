#include "module.h"

#include <format>
#include <regex>
#include <fstream>

module_t::module_t(
    const std::string name,
    const std::filesystem::path& builder_cpp_path,
    const std::filesystem::path& artifacts_root,
    const std::filesystem::path& modules_root
):
    m_name(name),
    m_builder_cpp_path(builder_cpp_path),
    m_artifacts_root(artifacts_root),
    m_modules_root(modules_root),
    m_artifact_path(artifacts_root / (name + ".builder")),
    m_construct_state(construct_state_t::NOT_CONSTRUCTED),
    m_build_state(build_state_t::NOT_BUILT),
    m_run_state(run_state_t::NOT_RAN)
{
}

const std::string& module_t::name() const {
    return m_name;
}

const std::filesystem::path& module_t::builder_cpp_path() const {
    return m_builder_cpp_path;
}

const std::unordered_map<std::string, module_t*>& module_t::dependencies() const {
    return m_dependencies;
}

void module_t::print_dependencies(std::ostream& os) const {
    print_dependencies(os, 0);
}

void module_t::construct(std::ostream& os, std::unordered_map<std::string, module_t*>& module_by_name) {
    std::stack<module_t*> processed_stack;
    construct(os, module_by_name, processed_stack);
}

void module_t::build(std::ostream& os) {
    switch (m_build_state) {
        case build_state_t::NOT_BUILT: {
            if (std::filesystem::exists(m_artifact_path)) {
                const auto time_last_write_artifact = std::filesystem::last_write_time(m_artifact_path);
                const auto time_last_write_builder_cpp = std::filesystem::last_write_time(m_builder_cpp_path);
                if (time_last_write_builder_cpp <= time_last_write_artifact) {
                    m_build_state = build_state_t::BUILT;
                    return ;
                }
            }
        } break ;
        case build_state_t::BUILDING: {
            os << std::format("[error] cyclic module build detected involving module: {}", m_name) << std::endl;
            exit(1);
        } break ;
        case build_state_t::BUILT: {
            return ;
        } break ;
        default: {
            os << std::format("[error] invalid build state for module: {}", m_name) << std::endl;
            exit(1);
        } break ;
    }

    for (const auto& [dependency_name, dependency_module] : m_dependencies) {
        (void) dependency_name;
        dependency_module->build(os);
    }

    m_build_state = build_state_t::BUILDING;

    const auto build_command = std::format(
        "g++ -g -O2 -std=c++23 -Wall -Werror -Wextra -I{} -o {} {}",
        m_modules_root.string(), m_artifact_path.string(), m_builder_cpp_path.string()
    );
    os << build_command << std::endl;

    const int build_result = std::system(build_command.c_str());
    if (build_result) {
        os << std::format("[error] module {} build failed with exit code {}", m_name, build_result) << std::endl;
        exit(1);
    }

    if (!std::filesystem::exists(m_artifact_path)) {
        os << std::format("[error] module {} build failed: artifact not found at {}", m_name, m_artifact_path.string()) << std::endl;
        exit(1);
    }

    m_build_state = build_state_t::BUILT;
}

void module_t::run(std::ostream& os) {
    switch (m_run_state) {
        case run_state_t::NOT_RAN: {
        } break ;
        case run_state_t::RUNNING: {
            os << std::format("[error] cyclic module run detected involving module: {}", m_name) << std::endl;
            exit(1);
        } break ;
        case run_state_t::RAN: {
            return ;
        } break ;
        default: {
            os << std::format("[error] invalid run state for module: {}", m_name) << std::endl;
            exit(1);
        } break ;
    }

    if (!std::filesystem::exists(m_artifact_path)) {
        os << std::format("[error] module {} build failed: artifact not found at {}", m_name, m_artifact_path.string()) << std::endl;
        exit(1);
    }

    m_run_state = run_state_t::RUNNING;

    const auto run_command = std::format(
        "{} {} {} {}",
        m_artifact_path.string(), m_artifacts_root.string(), m_modules_root.string(), m_name
    );
    os << run_command << std::endl;

    const int run_result = std::system(run_command.c_str());
    if (run_result) {
        os << std::format("[error] module {} run failed with exit code {}", m_name, run_result) << std::endl;
        exit(1);
    }

    m_run_state = run_state_t::RAN;
}

void module_t::print_dependencies(std::ostream& os, int depth) const {
    for (int i = 0; i < depth; ++i) {
        os << "  ";
    }
    os << m_name << std::endl;

    for (const auto& [dependency_name, dependency_module] : m_dependencies) {
        (void) dependency_name;
        dependency_module->print_dependencies(os, depth + 1);
    }
}

void module_t::construct(std::ostream& os, std::unordered_map<std::string, module_t*>& module_by_name, std::stack<module_t*>& processed_stack) {
    switch (m_construct_state) {
        case construct_state_t::NOT_CONSTRUCTED: {
        } break ;
        case construct_state_t::CONSTRUCTING: {
            os << std::format("[error] cyclic module dependency detected between the following modules") << std::endl;
            while (!processed_stack.empty()) {
                module_t* module = processed_stack.top();
                processed_stack.pop();
                os << std::format("  {}", module->m_name) << std::endl;
            }
            exit(1);
        } break ;
        case construct_state_t::CONSTRUCTED: {
            return ;
        } break ;
        default: {
            os << std::format("[error] invalid construct state for module: {}", m_name) << std::endl;
            exit(1);
        } break ;
    }

    m_construct_state = construct_state_t::CONSTRUCTING;
    processed_stack.emplace(this);

    std::ifstream ifs(m_builder_cpp_path);
    if (!ifs) {
        os << std::format("[error] failed to open target module file: {}", m_builder_cpp_path.string()) << std::endl;
        exit(1);
    }

    const std::regex import_regex(R"(^\s*#\s*include\s*[<"]\s*([A-Za-z0-9_]+)\s*/[^">]+[">])");
    std::string line;
    while (std::getline(ifs, line)) {
        std::smatch match;
        if (std::regex_match(line, match, import_regex)) {
            const std::string imported_module_name = match[1].str();
            if (imported_module_name != m_name) {
                auto it = module_by_name.find(imported_module_name);
                if (it != module_by_name.end()) {
                    module_t* imported_module = it->second;
                    m_dependencies.emplace(imported_module_name, imported_module);
                    imported_module->construct(os, module_by_name, processed_stack);
                }
            }
        }
    }

    processed_stack.pop();
    m_construct_state = construct_state_t::CONSTRUCTED;
}
