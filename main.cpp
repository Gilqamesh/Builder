#include <iostream>
#include <format>
#include <filesystem>
#include <string>
#include <chrono>

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << std::format("usage: {} <target_module>", argv[0]) << std::endl;
            return 1;
        }

        const auto target_module = std::string(argv[1]);
        const auto root_dir = std::filesystem::path(".");
        const auto modules_dir = root_dir / "modules";
        const auto module_name = std::string("builder");
        const auto artifacts_dir = std::filesystem::path("artifacts");

        const auto builder_module_dir = modules_dir / module_name;
        const auto builder_artifact_dir = artifacts_dir / module_name;

        const auto orchestrator_binary_name = std::string("orchestrator");

        auto latest_builder_artifact_dir = std::filesystem::path();
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(builder_artifact_dir, ec)) {
            if (entry.is_directory()) {
                latest_builder_artifact_dir = std::max(latest_builder_artifact_dir, entry.path());
            }
        }

        if (!latest_builder_artifact_dir.empty()) {
            const auto orchestrator_binary = std::filesystem::absolute(latest_builder_artifact_dir / orchestrator_binary_name);
            if (!std::filesystem::exists(orchestrator_binary)) {
                throw std::runtime_error(std::format("'{}' does not exist", orchestrator_binary.string()));
            }

            const auto run_command = std::format("{} {} {} {} {}", orchestrator_binary.string(), root_dir.string(), modules_dir.string(), target_module, artifacts_dir.string());
            std::cout << run_command << std::endl;
            if (std::system(run_command.c_str()) != 0) {
                throw std::runtime_error(std::format("failed to run '{}'", orchestrator_binary.string()));
            }
        } else {
            const auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            const auto tmp_bin = std::filesystem::absolute(std::filesystem::temp_directory_path() / (orchestrator_binary_name + "@" + std::to_string(time_now)));
            const auto compile_command = std::format("clang++ -g -O3 -std=c++23 {}/*.cpp -I{} -o {}", builder_module_dir.string(), root_dir.string(), tmp_bin.string());
            std::cout << compile_command << std::endl;
            if (std::system(compile_command.c_str()) != 0) {
                throw std::runtime_error(std::format("failed to compile '{}'", tmp_bin.string()));
            }

            if (!std::filesystem::exists(tmp_bin)) {
                throw std::runtime_error(std::format("'{}' was not created, something went wrong", tmp_bin.string()));
            }

            const auto run_tmp_command = std::format("{} {} {} {} {}", tmp_bin.string(), root_dir.string(), modules_dir.string(), target_module, artifacts_dir.string());
            std::cout << run_tmp_command << std::endl;
            if (std::system(run_tmp_command.c_str()) != 0) {
                std::filesystem::remove(tmp_bin);
                throw std::runtime_error(std::format("failed to run '{}'", tmp_bin.string()));
            }

            std::filesystem::remove(tmp_bin);
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
