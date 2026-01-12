#include <modules/builder/gzip/gzip.h>

#define HAVE_UNISTD_H 1
#include <modules/builder/gzip/external/zlib.h>

#include <iostream>
#include <format>
#include <fstream>

path_t gzip_t::gzip(const path_t& file, const path_t& install_gzip_path) {
    if (install_gzip_path.extension() != ".gz") {
        throw std::runtime_error(std::format("gzip_t::gzip: install path '{}' must have .gz extension", install_gzip_path.string()));
    }

    if (filesystem_t::exists(install_gzip_path)) {
        throw std::runtime_error(std::format("gzip_t::gzip: install path '{}' already exists", install_gzip_path.string()));
    }

    if (!filesystem_t::exists(file) || !filesystem_t::is_regular_file(file)) {
        throw std::runtime_error(std::format("gzip_t::gzip: input '{}' is not a regular file", file.string()));
    }

    std::cout << std::format("gzip -c {} > {}", file.string(), install_gzip_path.string()) << std::endl;

    std::ifstream ifs(file.string(), std::ios::binary);
    if (!ifs) {
        throw std::runtime_error(std::format("gzip_t::gzip: failed to open input '{}'", file.string()));
    }

    gzFile gzip = gzopen(install_gzip_path.string().c_str(), "wb");
    if (!gzip) {
        throw std::runtime_error(std::format("gzip_t::gzip: failed to open output '{}'", install_gzip_path.string()));
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
                throw std::runtime_error(std::format("gzip_t::gzip: gzwrite failed: {}", err_msg));
            }
        }

        if (ifs.eof()) {
            break ;
        }

        if (ifs.fail() || ifs.bad()) {
            gzclose(gzip);
            throw std::runtime_error(std::format("gzip_t::gzip: read failed from input '{}'", file.string()));
        }
    }

    if (gzclose(gzip) != Z_OK) {
        throw std::runtime_error(std::format("gzip_t::gzip: failed to close output '{}'", install_gzip_path.string()));
    }

    return install_gzip_path;
}

path_t gzip_t::ungzip(const path_t& gzip_path, const path_t& install_file) {
    if (gzip_path.extension() != ".gz") {
        throw std::runtime_error(std::format("gzip_t::ungzip: gzip path '{}' must have .gz extension", gzip_path.string()));
    }

    if (!filesystem_t::exists(gzip_path) || !filesystem_t::is_regular_file(gzip_path)) {
        throw std::runtime_error(std::format("gzip_t::ungzip: input '{}' is not a regular file", gzip_path.string()));
    }

    if (filesystem_t::exists(install_file)) {
        throw std::runtime_error(std::format("gzip_t::ungzip: install file '{}' already exists", install_file.string()));
    }

    const auto parent_install_file = install_file.parent();
    if (!filesystem_t::exists(parent_install_file)) {
        filesystem_t::create_directories(parent_install_file);
    }

    std::cout << std::format("gzip -d -c {} > {}", gzip_path.string(), install_file.string()) << std::endl;

    std::ofstream ofs(install_file.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("gzip_t::ungzip: failed to open output '{}'", install_file.string()));
    }

    gzFile gz = gzopen(gzip_path.string().c_str(), "rb");
    if (!gz) {
        throw std::runtime_error(std::format("gzip_t::ungzip: failed to open input '{}'", gzip_path.string()));
    }

    char buffer[4096];
    while (1) {
        const auto read_count = gzread(gz, buffer, sizeof(buffer));
        if (read_count < 0) {
            int err;
            std::string err_msg = gzerror(gz, &err);
            gzclose(gz);
            throw std::runtime_error(std::format("gzip_t::ungzip: gzread failed: {}", err_msg));
        }

        if (read_count == 0) {
            break ;
        }

        ofs.write(buffer, read_count);
        if (!ofs) {
            gzclose(gz);
            throw std::runtime_error(std::format("gzip_t::ungzip: write failed to output '{}'", install_file.string()));
        }
    }

    if (gzclose(gz) != Z_OK) {
        throw std::runtime_error(std::format("gzip_t::ungzip: failed to close input '{}'", gzip_path.string()));
    }

    return install_file;
}
