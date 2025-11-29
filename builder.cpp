// builder.cpp

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

namespace fs = std::filesystem;

// ------------------------- Utility helpers ----------------------------------

static std::string trim(const std::string &s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

static std::vector<std::string> split_ws(const std::string &s) {
    std::vector<std::string> out;
    std::istringstream is(s);
    std::string tok;
    while (is >> tok) {
        out.push_back(trim(tok));
    }
    return out;
}

static std::string shell_escape(const std::string &s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\"'\"'";
        else out += c;
    }
    out += "'";
    return out;
}

static void run_checked(const std::string &cmd) {
    std::cout << "[cmd] " << cmd << "\n";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[error] command failed with code " << rc << ": " << cmd << "\n";
        std::exit(1);
    }
}

static bool has_glob_chars(const std::string &s) {
    return s.find_first_of("*?") != std::string::npos;
}

// wildcard matcher supporting '*' and '?'
static bool wildcard_match(const std::string &pattern, const std::string &text) {
    std::size_t p = 0, t = 0;
    std::size_t star = std::string::npos, match = 0;

    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++p; ++t;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            match = t;
        } else if (star != std::string::npos) {
            p = star + 1;
            t = ++match;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

// --------------------------- Module struct ----------------------------------

struct Module {
    std::string name;
    std::string type;
    fs::path dir;

    std::vector<std::string> source_patterns;
    std::vector<std::string> sources;
    std::vector<std::string> deps;
    std::vector<std::string> sys_libs;
    std::string cxx_flags;
    std::string ld_flags;

    std::string fetch_url;

    fs::path build_dir;
    std::vector<fs::path> objects;
    std::vector<fs::path> output_libs;
    std::vector<fs::path> produced_libs;
    std::vector<fs::path> include_dirs;

    bool built = false;
    bool visiting = false;
};

static fs::path g_root;
static fs::path g_src_root;
static fs::path g_build_root;
static fs::path g_cache_root;
static std::unordered_map<std::string, Module> g_modules;

static Module &get_module(const std::string &name);

// ------------------------ build.module parsing ------------------------------

static void parse_build_module(Module &m) {
    fs::path cfg = m.dir / "build.module";
    if (!fs::exists(cfg)) {
        std::cerr << "[error] missing build.module for module '" << m.name
                  << "' at " << cfg << "\n";
        std::exit(1);
    }

    std::ifstream in(cfg);
    if (!in) {
        std::cerr << "[error] cannot open " << cfg << "\n";
        std::exit(1);
    }

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (!val.empty() && (val.front() == '"' || val.front() == '\'')) {
            char q = val.front();
            if (val.size() >= 2 && val.back() == q) {
                val = val.substr(1, val.size() - 2);
            }
        }

        if (key == "MODULE_NAME") {
            if (!val.empty()) m.name = val;
        } else if (key == "MODULE_TYPE") {
            m.type = val;
        } else if (key == "MODULE_SOURCES") {
            m.source_patterns = split_ws(val);
        } else if (key == "MODULE_DEPS") {
            m.deps = split_ws(val);
        } else if (key == "MODULE_SYS_LIBS") {
            m.sys_libs = split_ws(val);
        } else if (key == "MODULE_CXX_FLAGS") {
            m.cxx_flags = val;
        } else if (key == "MODULE_LD_FLAGS") {
            m.ld_flags = val;
        } else if (key == "MODULE_FETCH_URL") {
            m.fetch_url = val;
        }
    }

    if (m.type.empty()) {
        std::cerr << "[error] MODULE_TYPE is required in " << cfg << "\n";
        std::exit(1);
    }

    if (m.type != "remote_cmake") {
        m.include_dirs.push_back(m.dir);
    }
}

static Module &get_module(const std::string &name) {
    auto it = g_modules.find(name);
    if (it != g_modules.end()) return it->second;

    Module m;
    m.name = name;
    m.dir = g_src_root / name;
    if (!fs::exists(m.dir) || !fs::is_directory(m.dir)) {
        std::cerr << "[error] module directory not found: " << m.dir << "\n";
        std::exit(1);
    }
    parse_build_module(m);
    m.build_dir = g_build_root / m.name;
    auto [it2, _] = g_modules.emplace(m.name, std::move(m));
    return it2->second;
}

