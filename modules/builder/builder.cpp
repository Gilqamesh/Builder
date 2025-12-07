#include <builder/api.h>

#include <format>
#include <string>

int main(int argc, char** argv) {
    builder_api_t builder_api = builder_api_t::from_argv(argc, argv);

    const auto builder_bin_path = builder_api.artifact_path(builder_api_t::artifact_type_t::EXECUTABLE);

    if (std::filesystem::exists(builder_bin_path)) {
        const auto time_last_write_builder_cpp = std::filesystem::last_write_time(builder_api.module_dir() / "builder.cpp");
        const auto time_last_write_builder_bin = std::filesystem::last_write_time(builder_bin_path);
        if (time_last_write_builder_cpp <= time_last_write_builder_bin) {
            return 0;
        }
    }

    const int build_result = std::system(
        std::format(
            "g++ -g -O2 -std=c++23 -Wall -Werror -Wextra -I{} -o {} {} {}",
            builder_api.modules_root().string(),
            builder_bin_path.string(),
            (builder_api.module_dir() / "main.cpp").string(),
            (builder_api.module_dir() / "module.cpp").string()
        ).c_str()
    );


    if (build_result) {
        throw std::runtime_error(std::format("module {} failed with exit code {}", builder_api.module_name(), build_result));
    }

    return build_result;
}
