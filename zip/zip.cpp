#include <modules/builder/zip/zip.h>
#include <modules/builder/zip/external/miniz.h>

#include <format>
#include <iostream>

path_t zip_t::zip(const path_t& dir, const path_t& install_zip_path) {

    const auto regular_files = filesystem_t::find(dir, filesystem_t::is_regular, filesystem_t::descend_all);
    return zip(regular_files, install_zip_path);
}

path_t zip_t::zip(const std::vector<path_t>& regular_files, const path_t& install_zip_path) {
    if (install_zip_path.extension() != ".zip") {
        throw std::runtime_error(std::format("zip_t::zip: install path '{}' must have .zip extension", install_zip_path.string()));
    }

    if (filesystem_t::exists(install_zip_path)) {
        throw std::runtime_error(std::format("zip_t::zip: install path '{}' already exists", install_zip_path.string()));
    }

    std::cout << std::format("zip -c {} {}", install_zip_path.string(), [&regular_files]() {
        std::string files_str;
        for (const auto& regular_file : regular_files) {
            if (!files_str.empty()) {
                files_str += " ";
            }
            files_str += regular_file.string();
        }
        return files_str;
    }()) << std::endl;

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);

    if (!mz_zip_writer_init_file(&zip, install_zip_path.string().c_str(), 0)) {
        throw std::runtime_error(std::format("zip_t::zip: failed to create zip file at '{}'", install_zip_path.string()));
    }

    for (const auto& regular_file : regular_files) {
        if (!filesystem_t::exists(regular_file)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("zip_t::zip: file '{}' does not exist", regular_file.string()));
        }
        if (!filesystem_t::is_regular_file(regular_file)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("zip_t::zip: file '{}' is not a regular regular_file", regular_file.string()));
        }

        if (!mz_zip_writer_add_file(&zip, regular_file.filename().c_str(), regular_file.c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("zip_t::zip: failed to add file '{}' to zip archive", regular_file.string()));
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        throw std::runtime_error(std::format("zip_t::zip: failed to finalize zip archive at '{}'", install_zip_path.string()));
    }

    mz_zip_writer_end(&zip);

    return install_zip_path;
}

path_t zip_t::unzip(const path_t& zip_path, const path_t& install_dir) {
    if (zip_path.extension() != ".zip") {
        throw std::runtime_error(std::format("zip_t::unzip: zip file '{}' must have .zip extension", zip_path.string()));
    }

    if (filesystem_t::exists(install_dir) && !filesystem_t::is_directory(install_dir)) {
        throw std::runtime_error(std::format("zip_t::unzip: install path '{}' exists and is not a directory", install_dir.string()));
    }

    if (!filesystem_t::exists(zip_path) || !filesystem_t::is_regular_file(zip_path)) {
        throw std::runtime_error(std::format("zip_t::unzip: zip file '{}' does not exist or is not a regular file", zip_path.string()));
    }

    std::cout << std::format("unzip -d {} {}", install_dir.string(), zip_path.string()) << std::endl;

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);

    if (!mz_zip_reader_init_file(&zip, zip_path.string().c_str(), 0)) {
        throw std::runtime_error(std::format("zip_t::unzip: failed to open zip file at '{}'", zip_path.string()));
    }

    mz_uint num_files = mz_zip_reader_get_num_files(&zip);

    for (mz_uint i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat st;
        mz_zip_reader_file_stat(&zip, i, &st);

        path_t out_path = install_dir / relative_path_t(st.m_filename);

        if (st.m_is_directory) {
            filesystem_t::create_directories(out_path);
            continue ;
        }

        const auto parent = out_path.parent();
        if (!filesystem_t::exists(parent)) {
            filesystem_t::create_directories(out_path.parent());
        }

        if (!mz_zip_reader_extract_to_file(&zip, i, out_path.string().c_str(), 0)) {
            mz_zip_reader_end(&zip);
            throw std::runtime_error(std::format("zip_t::unzip: failed to extract file '{}' to '{}'", st.m_filename, out_path.string()));
        }
    }

    mz_zip_reader_end(&zip);

    return install_dir;
}
