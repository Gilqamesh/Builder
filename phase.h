#ifndef KERNEL_CPP_BUILDER_PHASE_H
# define KERNEL_CPP_BUILDER_PHASE_H

# include "graph.h"

# include <cstdint>
# include <format>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <unordered_set>
# include <vector>

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

struct iphase_t {
    virtual ~iphase_t() = default;

    virtual std::string_view name() const = 0;
    virtual const iphase_t* predecessor() const = 0;
    virtual graph::module_t& module() const = 0;

    virtual filesystem::path_t artifact_dir() const = 0;
    virtual filesystem::path_t build_dir() const = 0;
    virtual filesystem::path_t install_dir() const = 0;
};

struct config_phase_t;
struct source_phase_t;
struct interface_phase_t;
struct library_phase_t;
struct binary_phase_t;

class phase_base_t : public iphase_t {
public:
    phase_base_t(
        std::string_view name,
        graph::module_t& module,
        library_type_t library_type,
        const iphase_t* predecessor
    );

    std::string_view name() const override;
    const iphase_t* predecessor() const override;
    graph::module_t& module() const override;
    filesystem::path_t artifact_dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;
    library_type_t library_type() const;

    template <class phase_t>
    typename phase_t::output_t materialize() const;

    template <class phase_t>
    std::vector<typename phase_t::output_t> materialize_all() const;

protected:
    void declare_output(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const;

private:
    struct declared_output_t {
        filesystem::path_t artifact;
        filesystem::relative_path_t relative_install_path;
    };

    void execute() const;
    std::string producer_symbol_name() const;

    template <class phase_t>
    void materialize_all(
        const graph::module_scc_t& scc,
        std::unordered_set<const graph::module_scc_t*>& visited_sccs,
        std::vector<typename phase_t::output_t>& outputs
    ) const;

    std::string_view m_name;
    graph::module_t& m_module;
    const library_type_t m_library_type;
    const iphase_t* m_predecessor;
    mutable std::vector<declared_output_t> m_declared_outputs;
};

struct config_phase_t : phase_base_t {
    struct output_t {
        library_type_t library_type;
    };

    config_phase_t(
        graph::module_t& module,
        library_type_t library_type
    );
};

struct source_phase_t : phase_base_t {
    struct output_t {
        std::vector<filesystem::path_t> sources;
    };

    source_phase_t(
        graph::module_t& module,
        library_type_t library_type,
        const iphase_t* predecessor
    );

    void add_source(const filesystem::path_t& source, const filesystem::relative_path_t& relative_install_path) const;
};

struct interface_phase_t : phase_base_t {
    struct output_t {
        std::vector<filesystem::path_t> interfaces;
    };

    interface_phase_t(
        graph::module_t& module,
        library_type_t library_type,
        const iphase_t* predecessor
    );

    filesystem::relative_path_t source_relative_path(const filesystem::path_t& source) const;
    void add_interface(const filesystem::path_t& interface) const;
    void add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path) const;
};

struct library_phase_t : phase_base_t {
    struct output_t {
        std::vector<filesystem::path_t> libraries;
    };

    library_phase_t(
        graph::module_t& module,
        library_type_t library_type,
        const iphase_t* predecessor
    );

    void add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const;
};

struct binary_phase_t : phase_base_t {
    struct output_t {
        filesystem::path_t binary_root;
        filesystem::path_t cli;
    };

    binary_phase_t(
        graph::module_t& module,
        library_type_t library_type,
        const iphase_t* predecessor
    );

    void add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const;
};

class module_phases_t {
public:
    module_phases_t(graph::module_t& module, library_type_t library_type);

    config_phase_t config;
    source_phase_t source;
    interface_phase_t interface;
    library_phase_t library;
    binary_phase_t binary;
};

BUILDER_EXTERN void phase__source(const source_phase_t* phase);
BUILDER_EXTERN void phase__interface(const interface_phase_t* phase);
BUILDER_EXTERN void phase__library(const library_phase_t* phase);
BUILDER_EXTERN void phase__binary(const binary_phase_t* phase);

