#include "compiler.h"

#include <format>
#include <iostream>
#include <ranges>
#include <chrono>
#include <sstream>

std::filesystem::path compiler_t::update_object_file(
    const std::filesystem::path& source_file,
    const std::vector<std::filesystem::path>& header_files,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::filesystem::path& output_o_file,
    bool position_independent
) {
    if (!std::filesystem::exists(source_file)) {
        throw std::runtime_error(std::format("source file does not exist '{}'", source_file.string()));
    }

    std::filesystem::file_time_type latest_input_file_time = std::filesystem::last_write_time(source_file);
    for (const auto& header_file : header_files) {
        if (!std::filesystem::exists(header_file)) {
            throw std::runtime_error(std::format("header file does not exist '{}'", header_file.string()));
        }
        latest_input_file_time = std::max(latest_input_file_time, std::filesystem::last_write_time(header_file));
    }

    if (std::filesystem::exists(output_o_file) && latest_input_file_time <= std::filesystem::last_write_time(output_o_file)) {
        return output_o_file;
    }

    std::string define_flags;
    for (const auto& [key, value] : define_key_values) {
        define_flags += std::format("-D{}={} ", key, value);
    }

    std::string compile_command = std::format("clang++ {}-g {}-std=c++23 -c {} -o {}", define_flags.empty() ? "" : define_flags, position_independent ? "-fPIC " : "", source_file.string(), output_o_file.string());
    for (const auto& include_dir : include_dirs) {
        compile_command += " -I" + include_dir.string();
    }

    std::cout << compile_command << std::endl;
    const int result = std::system(compile_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to compile '{}'", source_file.string()));
    }

    return output_o_file;
}

std::filesystem::path compiler_t::update_static_library(
    const std::vector<std::filesystem::path>& objects,
    const std::filesystem::path& output_static_library
) {
    std::filesystem::file_time_type latest_object_time = std::filesystem::file_time_type::min();
    std::string archive_command = "ar rcs " + output_static_library.string();
    for (const auto& object : objects) {
        if (!std::filesystem::exists(object)) {
            throw std::runtime_error(std::format("object does not exist '{}'", object.string()));
        }
        latest_object_time = std::max(latest_object_time, std::filesystem::last_write_time(object));
        archive_command += " " + object.string();
    }

    if (std::filesystem::exists(output_static_library) && latest_object_time <= std::filesystem::last_write_time(output_static_library)) {
        return output_static_library;
    }

    std::cout << archive_command << std::endl;
    const int result = std::system(archive_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to create archive '{}'", output_static_library.string()));
    }

    return output_static_library;
}

std::filesystem::path compiler_t::bundle_static_libraries(const std::vector<std::filesystem::path>& archives, const std::filesystem::path& output_static_library) {
    if (archives.empty()) {
        throw std::runtime_error(std::format("empty archives list provided to bundle into '{}'", output_static_library.string()));
    }

    const auto tmp_dir = std::filesystem::temp_directory_path() / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    if (!std::filesystem::create_directories(tmp_dir)) {
        throw std::runtime_error(std::format("failed to create temporary directory '{}'", tmp_dir.string()));
    }

    struct guard_t {
        guard_t(const std::filesystem::path& dir): dir(dir) {}
        ~guard_t() noexcept {
            std::error_code ec;
            std::filesystem::remove_all(dir, ec);
        }
        std::filesystem::path dir;
    } guard(tmp_dir);

    std::filesystem::file_time_type latest_archive_time = std::filesystem::file_time_type::min();
    const auto out_lib_path = tmp_dir / "out.lib";

    std::vector<std::filesystem::path> links;
    links.reserve(archives.size());

    for (size_t i = 0; i < archives.size(); ++i) {
        const auto& archive = archives[i];
        if (!std::filesystem::exists(archive)) {
            throw std::runtime_error(std::format("archive does not exist '{}'", archive.string()));
        }
        latest_archive_time = std::max(latest_archive_time, std::filesystem::last_write_time(archive));
        const auto link = tmp_dir / ("lib" + std::to_string(i) + ".a");
        std::cout << std::format("ln -s {} {}", archive.string(), link.string()) << std::endl;
        std::error_code ec;
        std::filesystem::create_symlink(std::filesystem::absolute(archive), link, ec);
        if (ec) {
            throw std::runtime_error(std::format("failed to create symlink from '{}' to '{}', error: {}", archive.string(), link.string(), ec.message()));
        }
        links.push_back(link);
    }

    std::ostringstream bundle_command;
    bundle_command << "cd '" << tmp_dir.string() << "' && ";
    bundle_command << "printf \"CREATE %s\\n";
    for (size_t i = 0; i < links.size(); ++i) {
        bundle_command << "ADDLIB %s\\n";
    }
    bundle_command << "SAVE\\nEND\\n\" ";

    bundle_command << out_lib_path.string() << " ";
    for (const auto& link : links) {
        bundle_command << link.filename().string() << " ";
    }

    bundle_command << "| ar -M";

    std::cout << bundle_command.str() << std::endl;
    const int result = std::system(bundle_command.str().c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to bundle static libraries into '{}'", output_static_library.string()));
    }

    std::cout << std::format("cp {} {}", out_lib_path.string(), output_static_library.string()) << std::endl;
    std::filesystem::copy_file(out_lib_path, output_static_library, std::filesystem::copy_options::overwrite_existing);

    std::cout << std::format("rm {}", out_lib_path.string()) << std::endl;
    std::filesystem::remove(out_lib_path);

    return output_static_library;
}

