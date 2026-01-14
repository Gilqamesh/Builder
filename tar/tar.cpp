#include <builder/tar/tar.h>
#include <builder/tar/external/microtar.h>

#include <iostream>
#include <format>
#include <fstream>

path_t tar_t::tar(const path_t& dir, const path_t& install_tar_path) {
    if (install_tar_path.extension() != ".tar") {
        throw std::runtime_error(std::format("tar_t::tar: install path '{}' must have .tar extension", install_tar_path));
    }

    if (filesystem_t::exists(install_tar_path)) {
        throw std::runtime_error(std::format("tar_t::tar: install path '{}' already exists", install_tar_path));
    }

    std::cout << std::format("tar -cf {} {}", install_tar_path, dir) << std::endl;

    const auto parent_install_tar_path = install_tar_path.parent();
    if (!filesystem_t::exists(parent_install_tar_path)) {
        filesystem_t::create_directories(parent_install_tar_path);
    }

    mtar_t mtar;
    int r = mtar_open(&mtar, install_tar_path.c_str(), "w");
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("tar_t::tar: failed to open tar archive at '{}': {}", install_tar_path, mtar_strerror(r)));
    }

    for (const auto file_to_tar : filesystem_t::find(dir, filesystem_t::is_regular && !filesystem_t::path(install_tar_path), filesystem_t::descend_all)) {
        std::ifstream ifs(file_to_tar.string(), std::ios::binary);
        if (!ifs) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("tar_t::tar: failed to open file '{}' for reading to add to tar archive at '{}'", file_to_tar, install_tar_path));
        }

        const auto file_size = filesystem_t::file_size(file_to_tar);
        if (std::numeric_limits<unsigned>::max() < file_size) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("tar_t::tar: file '{}' is too large to add to tar archive at '{}'", file_to_tar, install_tar_path));
        }

        const auto relative_path = dir.relative(file_to_tar);
        r = mtar_write_file_header(&mtar, relative_path.c_str(), file_size);
        if (r != MTAR_ESUCCESS) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("tar_t::tar: failed to write header for file '{}' to tar archive at '{}': {}", file_to_tar, install_tar_path, mtar_strerror(r)));
        }

        char buffer[4096];
        while (1) {
            ifs.read(buffer, sizeof(buffer));
            const auto read_count = ifs.gcount();
            if (0 < read_count) {
                r = mtar_write_data(&mtar, buffer, read_count);
                if (r != MTAR_ESUCCESS) {
                    std::string err_msg = mtar_strerror(r);
                    mtar_close(&mtar);
                    throw std::runtime_error(std::format("tar_t::tar: failed to write data for file '{}' to tar archive at '{}': {}", file_to_tar, install_tar_path, err_msg));
                }
            }

            if (ifs.eof()) {
                break ;
            }

            if (ifs.fail() || ifs.bad()) {
                mtar_close(&mtar);
                throw std::runtime_error(std::format("tar_t::tar: read failed from file '{}' to add to tar archive at '{}'", file_to_tar, install_tar_path));
            }
        }
    }

    r = mtar_finalize(&mtar);
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("tar_t::tar: failed to finalize tar archive at '{}': {}", install_tar_path, mtar_strerror(r)));
    }

    r = mtar_close(&mtar);
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("tar_t::tar: failed to close tar archive at '{}': {}", install_tar_path, mtar_strerror(r)));
    }

    return install_tar_path;
}

path_t tar_t::untar(const path_t& tar_path, const path_t& install_dir) {
    if (tar_path.extension() != ".tar") {
        throw std::runtime_error(std::format("tar_t::untar: tar path '{}' must have .tar extension", tar_path));
    }

    if (filesystem_t::exists(install_dir) && !filesystem_t::is_directory(install_dir)) {
        throw std::runtime_error(std::format("tar_t::untar: install path '{}' exists and is not a directory", install_dir));
    }

    if (!filesystem_t::exists(tar_path)) {
        throw std::runtime_error(std::format("tar_t::untar: tar path '{}' does not exist", tar_path));
    }

    std::cout << std::format("tar -xf {} -C {}", tar_path, install_dir) << std::endl;

    mtar_t mtar;
    int r = mtar_open(&mtar, tar_path.c_str(), "r");
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("tar_t::untar: failed to open tar archive at '{}': {}", tar_path, mtar_strerror(r)));
    }

    while (1) {
        mtar_header_t mtar_header;
        r = mtar_read_header(&mtar, &mtar_header);
        if (r == MTAR_ENULLRECORD) {
            break ;
        }

        if (r != MTAR_ESUCCESS) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("tar_t::untar: failed to read header from tar archive at '{}': {}", tar_path, mtar_strerror(r)));
        }

        const auto install_path = install_dir / relative_path_t(mtar_header.name);

        if (mtar_header.type == MTAR_TDIR) {
            if (!filesystem_t::exists(install_path)) {
                filesystem_t::create_directories(install_path);
            }

            mtar_next(&mtar);
            continue ;
        }

        const auto parent_dir = install_path.parent();
        if (!filesystem_t::exists(parent_dir)) {
            filesystem_t::create_directories(parent_dir);
        }

        std::ofstream ofs(install_path.string(), std::ios::binary | std::ios::trunc);
        if (!ofs) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("tar_t::untar: failed to open file '{}' for writing from tar archive at '{}'", install_path, tar_path));
        }

        char buffer[4096];
        auto read_remaining = mtar_header.size;
        while (0 < read_remaining) {
            const auto read_count = std::min(sizeof(buffer), static_cast<size_t>(read_remaining));
            r = mtar_read_data(&mtar, buffer, read_count);
            if (r != MTAR_ESUCCESS) {
                r = mtar_close(&mtar);
                throw std::runtime_error(std::format("tar_t::untar: failed to read data for file '{}' from tar archive at '{}': {}", install_path, tar_path, mtar_strerror(r)));
            }

            ofs.write(buffer, read_count);
            if (!ofs) {
                r = mtar_close(&mtar);
                throw std::runtime_error(std::format("tar_t::untar: failed to write data for file '{}' from tar archive at '{}'", install_path, tar_path));
            }

            read_remaining -= read_count;
        }

        mtar_next(&mtar);
    }

    r = mtar_close(&mtar);
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("failed to close tar archive at '{}': {}", tar_path, mtar_strerror(r)));
    }

    return install_dir;
}
