#include "gzip.h"

#define HAVE_UNISTD_H 1
#include "external/zlib.h"

#include <iostream>
#include <format>
#include <fstream>

namespace gzip {

filesystem::path_t gzip(const filesystem::path_t& file, const filesystem::path_t& install_gzip_path) {
    if (install_gzip_path.extension() != ".gz") {
        throw std::runtime_error(std::format("gzip::gzip: install path '{}' must have .gz extension", install_gzip_path));
    }

    if (filesystem::exists(install_gzip_path)) {
        throw std::runtime_error(std::format("gzip::gzip: install path '{}' already exists", install_gzip_path));
    }

    if (!filesystem::exists(file) || !filesystem::is_regular_file(file)) {
        throw std::runtime_error(std::format("gzip::gzip: input '{}' is not a regular file", file));
    }

    std::cout << std::format("gzip -c {} > {}", file, install_gzip_path) << std::endl;

    std::ifstream ifs(file.string(), std::ios::binary);
    if (!ifs) {
        throw std::runtime_error(std::format("gzip::gzip: failed to open input '{}'", file));
    }

    gzFile gzip = gzopen(install_gzip_path.string().c_str(), "wb");
    if (!gzip) {
        throw std::runtime_error(std::format("gzip::gzip: failed to open output '{}'", install_gzip_path));
    }

    char buffer[4096];
    while (1) {
        ifs.read(buffer, sizeof(buffer));
        const auto read_count = ifs.gcount();
        if (0 < read_count) {
            if (gzwrite(gzip, buffer, static_cast<unsigned>(read_count)) == 0) {
                int err;
                std::string err_msg = gzerror(gzip, &err);
                gzclose(gzip);
                throw std::runtime_error(std::format("gzip::gzip: gzwrite failed: {}", err_msg));
            }
        }

        if (ifs.eof()) {
            break ;
        }

        if (ifs.fail() || ifs.bad()) {
            gzclose(gzip);
            throw std::runtime_error(std::format("gzip::gzip: read failed from input '{}'", file));
        }
    }

    if (gzclose(gzip) != Z_OK) {
        throw std::runtime_error(std::format("gzip::gzip: failed to close output '{}'", install_gzip_path));
    }

    return install_gzip_path;
}

filesystem::path_t ungzip(const filesystem::path_t& gzip_path, const filesystem::path_t& install_file) {
    if (gzip_path.extension() != ".gz") {
        throw std::runtime_error(std::format("gzip::ungzip: gzip path '{}' must have .gz extension", gzip_path));
    }

    if (!filesystem::exists(gzip_path) || !filesystem::is_regular_file(gzip_path)) {
        throw std::runtime_error(std::format("gzip::ungzip: input '{}' is not a regular file", gzip_path));
    }

    if (filesystem::exists(install_file)) {
        throw std::runtime_error(std::format("gzip::ungzip: install file '{}' already exists", install_file));
    }

    const auto parent_install_file = install_file.parent();
    if (!filesystem::exists(parent_install_file)) {
        filesystem::create_directories(parent_install_file);
    }

    std::cout << std::format("gzip -d -c {} > {}", gzip_path, install_file) << std::endl;

    std::ofstream ofs(install_file.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("gzip::ungzip: failed to open output '{}'", install_file));
    }

    gzFile gz = gzopen(gzip_path.string().c_str(), "rb");
    if (!gz) {
        throw std::runtime_error(std::format("gzip::ungzip: failed to open input '{}'", gzip_path));
    }

    char buffer[4096];
    while (1) {
        const auto read_count = gzread(gz, buffer, sizeof(buffer));
        if (read_count < 0) {
            int err;
            std::string err_msg = gzerror(gz, &err);
            gzclose(gz);
            throw std::runtime_error(std::format("gzip::ungzip: gzread failed: {}", err_msg));
        }

        if (read_count == 0) {
            break ;
        }

        ofs.write(buffer, read_count);
        if (!ofs) {
            gzclose(gz);
            throw std::runtime_error(std::format("gzip::ungzip: write failed to output '{}'", install_file));
        }
    }

    if (gzclose(gz) != Z_OK) {
        throw std::runtime_error(std::format("gzip::ungzip: failed to close input '{}'", gzip_path));
    }

    return install_file;
}

} // namespace gzip