std::filesystem::path compiler_t::update_shared_libary(
    const std::vector<std::filesystem::path>& input_files,
    const std::filesystem::path& output_shared_libary
) {
    std::filesystem::file_time_type latest_input_file_time = std::filesystem::file_time_type::min();
    std::string link_command = "clang++ -shared -o " + output_shared_libary.string();
    for (const auto& input_file : input_files) {
        if (!std::filesystem::exists(input_file)) {
            throw std::runtime_error(std::format("input file does not exist '{}'", input_file.string()));
        }
        latest_input_file_time = std::max(latest_input_file_time, std::filesystem::last_write_time(input_file));
        link_command += " " + input_file.string();
    }

    if (std::filesystem::exists(output_shared_libary) && latest_input_file_time <= std::filesystem::last_write_time(output_shared_libary)) {
        return output_shared_libary;
    }

    std::cout << link_command << std::endl;
    const int result = std::system(link_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to create shared library '{}'", output_shared_libary.string()));
    }

    return output_shared_libary;
}

std::filesystem::path compiler_t::update_binary(const std::vector<binary_input_t>& input_libraries, const std::filesystem::path& output_binary) {
    std::filesystem::file_time_type latest_input_library_time = std::filesystem::file_time_type::min();
    std::string link_command = "clang++ -o " + output_binary.string();
    const auto input_library_paths = [&]() {
        std::vector<std::filesystem::path> paths;
        for (const auto& input_library : input_libraries) {
            std::visit([&](const auto& v) {
                using T = std::decay_t<decltype(v)>;

                const auto str_to_paths = [](const std::string& str) {
                    std::vector<std::filesystem::path> result;
                    for (auto&& word : std::views::split(str, ' ')) {

                        if (std::ranges::empty(word)) {
                            continue ;
                        }

                        result.emplace_back(std::string(word.begin(), word.end()));
                    }
                    return result;
                };
                if constexpr (std::is_same_v<T, std::vector<std::filesystem::path>>) {
                    for (const auto& path : v) {
                        paths.push_back(path);
                    }
                } else if constexpr (std::is_same_v<T, std::string>) {
                    const auto subpaths = str_to_paths(v);
                    paths.insert(
                        paths.end(),
                        std::move_iterator(subpaths.begin()),
                        std::move_iterator(subpaths.end())
                    );
                } else if constexpr (std::is_same_v<T, std::filesystem::path>) {
                    paths.push_back(v);
                } else if constexpr (std::is_same_v<T, const char*>) {
                    const auto subpaths = str_to_paths(std::string(v));
                    paths.insert(
                        paths.end(),
                        std::move_iterator(subpaths.begin()),
                        std::move_iterator(subpaths.end())
                    );
                } else {
                    static_assert(std::false_type::value, "Unhandled variant alternative");
                }
            }, input_library);
        }
        return paths;
    }();

    for (const auto& input_library_path : input_library_paths) {
        if (!std::filesystem::exists(input_library_path)) {
            throw std::runtime_error(std::format("input file does not exist '{}'", input_library_path.string()));
        }
        latest_input_library_time = std::max(latest_input_library_time, std::filesystem::last_write_time(input_library_path));
        link_command += " " + input_library_path.string();
    }

    if (std::filesystem::exists(output_binary) && latest_input_library_time <= std::filesystem::last_write_time(output_binary)) {
        return output_binary;
    }

    std::cout << link_command << std::endl;
    const int result = std::system(link_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to create binary '{}'", output_binary.string()));
    }

    return output_binary;
}
