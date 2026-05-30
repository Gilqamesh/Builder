#ifndef KERNEL_CPP_BUILDER_PHASE_H
# define KERNEL_CPP_BUILDER_PHASE_H

# include "graph.h"

# include <cstdint>
# include <format>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <vector>

# include "shared_library.h"

namespace kernel {

namespace cpp_builder {

namespace builder {

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

enum class library_type_t : uint8_t {
    STATIC,
    SHARED
};

struct output_artifact_t {
    filesystem::path_t path;
    filesystem::relative_path_t relative_path;
};

struct output_t {
    filesystem::path_t root;
    std::vector<output_artifact_t> artifacts;
};

struct link_input_group_t {
    std::vector<filesystem::path_t> libraries;
    bool static_library_group;
};

struct link_inputs_t {
    std::vector<link_input_group_t> groups;
};

class phase_base_t {
public:
    phase_base_t(
        std::string_view name,
        graph::module_t& module,
        library_type_t library_type
    );

    std::string_view name() const;
    filesystem::path_t artifact_dir() const;
    filesystem::path_t build_dir() const;
    filesystem::path_t install_dir() const;
    library_type_t library_type() const;

    template <class phase_t>
    output_t materialize() const;

    template <class phase_t>
    std::vector<output_t> materialize_all() const;

private:
    std::string_view m_name;

protected:
    graph::module_t& m_module;

private:
    const library_type_t m_library_type;

protected:
    mutable output_t m_output;

private:
    template <class phase_t>
    void execute(const phase_t& phase) const;

    std::string producer_symbol_name() const;

};

struct source_phase_t : phase_base_t {
    source_phase_t(
        graph::module_t& module,
        library_type_t library_type
    );

    filesystem::path_t source_dir() const;
    void add_source(const filesystem::path_t& source, const filesystem::relative_path_t& relative_install_path) const;
};

struct interface_phase_t : phase_base_t {
    interface_phase_t(
        graph::module_t& module,
        library_type_t library_type
    );

    void add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& module_relative_install_path) const;
};

struct library_phase_t : phase_base_t {
    library_phase_t(
        graph::module_t& module,
        library_type_t library_type
    );

    link_inputs_t link_inputs() const;
    void add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const;
};

struct binary_phase_t : phase_base_t {
    binary_phase_t(
        graph::module_t& module,
        library_type_t library_type
    );

    link_inputs_t link_inputs() const;
    void add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const;
};

struct configured_module_t {
    configured_module_t(
        graph::module_t& module,
        library_type_t library_type
    );

    source_phase_t source;
    interface_phase_t interface;
    library_phase_t library;
    binary_phase_t binary;

    template <class phase_t>
    output_t materialize() const;
};

BUILDER_EXTERN void phase__source(const source_phase_t* phase);
BUILDER_EXTERN void phase__interface(const interface_phase_t* phase);
BUILDER_EXTERN void phase__library(const library_phase_t* phase);
BUILDER_EXTERN void phase__binary(const binary_phase_t* phase);

template <class phase_t>
output_t phase_base_t::materialize() const {
    static_assert(
        std::is_same_v<phase_t, source_phase_t>
        || std::is_same_v<phase_t, interface_phase_t>
        || std::is_same_v<phase_t, library_phase_t>
        || std::is_same_v<phase_t, binary_phase_t>,
        "phase_base_t::materialize: unsupported phase type"
    );

    const auto& configured_module = m_module.configured_module(library_type());
    const phase_t* requested_phase = nullptr;
    if constexpr (std::is_same_v<phase_t, source_phase_t>) {
        requested_phase = &configured_module.source;
    } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
        requested_phase = &configured_module.interface;
    } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
        requested_phase = &configured_module.library;
    } else if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
        requested_phase = &configured_module.binary;
    }

    const auto& phase = *requested_phase;
    const auto build_dir = phase.build_dir();
    const auto install_dir = phase.install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("{}.{}", phase.name(), state));
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
        throw std::runtime_error(std::format("phase_base_t::materialize: re-entry detected for phase '{}'", phase.name()));
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

        auto& output = static_cast<const phase_base_t&>(phase).m_output;
        output.artifacts.clear();
        static_cast<const phase_base_t&>(phase).template execute<phase_t>(phase);

        for (const auto& artifact : output.artifacts) {
            filesystem::copy(artifact.path, install_dir / artifact.relative_path);
        }

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

        const auto& phase_base = static_cast<const phase_base_t&>(phase);
        const auto phase_artifact_dir = phase.artifact_dir();
        const auto latest_dir = phase_base.m_module.artifact_latest_dir();
        const auto latest_stage_dir = latest_dir / phase_base.m_module.artifact_dir().relative(phase_artifact_dir);
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
output_t configured_module_t::materialize() const {
    static_assert(
        std::is_same_v<phase_t, source_phase_t>
        || std::is_same_v<phase_t, interface_phase_t>
        || std::is_same_v<phase_t, library_phase_t>
        || std::is_same_v<phase_t, binary_phase_t>,
        "configured_module_t::materialize: unsupported phase type"
    );

    if constexpr (std::is_same_v<phase_t, source_phase_t>) {
        return source.template materialize<phase_t>();
    } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
        return interface.template materialize<phase_t>();
    } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
        return library.template materialize<phase_t>();
    } else if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
        return binary.template materialize<phase_t>();
    }
}

template <class phase_t>
std::vector<output_t> phase_base_t::materialize_all() const {
    return m_module.materialize_all<phase_t>(library_type());
}

template <class phase_t>
void phase_base_t::execute(const phase_t& phase) const {
    shared_library::loader_t loader(m_module.materialize_builder_plugin(), shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const phase_t*);
    fn_t fn = loader.resolve(producer_symbol_name().c_str());
    fn(&phase);
}

} // namespace builder

namespace graph {

template <class phase_t>
builder::output_t module_t::materialize(builder::library_type_t library_type) const {
    return configured_module(library_type).template materialize<phase_t>();
}

template <class phase_t>
std::vector<builder::output_t> module_t::materialize_all(builder::library_type_t library_type) const {
    const auto* scc = module_scc;
    if (scc == nullptr) {
        throw std::runtime_error(std::format(
            "kernel::cpp_builder::graph::module_t::materialize_all: module '{}' has no SCC",
            std::format(
                "{}/{}",
                workspace->workspace_relative_path_to_workspace_ecosystem.string(),
                module_relative_path_to_workspace.string()
            )
        ));
    }

    std::unordered_set<const module_scc_t*> visited_sccs;
    std::vector<builder::output_t> outputs;
    const auto materialize_scc = [&]<class self_t>(self_t& self, const module_scc_t& current_scc) -> void {
        if (!visited_sccs.insert(&current_scc).second) {
            return ;
        }

        for (const auto* dependency : current_scc.dependencies) {
            self(self, *dependency);
        }

        for (auto* module : current_scc.modules) {
            outputs.push_back(module->materialize<phase_t>(library_type));
        }
    };

    materialize_scc(materialize_scc, *scc);
    return outputs;
}

} // namespace graph

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_PHASE_H
