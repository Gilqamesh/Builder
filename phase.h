#ifndef KERNEL_PHASE_H
# define KERNEL_PHASE_H

# include "graph.h"
# include "module_config.h"
# include "shared_library.h"

# include <format>
# include <stdexcept>
# include <string>
# include <string_view>
# include <unordered_set>
# include <vector>

namespace kernel {

namespace phase {

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

struct output_artifact_t {
    filesystem::path_t path;
    filesystem::relative_path_t relative_path;
};

struct output_t {
    filesystem::path_t root;
    std::vector<output_artifact_t> artifacts;
};

class phase_base_t {
public:
    phase_base_t(
        std::string_view name,
        graph::module_t& module,
        module_config::module_config_t module_config,
        output_t& output
    );

    std::string_view name() const;
    graph::module_t& module() const;
    module_config::module_config_t module_config() const;
    filesystem::path_t artifact_dir() const;
    filesystem::path_t build_dir() const;
    filesystem::path_t install_dir() const;

    template <class phase_t>
    output_t build() const;

    template <class phase_t>
    std::vector<output_t> build_closure() const;

private:
    filesystem::path_t builder_plugin() const;

private:
    std::string_view m_name;
    graph::module_t& m_module;
    module_config::module_config_t m_module_config;
    output_t& m_output;

protected:
    output_t& output() const;
};

struct source_phase_t : phase_base_t {
    source_phase_t(
        graph::module_t& module,
        module_config::module_config_t module_config,
        output_t& output
    );

    filesystem::path_t source_dir() const;
    void add_source_tree() const;
    void add_source(const filesystem::path_t& source, const filesystem::relative_path_t& relative_install_path) const;
};

struct interface_phase_t : phase_base_t {
    interface_phase_t(
        graph::module_t& module,
        module_config::module_config_t module_config,
        output_t& output
    );

    void add_headers_from_source() const;
    void add_interface_from_source(const filesystem::relative_path_t& relative_path) const;
    void add_interface(const output_artifact_t& artifact) const;
    void add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& module_relative_install_path) const;
};

struct library_phase_t : phase_base_t {
    library_phase_t(
        graph::module_t& module,
        module_config::module_config_t module_config,
        output_t& output
    );

    void add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const;
};

struct binary_phase_t : phase_base_t {
    binary_phase_t(
        graph::module_t& module,
        module_config::module_config_t module_config,
        output_t& output
    );

    void add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const;
};

BUILDER_EXTERN void phase__source(const source_phase_t* phase);
BUILDER_EXTERN void phase__interface(const interface_phase_t* phase);
BUILDER_EXTERN void phase__library(const library_phase_t* phase);
BUILDER_EXTERN void phase__binary(const binary_phase_t* phase);

template <class phase_t>
output_t phase_base_t::build() const {
    output_t output {
        .root = filesystem::path_t("/"),
        .artifacts = {}
    };
    const phase_t requested_phase(m_module, m_module_config, output);
    const auto build_dir = requested_phase.build_dir();
    const auto install_dir = requested_phase.install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("{}.{}", requested_phase.name(), state));
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (filesystem::exists(complete_marker)) {
        output_t result {
            .root = install_dir,
            .artifacts = {}
        };

        for (const auto& artifact : filesystem::find(install_dir, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)) {
            result.artifacts.push_back(output_artifact_t {
                .path = artifact,
                .relative_path = install_dir.relative(artifact)
            });
        }

        return result;
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("kernel::phase::phase_base_t::build: re-entry detected for phase '{}'", requested_phase.name()));
    }

    if (filesystem::exists(build_dir)) {
        filesystem::remove_all(build_dir);
    }
    if (filesystem::exists(install_dir)) {
        filesystem::remove_all(install_dir);
    }

    try {
        if (!filesystem::exists(build_dir)) {
            filesystem::create_directories(build_dir);
        }
        filesystem::touch(started_marker);
        filesystem::create_directories(install_dir);

        output.root = install_dir;
        output.artifacts.clear();

        shared_library::loader_t loader(
            requested_phase.builder_plugin(),
            shared_library::lifetime_t::PROCESS,
            shared_library::symbol_resolution_t::LAZY,
            shared_library::symbol_visibility_t::LOCAL
        );
        const auto symbol_name = std::format("phase__{}", requested_phase.name());
        using fn_t = void (*)(const phase_t*);
        fn_t fn = loader.resolve(symbol_name.c_str());
        fn(static_cast<const phase_t*>(&requested_phase));

        output_t result {
            .root = install_dir,
            .artifacts = {}
        };

        for (const auto& artifact : output.artifacts) {
            const auto installed_artifact = install_dir / artifact.relative_path;
            filesystem::copy(artifact.path, installed_artifact);
            result.artifacts.push_back(output_artifact_t {
                .path = installed_artifact,
                .relative_path = artifact.relative_path
            });
        }

        const auto phase_artifact_dir = requested_phase.artifact_dir();
        const auto latest_dir = m_module.artifact_latest_dir();
        const auto latest_stage_dir = latest_dir / m_module.artifact_dir().relative(phase_artifact_dir);
        const auto latest_stage_tmp_dir = latest_stage_dir + "_tmp";

        if (filesystem::exists(latest_stage_tmp_dir)) {
            filesystem::remove_all(latest_stage_tmp_dir);
        }

        if (!filesystem::exists(latest_dir)) {
            filesystem::create_directories(latest_dir);
        }

        filesystem::create_directory_symlink(phase_artifact_dir, latest_stage_tmp_dir);
        filesystem::rename_replace(latest_stage_tmp_dir, latest_stage_dir);

        filesystem::touch(complete_marker);
        filesystem::remove(started_marker);

        return result;
    } catch (...) {
        filesystem::remove_all(build_dir);
        filesystem::remove_all(install_dir);
        throw ;
    }
}

template <class phase_t>
std::vector<output_t> phase_base_t::build_closure() const {
    const auto* scc = m_module.module_scc;
    if (scc == nullptr) {
        throw std::runtime_error(std::format(
            "kernel::phase::phase_base_t::build_closure: module '{}' has no SCC",
            std::format(
                "{}/{}",
                m_module.workspace->relative_path,
                m_module.module_relative_path_to_workspace
            )
        ));
    }

    std::unordered_set<const graph::module_scc_t*> visited_sccs;
    std::vector<output_t> outputs;
    const auto build_scc = [&]<class self_t>(self_t& self, const graph::module_scc_t& current_scc) -> void {
        if (!visited_sccs.insert(&current_scc).second) {
            return ;
        }

        for (const auto* dependency : current_scc.dependencies) {
            self(self, *dependency);
        }

        for (auto* scc_module : current_scc.modules) {
            output_t output {
                .root = filesystem::path_t("/"),
                .artifacts = {}
            };
            const phase_t requested_phase(*scc_module, m_module_config, output);
            outputs.push_back(requested_phase.template build<phase_t>());
        }
    };

    build_scc(build_scc, *scc);
    return outputs;
}

} // namespace phase

} // namespace kernel

#endif // KERNEL_PHASE_H