template <class phase_t>
typename phase_t::output_t phase_base_t::materialize() const {
    const auto* current_phase = this;
    const phase_t* requested_phase = nullptr;

    while (requested_phase == nullptr) {
        requested_phase = dynamic_cast<const phase_t*>(current_phase);
        if (requested_phase != nullptr) {
            break ;
        }

        const auto* previous_phase = current_phase->predecessor();
        if (previous_phase == nullptr) {
            throw std::runtime_error(std::format(
                "phase_base_t::materialize: requested phase is not in the current phase '{}' or its configured predecessor chain",
                name()
            ));
        }

        current_phase = dynamic_cast<const phase_base_t*>(previous_phase);
        if (current_phase == nullptr) {
            throw std::runtime_error(std::format(
                "phase_base_t::materialize: predecessor of phase '{}' is not a phase_base_t",
                name()
            ));
        }
    }

    const auto& phase = *requested_phase;
    const auto build_dir = phase.build_dir();
    const auto install_dir = phase.install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("{}.{}", phase.name(), state));
    };
    const auto publish_latest_stage = [&]() {
        const auto phase_artifact_dir = phase.artifact_dir();
        const auto latest_dir = phase.module().artifact_latest_dir();
        const auto latest_stage_dir = latest_dir / phase.module().artifact_dir().relative(phase_artifact_dir);
        const auto latest_stage_tmp_dir = latest_stage_dir + "_tmp";

        if (filesystem::exists(latest_stage_tmp_dir)) {
            filesystem::remove_all(latest_stage_tmp_dir);
        }

        if (!filesystem::exists(latest_dir)) {
            filesystem::create_directories(latest_dir);
        }

        filesystem::create_directory_symlink(phase_artifact_dir, latest_stage_tmp_dir);
        filesystem::rename_replace(latest_stage_tmp_dir, latest_stage_dir);
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");
    const auto installed_output = [&]() -> typename phase_t::output_t {
        if constexpr (std::is_same_v<phase_t, config_phase_t>) {
            return config_phase_t::output_t {
                .library_type = phase.library_type()
            };
        } else if constexpr (std::is_same_v<phase_t, source_phase_t>) {
            return source_phase_t::output_t {
                .sources = filesystem::find(install_dir, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)
            };
        } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
            return interface_phase_t::output_t {
                .interfaces = { install_dir }
            };
        } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
            return library_phase_t::output_t {
                .libraries = filesystem::find(install_dir, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)
            };
        } else if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
            const auto cli = install_dir / filesystem::relative_path_t("cli");

            if (!filesystem::exists(cli)) {
                throw std::runtime_error(std::format("phase_base_t::materialize: expected module cli '{}' to exist", cli));
            }

            return binary_phase_t::output_t {
                .binary_root = install_dir,
                .cli = cli
            };
        } else {
            static_assert(std::is_same_v<phase_t, void>, "unsupported phase type");
        }
    };

    if (filesystem::exists(complete_marker)) {
        auto result = installed_output();
        publish_latest_stage();
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

        static_cast<const phase_base_t&>(phase).m_declared_outputs.clear();
        static_cast<const phase_base_t&>(phase).execute();

        for (const auto& output : static_cast<const phase_base_t&>(phase).m_declared_outputs) {
            filesystem::copy(output.artifact, install_dir / output.relative_install_path);
        }

        auto result = installed_output();

        publish_latest_stage();
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
std::vector<typename phase_t::output_t> phase_base_t::materialize_all() const {
    const auto* scc = module().module_scc;
    if (scc == nullptr) {
        throw std::runtime_error(std::format(
            "phase_base_t::materialize_all: module '{}' has no SCC",
            std::format(
                "{}/{}",
                module().workspace->workspace_relative_path_to_workspace_ecosystem.string(),
                module().module_relative_path_to_workspace.string()
            )
        ));
    }

    std::unordered_set<const graph::module_scc_t*> visited_sccs;
    std::vector<typename phase_t::output_t> outputs;
    materialize_all<phase_t>(*scc, visited_sccs, outputs);
    return outputs;
}

template <class phase_t>
void phase_base_t::materialize_all(
    const graph::module_scc_t& scc,
    std::unordered_set<const graph::module_scc_t*>& visited_sccs,
    std::vector<typename phase_t::output_t>& outputs
) const {
    if (!visited_sccs.insert(&scc).second) {
        return ;
    }

    for (const auto* dependency : scc.dependencies) {
        materialize_all<phase_t>(*dependency, visited_sccs, outputs);
    }

    for (auto* module : scc.modules) {
        if constexpr (std::is_same_v<phase_t, config_phase_t>) {
            outputs.push_back(module->config_phase(library_type()).template materialize<phase_t>());
        } else if constexpr (std::is_same_v<phase_t, source_phase_t>) {
            outputs.push_back(module->source_phase(library_type()).template materialize<phase_t>());
        } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
            outputs.push_back(module->interface_phase(library_type()).template materialize<phase_t>());
        } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
            outputs.push_back(module->library_phase(library_type()).template materialize<phase_t>());
        } else if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
            outputs.push_back(module->binary_phase(library_type()).template materialize<phase_t>());
        } else {
            static_assert(std::is_same_v<phase_t, void>, "unsupported phase type");
        }
    }
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_PHASE_H