// ------------------------ JSON graph dumper ---------------------------------

static void dump_dep_graph(const std::string &root_name) {
    std::unordered_set<std::string> seen;
    std::vector<std::string> order;

    std::function<void(const std::string &)> dfs =
        [&](const std::string &name) {
            if (!seen.insert(name).second) return;
            Module &m = get_module(name);
            order.push_back(m.name);
            for (const auto &d : m.deps) dfs(d);
        };

    dfs(root_name);

    fs::create_directories(g_build_root);
    fs::path out_path = g_build_root / ("graph_" + root_name + ".json");

    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "[error] cannot write JSON graph: " << out_path << "\n";
        std::exit(1);
    }

    out << "{\n";
    out << "  \"root\": \"" << root_name << "\",\n";
    out << "  \"modules\": {\n";

    for (std::size_t i = 0; i < order.size(); ++i) {
        const std::string &name = order[i];
        Module &m = get_module(name);

        out << "    \"" << m.name << "\": { ";
        out << "\"type\": \"" << m.type << "\"";

        if (m.type == "remote_cmake" && !m.fetch_url.empty()) {
            out << ", \"fetch_url\": \"" << m.fetch_url << "\"";
        }

        out << ", \"deps\": [";
        for (std::size_t j = 0; j < m.deps.size(); ++j) {
            out << "\"" << m.deps[j] << "\"";
            if (j + 1 < m.deps.size()) out << ", ";
        }
        out << "] }";
        if (i + 1 < order.size()) out << ",";
        out << "\n";
    }

    out << "  }\n";
    out << "}\n";

    std::cout << "[graph] wrote " << out_path << "\n";
}

// --------------------------- Source globbing --------------------------------

static void expand_sources(Module &m) {
    m.sources.clear();

    if (m.type == "remote_cmake" || m.type == "header_only" || m.type == "meta") {
        return;
    }

    if (m.source_patterns.empty()) return;

    std::set<std::string> uniq;

    for (const auto &pat : m.source_patterns) {
        if (!has_glob_chars(pat)) {
            uniq.insert(pat);
            continue;
        }

        fs::path rel = pat;
        fs::path pat_dir = rel.parent_path();
        std::string fname_pattern = rel.filename().string();

        fs::path base_dir = m.dir / pat_dir;
        if (!fs::exists(base_dir) || !fs::is_directory(base_dir)) {
            continue;
        }

        for (auto const &entry : fs::recursive_directory_iterator(base_dir)) {
            if (!entry.is_regular_file()) continue;
            std::string fn = entry.path().filename().string();
            if (!wildcard_match(fname_pattern, fn)) continue;

            fs::path rel_path = fs::relative(entry.path(), m.dir);
            uniq.insert(rel_path.generic_string());
        }
    }

    for (const auto &s : uniq) {
        m.sources.push_back(s);
    }
}

// --------------------------- remote_cmake -----------------------------------

static fs::path fetch_remote_tarball(const Module &m) {
    if (m.fetch_url.empty()) {
        std::cerr << "[error] MODULE_FETCH_URL is required for remote_cmake module '"
                  << m.name << "'\n";
        std::exit(1);
    }

    fs::path mod_cache_root = g_cache_root / "remote" / m.name;
    fs::create_directories(mod_cache_root);

    fs::path tar_path = mod_cache_root / "src.tar.gz";
    if (!fs::exists(tar_path)) {
        std::string cmd =
            "curl -L " + shell_escape(m.fetch_url) + " -o " + shell_escape(tar_path.string());
        run_checked(cmd);
    }

    bool extracted = false;
    for (auto &e : fs::directory_iterator(mod_cache_root)) {
        if (e.is_directory()) {
            extracted = true;
            break;
        }
    }
    if (!extracted) {
        std::string cmd =
            "tar -xf " + shell_escape(tar_path.string()) +
            " -C " + shell_escape(mod_cache_root.string());
        run_checked(cmd);
    }

    for (auto &e : fs::directory_iterator(mod_cache_root)) {
        if (e.is_directory()) {
            return e.path();
        }
    }

    std::cerr << "[error] failed to find extracted source dir for remote module '"
              << m.name << "'\n";
    std::exit(1);
}

