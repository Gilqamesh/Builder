#ifndef BUILDER_API_H
# define BUILDER_API_H

# include <filesystem>
# include <string>
# include <format>

class builder_api_t {
public:
    enum class artifact_type_t {
        STATIC_LIBRARY,
        SHARED_LIBRARY,
        EXECUTABLE
    };

public:
    inline builder_api_t(const std::filesystem::path& artifacts_root, const std::filesystem::path& modules_root, const std::string& module_name):
        m_artifacts_root(std::filesystem::canonical(artifacts_root)),
        m_modules_root(std::filesystem::canonical(modules_root)),
        m_module_name(module_name)
    {
    }

    inline static builder_api_t from_argv(int argc, char** argv) {
        if (argc != 4) {
            throw std::runtime_error(std::format("usage: {} <artifacts_root> <modules_root> <module_name>", argv[0]));
        }

        return builder_api_t(argv[1], argv[2], argv[3]);
    }

    inline const std::filesystem::path& artifacts_root() const {
        return m_artifacts_root;
    }

    inline const std::filesystem::path& modules_root() const {
        return m_modules_root;
    }
    
    inline const std::string& module_name() const {
        return m_module_name;
    }

    inline std::filesystem::path module_dir() const {
        return m_modules_root / m_module_name;
    }

    inline std::filesystem::path artifact_path(artifact_type_t type) const {
        switch (type) {
            case artifact_type_t::STATIC_LIBRARY: {
                return m_artifacts_root / (m_module_name + ".static_library");
            } break ;
            case artifact_type_t::SHARED_LIBRARY: {
                return m_artifacts_root / (m_module_name + ".shared_library");
            } break ;
            case artifact_type_t::EXECUTABLE: {
                return m_artifacts_root / (m_module_name + ".executable");
            } break ;
            default: throw std::runtime_error("unhandled artifact type");
        }
    }

private:
    std::filesystem::path m_artifacts_root;
    std::filesystem::path m_modules_root;
    std::string m_module_name;
};

#endif // BUILDER_API_H
