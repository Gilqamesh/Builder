#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "module_builder/api.h"
#include "module_builder/context.h"

int main(int argc, char** argv) {
    module_builder::context_t ctx = module_builder::load_context_from_argv(argc, argv);

    std::vector<std::filesystem::path> sources;
    for (const auto& entry : std::filesystem::directory_iterator(ctx.module_dir)) {
        if (entry.path().filename() == "builder.cpp") {
            continue;
        }
        if (entry.path().extension() == ".cpp") {
            sources.push_back(entry.path());
        }
    }

    std::cout << "[example_module] sources:" << std::endl;
    for (const auto& path : sources) {
        std::cout << "  - " << path << std::endl;
    }

    std::filesystem::path artifact = module_builder::artifact_path(
        ctx, ctx.module_name, module_builder::artifact_kind_t::EXECUTABLE);
    std::filesystem::create_directories(artifact.parent_path());

    std::ofstream out(artifact);
    out << "dummy executable for " << ctx.module_name << "\n";
    out.close();

    std::cout << "[example_module] wrote artifact to " << artifact << std::endl;
    return 0;
}

