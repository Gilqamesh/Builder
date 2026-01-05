#include <modules/builder/zip/zip.h>
#include <modules/builder/zip/external/miniz.h>

#include <format>

std::filesystem::path zip_t::zip(const std::filesystem::path& dir, const std::filesystem::path& install_zip_path) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        throw std::runtime_error(std::format("source dir '{}' does not exist or is not a directory", dir.string()));
    }

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

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue ;
        }

        auto rel = std::filesystem::relative(entry.path(), dir);
        auto archive_name = rel.generic_string();

        if (!mz_zip_writer_add_file(&zip, archive_name.c_str(), entry.path().string().c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("failed to add file '{}' to zip archive", entry.path().string()));
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
            std::filesystem::create_directories(out_path);
            continue ;
        }

        std::filesystem::create_directories(out_path.parent_path());

        if (!mz_zip_reader_extract_to_file(&zip, i, out_path.string().c_str(), 0)) {
            mz_zip_reader_end(&zip);
            throw std::runtime_error(std::format("failed to extract file '{}' to '{}'", st.m_filename, out_path.string()));
        }
    }

    mz_zip_reader_end(&zip);

    return install_dir;
}