static void discover_remote_libs(Module &m, const fs::path &build_dir) {
    m.produced_libs.clear();
    for (auto &e : fs::recursive_directory_iterator(build_dir)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        if (ext == ".a" || ext == ".so" || ext == ".dylib") {
            m.produced_libs.push_back(e.path());
        }
    }
}

static void build_remote_cmake(Module &m) {
    fs::path src_root = fetch_remote_tarball(m);
    fs::path build_dir = src_root.parent_path() / ("build_" + m.name);
    fs::create_directories(build_dir);

    bool has_lib = false;
    for (auto &e : fs::recursive_directory_iterator(build_dir)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        if (ext == ".a" || ext == ".so" || ext == ".dylib") {
            has_lib = true;
            break;
        }
    }

    if (!has_lib) {
        std::string cmd_cfg = "cmake -S " + shell_escape(src_root.string()) +
                              " -B " + shell_escape(build_dir.string());
        run_checked(cmd_cfg);

        std::string cmd_build = "cmake --build " + shell_escape(build_dir.string()) +
                                " --config Release -j";
        run_checked(cmd_build);
    }

    discover_remote_libs(m, build_dir);

    m.include_dirs.clear();
    m.include_dirs.push_back(src_root);
    if (fs::exists(src_root / "include")) m.include_dirs.push_back(src_root / "include");
    if (fs::exists(src_root / "src"))     m.include_dirs.push_back(src_root / "src");
}

// --------------------- Header dependency scanning ---------------------------

static void collect_file_dependencies(
    const fs::path &file,
    const std::vector<fs::path> &include_search_dirs,
    std::unordered_set<std::string> &visited,
    std::unordered_set<fs::path> &out_deps)
{
    fs::path norm = fs::weakly_canonical(file);
    std::string key = norm.string();
    if (visited.count(key)) return;
    visited.insert(key);

    out_deps.insert(norm);

    std::ifstream in(file);
    if (!in) return;

    static const std::regex include_re("^\\s*#\\s*include\\s*\"([^\"]+)\"");

    std::string line;
    while (std::getline(in, line)) {
        std::smatch m;
        if (!std::regex_search(line, m, include_re)) continue;
        if (m.size() < 2) continue;

        std::string header_name = m[1].str();

        for (const auto &inc_dir : include_search_dirs) {
            fs::path candidate = inc_dir / header_name;
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                collect_file_dependencies(candidate, include_search_dirs, visited, out_deps);
                break;
            }
        }
    }
}

// --------------------- Local compilation & archiving ------------------------

static void compile_module(const std::string &name);

static void collect_dep_include_dirs(
    Module &m,
    std::vector<fs::path> &dirs,
    std::unordered_set<std::string> &seen)
{
    for (const auto &dep_name : m.deps) {
        if (!seen.insert(dep_name).second) continue;
        Module &dep = get_module(dep_name);
        for (const auto &inc : dep.include_dirs) {
            dirs.push_back(inc);
        }
        collect_dep_include_dirs(dep, dirs, seen);
    }
}

