#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../module_builder/context.h"

int main(int argc, char** argv) {
    auto ctx = load_context_from_argv(argc, argv);

    std::vector<std::filesystem::path> cpp_files;
    for (const auto& entry : std::filesystem::directory_iterator(ctx.module_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".cpp" && entry.path().filename() != "builder.cpp") {
            cpp_files.push_back(entry.path());
        }
    }

    (void)cpp_files;

    auto artifact = ctx.build_root / ctx.module_name / (ctx.module_name + ".executable");
    std::filesystem::create_directories(artifact.parent_path());
    std::ofstream output(artifact);
    output << "dummy executable";
    if (!output) {
        std::cerr << "Failed to create artifact" << std::endl;
        return 1;
    }

    return 0;
}
