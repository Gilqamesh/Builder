#include "compiler.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace compiler_gcc {
namespace {

std::filesystem::path object_path(const module_builder::Context &ctx,
                                  const std::string &module_name,
                                  const std::filesystem::path &source) {
    auto filename = source.filename();
    return ctx.build_root / module_name / filename.replace_extension(".o");
}

bool is_outdated(const std::filesystem::path &output, const std::filesystem::path &input) {
    return !std::filesystem::exists(output) ||
           std::filesystem::last_write_time(input) > std::filesystem::last_write_time(output);
}

}  // namespace

std::filesystem::path build_static_library(const module_builder::Context &ctx,
                                           const std::string &module_name,
                                           const std::vector<std::filesystem::path> &sources,
                                           const std::vector<std::string> &deps,
                                           const BuildConfig &config) {
    auto target = module_builder::artifact_path(ctx, module_name, module_builder::ArtifactKind::StaticLibrary);
    std::filesystem::create_directories(target.parent_path());

    std::vector<std::filesystem::path> objects;
    for (const auto &source : sources) {
        auto obj = object_path(ctx, module_name, source);
        std::filesystem::create_directories(obj.parent_path());

        if (is_outdated(obj, source)) {
            std::stringstream cmd;
            cmd << "g++ -std=c++17 -c";
            cmd << " -I" << ctx.modules_root;
            cmd << " -I" << (ctx.modules_root / module_name);
            for (const auto &dep : deps) {
                cmd << " -I" << (ctx.modules_root / dep);
            }
            for (const auto &extra : config.include_dirs) {
                cmd << " -I" << extra;
            }
            cmd << " -o " << obj << ' ' << source;

            std::cout << "Compiling " << source << "\n";
            if (std::system(cmd.str().c_str()) != 0) {
                throw std::runtime_error("Compilation failed for: " + source.string());
            }
        }

        objects.push_back(obj);
    }

    bool relink = !std::filesystem::exists(target);
    if (!relink) {
        for (const auto &obj : objects) {
            if (is_outdated(target, obj)) {
                relink = true;
                break;
            }
        }
    }

    if (relink) {
        std::stringstream cmd;
        cmd << "ar rcs " << target;
        for (const auto &obj : objects) {
            cmd << ' ' << obj;
        }
        std::cout << "Linking static library for module " << module_name << "\n";
        if (std::system(cmd.str().c_str()) != 0) {
            throw std::runtime_error("Failed to create static library for module: " + module_name);
        }
    }

    return target;
}

}  // namespace compiler_gcc
