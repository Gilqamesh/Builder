#include <function_ir_file_repository.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <function_ir_binary.h>

function_ir_file_repository_t::function_ir_file_repository_t(const std::filesystem::path& directory_path):
    m_directory_path(directory_path)
{
    if (!std::filesystem::exists(m_directory_path)) {
        std::filesystem::create_directories(m_directory_path);
    }
}

void function_ir_file_repository_t::save(const function_ir_t& function_ir) {
    function_ir_binary_t function_ir_binary(function_ir);

    const auto path = function_id_to_path(function_ir.function_id);
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error(std::format("failed to open file for writing: {}", path.string()));
    }

    const auto& bytes = function_ir_binary.bytes();
    ofs.write((const char*) bytes.data(), (std::streamsize) bytes.size());
}

function_ir_t function_ir_file_repository_t::load(const function_id_t& function_id) const {
    const auto path = function_id_to_path(function_id);
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error(std::format("failed to open file for reading: {}", path.string()));
    }

    ifs.seekg(0, std::ios::end);
    size_t size = (size_t) ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(size);
    ifs.read((char*) bytes.data(), (std::streamsize) size);

    function_ir_binary_t function_ir_binary(std::move(bytes));

    function_ir_t result = function_ir_binary.function_ir();

    return result;
}

function_ir_t function_ir_file_repository_t::load_latest(const std::string& ns, const std::string& name) const {
    function_id_t latest_function_id;
    for (const auto& entry : std::filesystem::directory_iterator(m_directory_path)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const function_id_t function_id = path_to_function_id(entry.path());
        if (function_id.ns != ns || function_id.name != name) {
            continue;
        }

        if (!latest_function_id) {
            latest_function_id = function_id;
            continue;
        }
        if (latest_function_id.creation_time < function_id.creation_time) {
            latest_function_id = function_id;
        }
    }

    if (!latest_function_id) {
        throw std::runtime_error(std::format("no latest function found for {}::{} in directory {}", ns, name, m_directory_path.string()));
    }

    function_ir_t result = load(latest_function_id);

    return result;
}

function_id_t function_ir_file_repository_t::path_to_function_id(const std::filesystem::path& path) const {
    function_id_t result;

    const std::string filename = path.filename().string();

    result = function_id_t::from_string(path.filename().string());

    return result;
}

std::filesystem::path function_ir_file_repository_t::function_id_to_path(const function_id_t& id) const {
    std::filesystem::path result = m_directory_path / (function_id_t::to_string(id));

    return result;
}

#ifdef BUILDER_ENABLE_TESTS
#include <gtest/gtest.h>
#include <filesystem>

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
#endif