static void compile_sources(Module &m) {
    fs::create_directories(m.build_dir);
    m.objects.clear();

    std::vector<fs::path> include_search_dirs;
    include_search_dirs.push_back(g_src_root);
    for (const auto &d : m.include_dirs) {
        include_search_dirs.push_back(d);
    }
    {
        std::unordered_set<std::string> seen;
        collect_dep_include_dirs(m, include_search_dirs, seen);
    }

    std::sort(include_search_dirs.begin(), include_search_dirs.end());
    include_search_dirs.erase(
        std::unique(include_search_dirs.begin(), include_search_dirs.end()),
        include_search_dirs.end()
    );

    for (const auto &rel_src : m.sources) {
        fs::path src_path = m.dir / rel_src;
        if (!fs::exists(src_path)) {
            std::cerr << "[error] Missing source file: " << src_path << "\n";
            std::exit(1);
        }

        fs::path obj = m.build_dir / (src_path.filename().string() + ".o");
        m.objects.push_back(obj);

        std::unordered_set<std::string> visited;
        std::unordered_set<fs::path> deps;
        collect_file_dependencies(src_path, include_search_dirs, visited, deps);

        bool need_rebuild = false;
        if (!fs::exists(obj)) {
            need_rebuild = true;
        } else {
            auto obj_time = fs::last_write_time(obj);
            for (const auto &dep : deps) {
                try {
                    auto dep_time = fs::last_write_time(dep);
                    if (dep_time > obj_time) {
                        need_rebuild = true;
                        break;
                    }
                } catch (...) {
                    need_rebuild = true;
                    break;
                }
            }
        }

        if (!need_rebuild) {
            std::cout << "[c++] " << m.name << ": up-to-date " << obj << "\n";
            continue;
        }

        std::ostringstream cmd;
        const char *cxx_env = std::getenv("CXX");
        cmd << (cxx_env ? cxx_env : "c++")
            << " " << m.cxx_flags
            << " -c " << shell_escape(src_path.string());

        for (const auto &inc : include_search_dirs) {
            cmd << " -I" << shell_escape(inc.string());
        }

        cmd << " -o " << shell_escape(obj.string());

        std::cout << "[c++] " << m.name << ": " << src_path << " -> " << obj << "\n";
        run_checked(cmd.str());
    }
}

static void make_static_lib(Module &m) {
    if (m.objects.empty()) return;

    fs::path lib_path = m.build_dir / ("lib" + m.name + ".a");

    bool need_archive = false;
    if (!fs::exists(lib_path)) {
        need_archive = true;
    } else {
        auto lib_time = fs::last_write_time(lib_path);
        for (const auto &obj : m.objects) {
            try {
                auto obj_time = fs::last_write_time(obj);
                if (obj_time > lib_time) {
                    need_archive = true;
                    break;
                }
            } catch (...) {
                need_archive = true;
                break;
            }
        }
    }

    if (!need_archive) {
        std::cout << "[ar] " << m.name << ": up-to-date " << lib_path << "\n";
        m.output_libs = {lib_path};
        return;
    }

    std::ostringstream cmd;
    cmd << "ar rcs " << shell_escape(lib_path.string());
    for (const auto &obj : m.objects) {
        cmd << " " << shell_escape(obj.string());
    }

    std::cout << "[ar] " << m.name << ": " << lib_path << "\n";
    run_checked(cmd.str());
    m.output_libs = {lib_path};
}

static void make_shared_lib(Module &m) {
    if (m.objects.empty()) return;

    fs::path so_path = m.build_dir / ("lib" + m.name + ".so");

    bool need_link = false;
    if (!fs::exists(so_path)) {
        need_link = true;
    } else {
        auto so_time = fs::last_write_time(so_path);
        for (const auto &obj : m.objects) {
            try {
                auto obj_time = fs::last_write_time(obj);
                if (obj_time > so_time) {
                    need_link = true;
                    break;
                }
            } catch (...) {
                need_link = true;
                break;
            }
        }
    }

    if (!need_link) {
        std::cout << "[link-shared] " << m.name << ": up-to-date " << so_path << "\n";
        m.output_libs = {so_path};
        return;
    }

    std::ostringstream cmd;
    const char *cxx_env = std::getenv("CXX");
    cmd << (cxx_env ? cxx_env : "c++")
        << " -shared -o " << shell_escape(so_path.string());

    for (const auto &obj : m.objects) {
        cmd << " " << shell_escape(obj.string());
    }
    if (!m.ld_flags.empty()) cmd << " " << m.ld_flags;

    std::cout << "[link-shared] " << m.name << ": " << so_path << "\n";
    run_checked(cmd.str());
    m.output_libs = {so_path};
}

// --------------------------- Link ordering (DFS) -----------------------------

static void append_libs_for_module(
    const Module &m,
    std::vector<fs::path> &libs,
    std::unordered_set<std::string> &visiting)
{
    if (visiting.count(m.name)) return;
    visiting.insert(m.name);

    for (const auto &lib : m.output_libs) libs.push_back(lib);
    for (const auto &lib : m.produced_libs) libs.push_back(lib);

    for (const auto &dep_name : m.deps) {
        const Module &dep = get_module(dep_name);
        append_libs_for_module(dep, libs, visiting);
    }

    visiting.erase(m.name);
}

