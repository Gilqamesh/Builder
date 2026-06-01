#ifndef KERNEL_COMPILER_H
# define KERNEL_COMPILER_H

# include "filesystem.h"

# include <cstdint>
# include <string>
# include <vector>

namespace kernel {

namespace compiler {

#ifndef KERNEL_CXX_COMPILER_PATH
# define KERNEL_CXX_COMPILER_PATH "/usr/bin/clang++"
#endif

#ifndef KERNEL_CC_COMPILER_PATH
# define KERNEL_CC_COMPILER_PATH "/usr/bin/clang"
#endif

#ifndef KERNEL_AR_PATH
# define KERNEL_AR_PATH "/usr/bin/ar"
#endif

enum class library_type_t : uint8_t {
    STATIC,
    SHARED
};

class define_t {
public:
    define_t(std::string key, std::string value);

    const std::string& key() const;
    const std::string& replacement() const;

private:
    std::string m_key;
    std::string m_replacement;
};

struct link_input_group_t {
    std::vector<filesystem::path_t> libraries;
    bool static_library_group;
};

struct link_inputs_t {
    std::vector<link_input_group_t> groups;
};

filesystem::path_t create_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    library_type_t library_type,
    const link_inputs_t& link_inputs,
    const filesystem::path_t& output_path
);

filesystem::path_t create_binary(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const filesystem::path_t& output_path
);

} // namespace compiler

} // namespace kernel

#endif // KERNEL_COMPILER_H
