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

struct source_output_t {
    filesystem::path_t source_root;
};

struct interface_output_t {
    std::vector<filesystem::path_t> interfaces;
};

struct library_output_t {
    std::vector<filesystem::path_t> libraries;
};

struct binary_output_t {
    filesystem::path_t binary_root;
    filesystem::path_t cli;
};

class phase_base_t : public iphase_t {
public:
    phase_base_t(std::string_view name, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor);

    std::string_view name() const override;
    const iphase_t* predecessor() const override;
    graph::module_t& module() const override;
    filesystem::path_t artifact_dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;
    library_type_t library_type() const;

    template <class phase_t>
    std::vector<typename phase_t::output_t> materialize() const;

private:
    std::string producer_symbol_name() const;

    template <class phase_t>
    const phase_t& exact_phase() const;

    template <class phase_t>
    std::vector<typename phase_t::output_t> materialize(std::unordered_set<const graph::module_scc_t*>& visited_sccs) const;

    template <class phase_t>
    typename phase_t::output_t materialize_exact() const;

    template <class phase_t>
    void append_materialized_scc_outputs(const graph::module_scc_t& scc, std::unordered_set<const graph::module_scc_t*>& visited_sccs, std::vector<typename phase_t::output_t>& outputs) const;

    std::string_view m_name;
    graph::module_t& m_module;
    const library_type_t m_library_type;
    const iphase_t* m_predecessor;
};

struct source_phase_t : phase_base_t {
    using output_t = source_output_t;

    source_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    void execute() const;
};

struct interface_phase_t : phase_base_t {
    using output_t = interface_output_t;

    interface_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    void execute() const;
};

struct library_phase_t : phase_base_t {
    using output_t = library_output_t;

    library_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    void execute() const;
};

struct binary_phase_t : phase_base_t {
    using output_t = binary_output_t;

    binary_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    void execute() const;
};

class phase_chain_t {
public:
    phase_chain_t(graph::module_t& module, library_type_t library_type);

    source_phase_t source;
    interface_phase_t interface;
    library_phase_t library;
    binary_phase_t binary;
};

void install_interface(const interface_phase_t& phase, const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path);
void install_library(const library_phase_t& phase, const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path);
void install_binary(const binary_phase_t& phase, const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path);

BUILDER_EXTERN void phase__interface(const interface_phase_t* phase);
BUILDER_EXTERN void phase__library(const library_phase_t* phase);
BUILDER_EXTERN void phase__binary(const binary_phase_t* phase);

source_output_t materialized_output(const source_phase_t& phase);
interface_output_t materialized_output(const interface_phase_t& phase);
library_output_t materialized_output(const library_phase_t& phase);
binary_output_t materialized_output(const binary_phase_t& phase);

namespace detail {

inline std::string module_display_name(const graph::module_t& module) {
    return std::format("{}/{}", module.workspace->workspace_relative_path_to_workspace_ecosystem.string(), module.module_relative_path_to_workspace.string());
}

template <class>
inline constexpr bool always_false_v = false;

template <class phase_t>
const phase_t& phase_from_chain(const phase_chain_t& phase_chain) {
    if constexpr (std::is_same_v<phase_t, source_phase_t>) {
        return phase_chain.source;
    } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
        return phase_chain.interface;
    } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
        return phase_chain.library;
    } else if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
        return phase_chain.binary;
    } else {
        static_assert(always_false_v<phase_t>, "unsupported phase type");
    }
}

} // namespace detail

template <class phase_t>
const phase_t& phase_base_t::exact_phase() const {
    const auto* phase = dynamic_cast<const phase_t*>(this);
    if (phase == nullptr) {
        const auto* previous_phase = predecessor();
        if (previous_phase == nullptr) {
            throw std::runtime_error(std::format("phase_base_t::exact_phase: requested phase is not in the current phase '{}' or its configured predecessor chain", name()));
        }

        const auto* previous_phase_base = dynamic_cast<const phase_base_t*>(previous_phase);
        if (previous_phase_base == nullptr) {
            throw std::runtime_error(std::format("phase_base_t::exact_phase: predecessor of phase '{}' is not a phase_base_t", name()));
        }

        return previous_phase_base->exact_phase<phase_t>();
    }

    return *phase;
}

template <class phase_t>
typename phase_t::output_t phase_base_t::materialize_exact() const {
    const auto& phase = exact_phase<phase_t>();
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

    if (filesystem::exists(complete_marker)) {
        auto result = materialized_output(phase);
        publish_latest_stage();
        return result;
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("phase_base_t::materialize_exact: re-entry detected for phase '{}'", phase.name()));
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

        phase.execute();
        auto result = materialized_output(phase);

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
std::vector<typename phase_t::output_t> phase_base_t::materialize() const {
    std::unordered_set<const graph::module_scc_t*> visited_sccs;
    return materialize<phase_t>(visited_sccs);
}

template <class phase_t>
std::vector<typename phase_t::output_t> phase_base_t::materialize(std::unordered_set<const graph::module_scc_t*>& visited_sccs) const {
    const auto* scc = module().module_scc;
    if (scc == nullptr) {
        throw std::runtime_error(std::format("phase_base_t::materialize: module '{}' has no SCC", detail::module_display_name(module())));
    }

    std::vector<typename phase_t::output_t> outputs;
    append_materialized_scc_outputs<phase_t>(*scc, visited_sccs, outputs);
    return outputs;
}

template <class phase_t>
void phase_base_t::append_materialized_scc_outputs(const graph::module_scc_t& scc, std::unordered_set<const graph::module_scc_t*>& visited_sccs, std::vector<typename phase_t::output_t>& outputs) const {
    if (!visited_sccs.insert(&scc).second) {
        return ;
    }

    for (const auto* dependency : scc.dependencies) {
        append_materialized_scc_outputs<phase_t>(*dependency, visited_sccs, outputs);
    }

    for (auto* module : scc.modules) {
        phase_chain_t phase_chain(*module, library_type());
        outputs.push_back(detail::phase_from_chain<phase_t>(phase_chain).template materialize_exact<phase_t>());
    }
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_PHASE_H
