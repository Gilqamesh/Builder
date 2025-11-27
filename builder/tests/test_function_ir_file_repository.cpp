#include <gtest/gtest.h>
#include <filesystem>

#include "function_ir_file_repository.h"

TEST(FunctionIrFileRepositoryTest, SavesAndLoads) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "builder_ir_repo_test";
    std::filesystem::remove_all(temp_dir);

    function_ir_file_repository_t repo(temp_dir);

    const function_id_t id{
        .ns = "ns",
        .name = "fn",
        .creation_time = std::chrono::system_clock::now()
    };

    function_ir_t ir{
        .function_id = id,
        .left = 0,
        .right = 1,
        .top = 0,
        .bottom = 1,
        .children = {},
        .connections = {}
    };

    repo.save(ir);
    const auto loaded = repo.load(id);

    EXPECT_EQ(loaded.function_id.ns, ir.function_id.ns);
    EXPECT_EQ(loaded.function_id.name, ir.function_id.name);
    std::filesystem::remove_all(temp_dir);
}
