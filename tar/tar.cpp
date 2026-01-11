#include <modules/builder/tar/tar.h>
#include <modules/builder/tar/external/microtar.h>
#include <modules/builder/find/find.h>

#include <format>
#include <fstream>

std::filesystem::path tar_t::tar(const std::filesystem::path& dir, const std::filesystem::path& install_tar_path) {
    if (install_tar_path.extension() != ".tar") {
        throw std::runtime_error(std::format("install path '{}' must have .tar extension", install_tar_path.string()));
    }

    if (std::filesystem::exists(install_tar_path)) {
        throw std::runtime_error(std::format("install path '{}' already exists", install_tar_path.string()));
    }

    mtar_t mtar;
    int r = mtar_open(&mtar, install_tar_path.string().c_str(), "w");
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("failed to open tar archive at '{}': {}", install_tar_path.string(), mtar_strerror(r)));
    }

    const auto normalized_dir = std::filesystem::absolute(dir).lexically_normal();
    const auto files_to_tar = find_t::find(normalized_dir, find_t::all, true);

    const auto normalized_install_tar_path = std::filesystem::absolute(install_tar_path).lexically_normal();
    for (const auto file_to_tar : files_to_tar) {
        const auto normalized_file_to_tar = std::filesystem::absolute(file_to_tar).lexically_normal();
        if (normalized_file_to_tar == normalized_install_tar_path) {
            continue ;
        }

        const auto relative_path = file_to_tar.lexically_relative(normalized_dir);
        if (relative_path.empty() || relative_path.native().starts_with("..")) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("invalid path '{}' for file to add to tar archive at '{}'", relative_path.string(), install_tar_path.string()));
        }


        std::ifstream ifs(file_to_tar);
        if (!ifs) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("failed to open file '{}' for reading to add to tar archive at '{}'", file_to_tar.string(), install_tar_path.string()));
        }

        const auto file_size = std::filesystem::file_size(file_to_tar);
        if (std::numeric_limits<unsigned>::max() < file_size) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("file '{}' is too large to add to tar archive at '{}'", file_to_tar.string(), install_tar_path.string()));
        }
        r = mtar_write_file_header(&mtar, relative_path.c_str(), file_size);
        if (r != MTAR_ESUCCESS) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("failed to write header for file '{}' to tar archive at '{}': {}", relative_path.string(), install_tar_path.string(), mtar_strerror(r)));
        }

        char buffer[4096];
        while (ifs) {
            ifs.read(buffer, sizeof(buffer));
            const auto read_count = ifs.gcount();
            if (read_count == 0) {
                break ;
            }

            r = mtar_write_data(&mtar, buffer, read_count);
            if (r != MTAR_ESUCCESS) {
                r = mtar_close(&mtar);
                throw std::runtime_error(std::format("failed to write data for file '{}' to tar archive at '{}': {}", relative_path.string(), install_tar_path.string(), mtar_strerror(r)));
            }
        }
    }

    r = mtar_finalize(&mtar);
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("failed to finalize tar archive at '{}': {}", install_tar_path.string(), mtar_strerror(r)));
    }

    r = mtar_close(&mtar);
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("failed to close tar archive at '{}': {}", install_tar_path.string(), mtar_strerror(r)));
    }

    return install_tar_path;
}

std::filesystem::path tar_t::untar(const std::filesystem::path& tar_path, const std::filesystem::path& install_dir) {
    if (tar_path.extension() != ".tar") {
        throw std::runtime_error(std::format("tar path '{}' must have .tar extension", tar_path.string()));
    }

    if (std::filesystem::exists(install_dir) && !std::filesystem::is_directory(install_dir)) {
        throw std::runtime_error(std::format("install path '{}' exists and is not a directory", install_dir.string()));
    }

    if (!std::filesystem::exists(tar_path)) {
        throw std::runtime_error(std::format("tar path '{}' does not exist", tar_path.string()));
    }

    const auto normalized_install_dir = std::filesystem::absolute(install_dir).lexically_normal();

    mtar_t mtar;
    int r = mtar_open(&mtar, tar_path.string().c_str(), "r");
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("failed to open tar archive at '{}': {}", tar_path.string(), mtar_strerror(r)));
    }

    while (1) {
        mtar_header_t mtar_header;
        r = mtar_read_header(&mtar, &mtar_header);
        if (r == MTAR_ENULLRECORD) {
            break ;
        }

        if (r != MTAR_ESUCCESS) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("failed to read header from tar archive at '{}': {}", tar_path.string(), mtar_strerror(r)));
        }

        const auto install_path = normalized_install_dir / mtar_header.name;
        const auto rel = install_path.lexically_relative(normalized_install_dir);
        if (rel.empty() || rel.native().starts_with("..")) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("invalid path '{}' in tar archive at '{}'", mtar_header.name, tar_path.string()));
        }

        const auto parent_dir = install_path.parent_path();
        if (!std::filesystem::exists(parent_dir)) {
            if (!std::filesystem::create_directories(parent_dir)) {
                r = mtar_close(&mtar);
                throw std::runtime_error(std::format("failed to create parent directory '{}' for file '{}' from tar archive at '{}'", parent_dir.string(), install_path.string(), tar_path.string()));
            }
        }

        std::ofstream ofs(install_path);
        if (!ofs) {
            r = mtar_close(&mtar);
            throw std::runtime_error(std::format("failed to open file '{}' for writing from tar archive at '{}'", install_path.string(), tar_path.string()));
        }

        char buffer[4096];
        auto read_remaining = mtar_header.size;
        while (0 < read_remaining) {
            const auto read_count = std::min(sizeof(buffer), static_cast<size_t>(read_remaining));
            r = mtar_read_data(&mtar, buffer, read_count);
            if (r != MTAR_ESUCCESS) {
                r = mtar_close(&mtar);
                throw std::runtime_error(std::format("failed to read data for file '{}' from tar archive at '{}': {}", install_path.string(), tar_path.string(), mtar_strerror(r)));
            }

            ofs.write(buffer, read_count);
            if (!ofs) {
                r = mtar_close(&mtar);
                throw std::runtime_error(std::format("failed to write data for file '{}' from tar archive at '{}'", install_path.string(), tar_path.string()));
            }

            read_remaining -= read_count;
        }

        mtar_next(&mtar);
    }

    r = mtar_close(&mtar);
    if (r != MTAR_ESUCCESS) {
        throw std::runtime_error(std::format("failed to close tar archive at '{}': {}", tar_path.string(), mtar_strerror(r)));
    }

    return normalized_install_dir;
}
