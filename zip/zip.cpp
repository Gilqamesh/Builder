#include <modules/builder/zip/zip.h>
#include <modules/builder/zip/external/miniz.h>
#include <modules/builder/find/find.h>

#include <format>

std::filesystem::path zip_t::zip(const std::filesystem::path& dir, const std::filesystem::path& install_zip_path) {
    std::vector<std::filesystem::path> regular_files = find_t::find(dir, find_t::all, true);
    return zip(regular_files, install_zip_path);
}


std::filesystem::path zip_t::zip(const std::vector<std::filesystem::path>& regular_files, const std::filesystem::path& install_zip_path) {
    if (install_zip_path.extension() != ".zip") {
        throw std::runtime_error(std::format("install path '{}' must have .zip extension", install_zip_path.string()));
    }

    if (std::filesystem::exists(install_zip_path)) {
        throw std::runtime_error(std::format("install path '{}' already exists", install_zip_path.string()));
    }

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);

    if (!mz_zip_writer_init_file(&zip, install_zip_path.string().c_str(), 0)) {
        throw std::runtime_error(std::format("failed to create zip file at '{}'", install_zip_path.string()));
    }

    for (const auto& regular_file : regular_files) {
        if (!std::filesystem::exists(regular_file)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("file '{}' does not exist", regular_file.string()));
        }
        if (!std::filesystem::is_regular_file(regular_file)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("file '{}' is not a regular regular_file", regular_file.string()));
        }

        if (!mz_zip_writer_add_file(&zip, regular_file.filename().c_str(), regular_file.c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("failed to add file '{}' to zip archive", regular_file.string()));
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        throw std::runtime_error(std::format("failed to finalize zip archive at '{}'", install_zip_path.string()));
    }

    mz_zip_writer_end(&zip);

    return install_zip_path;
}

std::filesystem::path zip_t::unzip(const std::filesystem::path& zip_path, const std::filesystem::path& install_dir) {
    if (zip_path.extension() != ".zip") {
        throw std::runtime_error(std::format("zip file '{}' must have .zip extension", zip_path.string()));
    }

    if (std::filesystem::exists(install_dir) && !std::filesystem::is_directory(install_dir)) {
        throw std::runtime_error(std::format("install path '{}' exists and is not a directory", install_dir.string()));
    }

    if (!std::filesystem::exists(zip_path) || !std::filesystem::is_regular_file(zip_path)) {
        throw std::runtime_error(std::format("zip file '{}' does not exist or is not a regular file", zip_path.string()));
    }

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);

    if (!mz_zip_reader_init_file(&zip, zip_path.string().c_str(), 0)) {
        throw std::runtime_error(std::format("failed to open zip file at '{}'", zip_path.string()));
    }

    mz_uint num_files = mz_zip_reader_get_num_files(&zip);

    for (mz_uint i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat st;
        mz_zip_reader_file_stat(&zip, i, &st);

        std::filesystem::path out_path = install_dir / st.m_filename;

        if (st.m_is_directory) {
            std::error_code ec;
            if (!std::filesystem::create_directories(out_path, ec)) {
                mz_zip_reader_end(&zip);
                throw std::runtime_error(std::format("failed to create directory '{}', error: {}", out_path.string(), ec.message()));
            }
            continue ;
        }

        std::error_code ec;
        if (!std::filesystem::create_directories(out_path.parent_path(), ec)) {
            mz_zip_reader_end(&zip);
            throw std::runtime_error(std::format("failed to create directory '{}', error: {}", out_path.parent_path().string(), ec.message()));
        }

        if (!mz_zip_reader_extract_to_file(&zip, i, out_path.string().c_str(), 0)) {
            mz_zip_reader_end(&zip);
            throw std::runtime_error(std::format("failed to extract file '{}' to '{}'", st.m_filename, out_path.string()));
        }
    }

    mz_zip_reader_end(&zip);

    return install_dir;
}
