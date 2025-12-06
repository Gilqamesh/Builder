#include <filesystem>
#include <iostream>
#include <vector>

#include <module/dep_one/dep_one.h>
#include <module/dep_two/dep_two.h>

#include "compiler_gcc/compiler.h"
#include "module_builder/context.h"

int main(int argc, char **argv) {
    module_builder::Context ctx = module_builder::load_context(argc, argv);

    std::vector<std::filesystem::path> sources;
    for (const auto &entry : std::filesystem::directory_iterator(ctx.module_dir)) {
        if (entry.path().filename() == "builder.cpp") {
            continue;
        }
        if (entry.path().extension() == ".cpp") {
            sources.push_back(entry.path());
        }
    }

    compiler_gcc::BuildConfig config;
    std::vector<std::string> deps = {"dep_one", "dep_two"};

    compiler_gcc::build_static_library(ctx, ctx.module_name, sources, deps, config);

    std::cout << "Built example_module static library" << std::endl;
    return 0;
}
