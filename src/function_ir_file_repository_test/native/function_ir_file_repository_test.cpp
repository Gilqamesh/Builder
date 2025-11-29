#include <function_ir_file_repository.h>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace {

function_ir_t make_function_ir(const std::string& ns, const std::string& name, int64_t creation_time_seconds) {
    const function_id_t id{
        .ns = ns,
        .name = name,
        .creation_time = std::chrono::system_clock::time_point(std::chrono::seconds(creation_time_seconds))
    };

    function_ir_t ir{
        .function_id = id,
        .left = 0,
        .right = 1,
        .top = 0,
        .bottom = 1,
        .children = {},
        .connections = {},
    };

    return ir;
}

} // namespace

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

TEST(FunctionIrFileRepositoryTest, LoadLatestReturnsNewestMatchingFunction) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "builder_ir_repo_test_load_latest";
    std::filesystem::remove_all(temp_dir);

    function_ir_file_repository_t repo(temp_dir);

    repo.save(make_function_ir("ns", "fn", 1));
    repo.save(make_function_ir("other", "fn", 3));
    const auto latest = make_function_ir("ns", "fn", 5);
    repo.save(latest);

    const auto loaded = repo.load_latest("ns", "fn");

    EXPECT_EQ(loaded.function_id.ns, latest.function_id.ns);
    EXPECT_EQ(loaded.function_id.name, latest.function_id.name);
    EXPECT_EQ(loaded.function_id.creation_time, latest.function_id.creation_time);
    std::filesystem::remove_all(temp_dir);
}

TEST(FunctionIrFileRepositoryTest, LoadThrowsForMissingFile) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "builder_ir_repo_test_missing";
    std::filesystem::remove_all(temp_dir);

    function_ir_file_repository_t repo(temp_dir);

    const function_id_t missing_id{
        .ns = "missing",
        .name = "fn",
        .creation_time = std::chrono::system_clock::time_point(std::chrono::seconds(1))
    };

    EXPECT_THROW(repo.load(missing_id), std::runtime_error);
    std::filesystem::remove_all(temp_dir);
}

TEST(FunctionIrFileRepositoryTest, LoadThrowsForCorruptedFile) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "builder_ir_repo_test_corrupt";
    std::filesystem::remove_all(temp_dir);

    function_ir_file_repository_t repo(temp_dir);
    const function_ir_t ir = make_function_ir("ns", "fn", 2);
    const auto corrupt_path = temp_dir / function_id_t::to_string(ir.function_id);

    {
        std::ofstream ofs(corrupt_path, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        // Write insufficient data to trigger a deserialize error.
        ofs.write("\x01", 1);
    }

    EXPECT_THROW(repo.load(ir.function_id), std::runtime_error);
    std::filesystem::remove_all(temp_dir);
}

TEST(FunctionIrFileRepositoryTest, LoadLatestThrowsWhenNoMatchingFunction) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "builder_ir_repo_test_no_match";
    std::filesystem::remove_all(temp_dir);

    function_ir_file_repository_t repo(temp_dir);
    repo.save(make_function_ir("ns", "fn", 1));

    EXPECT_THROW(repo.load_latest("other", "fn"), std::runtime_error);
    std::filesystem::remove_all(temp_dir);
}
