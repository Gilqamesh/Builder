#ifndef MODULE_H
# define MODULE_H

# include <string>
# include <filesystem>
# include <unordered_map>
# include <unordered_set>

class module_t {
public:
    module_t(const std::string& name, const std::filesystem::path module_cpp_path);

    const std::string& name() const;
    const std::filesystem::path& module_cpp_path() const;
    std::unordered_map<std::string, module_t*>& dependencies();

    void print_dependencies() const;

private:
    void print_dependencies(int depth) const;

private:
    std::string m_name;
    std::filesystem::path m_module_cpp_path;
    std::unordered_map<std::string, module_t*> m_dependencies;
};

#endif // MODULE_H