static fs::path make_executable(Module &m) {
    if (m.objects.empty()) return {};

    fs::path exe_path = m.build_dir / m.name;

    std::vector<fs::path> libs;
    std::unordered_set<std::string> visiting;
    for (const auto &dep_name : m.deps) {
        const Module &dep = get_module(dep_name);
        append_libs_for_module(dep, libs, visiting);
    }

    bool need_link = false;
    if (!fs::exists(exe_path)) {
        need_link = true;
    } else {
        auto exe_time = fs::last_write_time(exe_path);

        for (const auto &obj : m.objects) {
            try {
                auto obj_time = fs::last_write_time(obj);
                if (obj_time > exe_time) {
                    need_link = true;
                    break;
                }
            } catch (...) {
                need_link = true;
                break;
            }
        }

        if (!need_link) {
            for (const auto &lib : libs) {
                try {
                    auto lib_time = fs::last_write_time(lib);
                    if (lib_time > exe_time) {
                        need_link = true;
                        break;
                    }
                } catch (...) {
                    need_link = true;
                    break;
                }
            }
        }
    }

    if (!need_link) {
        std::cout << "[link-exe] " << m.name << ": up-to-date " << exe_path << "\n";
        return exe_path;
    }

    std::ostringstream cmd;
    const char *cxx_env = std::getenv("CXX");
    cmd << (cxx_env ? cxx_env : "c++")
        << " -o " << shell_escape(exe_path.string());

    for (const auto &obj : m.objects) {
        cmd << " " << shell_escape(obj.string());
    }
    for (const auto &lib : libs) {
        cmd << " " << shell_escape(lib.string());
    }
    for (const auto &sys : m.sys_libs) {
        cmd << " -l" << sys;
    }
    if (!m.ld_flags.empty()) cmd << " " << m.ld_flags;

    std::cout << "[link-exe] " << m.name << ": " << exe_path << "\n";
    run_checked(cmd.str());
    return exe_path;
}

// ---------------------------- Build driver ----------------------------------

static void compile_module(const std::string &name) {
    Module &m = get_module(name);

    if (m.built) return;
    if (m.visiting) {
        std::cerr << "[error] dependency cycle detected at module '" << name << "'\n";
        std::exit(1);
    }

    m.visiting = true;
    std::cout << "[build] module: " << m.name << "\n";

    for (const auto &dep : m.deps) {
        compile_module(dep);
    }

    if (m.type == "header_only" || m.type == "meta") {
        m.built = true;
        m.visiting = false;
        return;
    }

    if (m.type == "remote_cmake") {
        build_remote_cmake(m);
        m.built = true;
        m.visiting = false;
        return;
    }

    expand_sources(m);
    compile_sources(m);

    if (m.type == "static_lib") {
        make_static_lib(m);
    } else if (m.type == "shared_lib") {
        make_shared_lib(m);
    } else if (m.type == "executable") {
        make_executable(m);
    } else {
        std::cerr << "[error] unknown MODULE_TYPE '" << m.type
                  << "' for module '" << m.name << "'\n";
        std::exit(1);
    }

    m.built = true;
    m.visiting = false;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <module_name>\n"
                  << "  " << argv[0] << " --graph <module_name>\n";
        return 1;
    }

    g_root = fs::current_path();
    g_src_root = g_root / "src";
    g_build_root = g_root / "build";
    g_cache_root = g_root / ".cache";

    if (!fs::exists(g_src_root) || !fs::is_directory(g_src_root)) {
        std::cerr << "[error] src/ directory not found in " << g_root << "\n";
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--graph") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " --graph <module_name>\n";
            return 1;
        }
        dump_dep_graph(argv[2]);
        return 0;
    }

    std::string target_module = arg1;
    compile_module(target_module);

    Module &m = get_module(target_module);
    if (m.type == "executable") {
        fs::path exe_path = m.build_dir / m.name;
        if (fs::exists(exe_path) && fs::is_regular_file(exe_path)) {
            std::cout << "[run] " << exe_path << "\n";
            std::string cmd = shell_escape(exe_path.string());
            return std::system(cmd.c_str());
        }
    }

    return 0;
}
