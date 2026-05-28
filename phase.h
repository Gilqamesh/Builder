#ifndef KERNEL_CPP_BUILDER_PHASE_H
# define KERNEL_CPP_BUILDER_PHASE_H

# include "graph.h"

# include <cstdint>
# include <string>
# include <string_view>
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

class module_builder_t;

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
    explicit source_output_t(const filesystem::path_t& source_root);

    filesystem::path_t source_root;
};

struct interface_output_t {
    std::vector<filesystem::path_t> interfaces;
};

struct library_output_t {
    std::vector<filesystem::path_t> libraries;
};

struct binary_output_t {
    explicit binary_output_t(const filesystem::path_t& binary_root);

    filesystem::path_t binary_root;
    filesystem::path_t cli;
};

class phase_base_t : public iphase_t {
public:
    phase_base_t(std::string_view name, module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor);

    std::string_view name() const override;
    const iphase_t* predecessor() const override;
    graph::module_t& module() const override;
    filesystem::path_t artifact_dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;
    std::string producer_symbol_name() const;

    const library_type_t library_type;

    template <class phase_t>
    const typename phase_t::output_t& materialize() const;

protected:
    module_builder_t& module_builder() const;

private:
    std::string_view m_name;
    module_builder_t& m_module_builder;
    graph::module_t& m_module;
    const iphase_t* m_predecessor;
};

template <class phase_t>
class producer_phase_t : public phase_base_t {
protected:
    using phase_base_t::phase_base_t;

private:
    friend class phase_base_t;

    void execute() const;
};

struct source_phase_t : phase_base_t {
    using output_t = source_output_t;

    source_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    void execute() const;

    mutable output_t m_output;
};

struct interface_phase_t : producer_phase_t<interface_phase_t> {
    using output_t = interface_output_t;

    interface_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    mutable output_t m_output;
};

struct library_phase_t : producer_phase_t<library_phase_t> {
    using output_t = library_output_t;

    library_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    mutable output_t m_output;
};

struct binary_phase_t : producer_phase_t<binary_phase_t> {
    using output_t = binary_output_t;

    binary_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

private:
    friend class phase_base_t;

    mutable output_t m_output;
};

class phase_chain_t {
public:
    phase_chain_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type);

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

} // namespace builder

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_PHASE_H
