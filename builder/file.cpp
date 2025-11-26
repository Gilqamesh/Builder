#include "file.h"

#include "ir.h"
#include "binary.h"

const std::filesystem::path PROGRAMS_DIR = "programs";

// example file name: math.fun_1237327827346
struct locator_t {
    std::string filename;
    std::chrono::system_clock::time_point creation_time;
};

static locator_t path_to_locator(const std::filesystem::path& path) {
    locator_t result;

    size_t underscore_pos = filename.rfind('_');
    if (underscore_pos == std::string::npos) {
        throw std::runtime_error(std::format("invalid node file path: {}", path.string()));
    }
    result.filename = filename.substr(0, underscore_pos);

    std::string creation_time_str = filename.substr(underscore_pos + 1);
    uint64_t creation_time_seconds = std::stoull(creation_time_str);
    result.creation_time = std::chrono::system_clock::time_point(std::chrono::seconds(creation_time_seconds));

    return result;
}

static std::filesystem::path locator_to_path(const locator_t& locator) {
    return PROGRAMS_DIR / (locator.filename + "_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(locator.creation_time.time_since_epoch()).count()));
}

file_t file_t::find_latest(const std::string& ns, const std::string& name) {
    file_t result;

    const std::string filename = ns + "." + name;
    std::chrono::system_clock::time_point latest_creation_time;
    for (const auto& entry : std::filesystem::directory_iterator(PROGRAMS_DIR)) {
        if (!entry.is_regular_file()) {
            continue ;
        }

        const std::filesystem::path path = entry.path();
        locator_t locator = path_to_locator(path.filename());
        if (locator.name != filename) {
            continue ;
        }

        if (latest_creation_time < locator.creation_time) {
            latest_creation_time = locator.creation_time;
            result.path = path;
        }
    }

    if (result.path.empty()) {
        throw std::runtime_error(std::format("no latest file found for '{}'", filename));
    }

    return result;
}

file_t file_t::save(const ir_t& ir) {
    file_t result;

    result.path = locator_to_path({
        .ns = ir.ns,
        .name = ir.name,
        .creation_time = std::chrono::system_clock::now()
    });

    binary_t binary = coerce(ir);

    // std::ofstream bin_file(to_bin_file_path(node->id().name), std::ios::binary);
    // std::ofstream human_readable_file(to_human_readable_file_path(node->id().name));

    // binary_t binary;
    // if (!coerce(&node, &binary)) {
    //     return false;
    // }

    // bin_file.write((const char*) binary.binary.data(), (std::streamsize) binary.binary.size());

    // node_assembly_t node_assembly;
    // if (!coerce(&binary, &node_assembly)) {
    //     return false;
    // }
    // human_readable_file.write(node_assembly.assembly.c_str(), (std::streamsize) node_assembly.assembly.size());

    // for (node_t* child : node->inner_nodes()) {
    //     persist_save(child);
    // }

    // return true;
}

ir_t file_t::load(const file_t& file) {
    const std::string bin_path = PROGRAMS_DIR / (path + ".bin");
    std::ifstream bin_file(bin_path);
    if (!bin_file.is_open()) {
        return nullptr;
        // throw std::runtime_error("failed to open binary file ' + bin_path + ');
    }

    binary_t binary;

    bin_file.seekg(0, std::ios::end);
    size_t size = (size_t) bin_file.tellg();
    bin_file.seekg(0, std::ios::beg);
    binary.binary.resize(size);
    bin_file.read((char*) binary.binary.data(), (std::streamsize) size);

    node_t* node = new node_t;
    if (!coerce(&binary, &node)) {
        delete node;
        return nullptr;
    }

    return node;
}

struct file_install_coercions {
    file_install_coercions() {
        TYPESYSTEM.register_coercion(&ir_to_file);
        TYPESYSTEM.register_coercion(&file_to_ir);
    }
} FILE_INSTALL_COERCIONS;
