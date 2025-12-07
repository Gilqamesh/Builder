#ifndef BUILDER_MODULE_H
# define BUILDER_MODULE_H

# include <filesystem>
# include <string>
# include <unordered_map>
# include <stack>
# include <iostream>

class module_t {
public:
    enum class construct_state_t {
        NOT_CONSTRUCTED,
        CONSTRUCTING,
        CONSTRUCTED
    };

    enum class build_state_t {
        NOT_BUILT,
        BUILDING,
        BUILT
    };

    enum class run_state_t {
        NOT_RAN,
        RUNNING,
        RAN
    };

public:
    module_t(
        const std::string name,
        const std::filesystem::path& builder_cpp_path,
        const std::filesystem::path& artifacts_root,
        const std::filesystem::path& modules_root
    );

    const std::string& name() const;
    const std::filesystem::path& builder_cpp_path() const;
    const std::unordered_map<std::string, module_t*>& dependencies() const;

    void print_dependencies(std::ostream& os) const;
    void construct(std::ostream& os, std::unordered_map<std::string, module_t*>& module_by_name);
    void build(std::ostream& os);
    void run(std::ostream& os);

private:
    void print_dependencies(std::ostream& os, int depth) const;
    void construct(std::ostream& os, std::unordered_map<std::string, module_t*>& module_by_name, std::stack<module_t*>& processed_stack);

private:
    std::string m_name;
    std::filesystem::path m_builder_cpp_path;
    const std::filesystem::path& m_artifacts_root;
    const std::filesystem::path& m_modules_root;

    const std::filesystem::path m_artifact_path;
    std::unordered_map<std::string, module_t*> m_dependencies;
    construct_state_t m_construct_state;
    build_state_t m_build_state;
    run_state_t m_run_state;
};

#endif // BUILDER_MODULE_H
