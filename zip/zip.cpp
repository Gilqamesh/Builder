#include "zip.h"
#include "external/miniz.h"

#include <format>
#include <iostream>

namespace zip {

filesystem::path_t zip(const filesystem::path_t& dir, const filesystem::path_t& install_zip_path) {
    if (install_zip_path.extension() != ".zip") {
        throw std::runtime_error(std::format("zip::zip: install path '{}' must have .zip extension", install_zip_path));
    }

    if (filesystem::exists(install_zip_path)) {
        throw std::runtime_error(std::format("zip::zip: install path '{}' already exists", install_zip_path));
    }

    const auto regular_files = filesystem::find(dir, filesystem::find_include_predicate_t::is_regular, filesystem::find_descend_predicate_t::descend_all);

    std::cout << std::format("zip -c {} {}", install_zip_path, [&regular_files]() {
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
        throw std::runtime_error(std::format("zip::zip: failed to create zip file at '{}'", install_zip_path));
    }

    for (const auto& regular_file : regular_files) {
        if (!filesystem::exists(regular_file)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("zip::zip: file '{}' does not exist", regular_file));
        }
        if (!filesystem::is_regular_file(regular_file)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("zip::zip: file '{}' is not a regular regular_file", regular_file));
        }

        const auto rel = dir.relative(regular_file);
        if (!mz_zip_writer_add_file(&zip, rel.c_str(), regular_file.c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION)) {
            mz_zip_writer_end(&zip);
            throw std::runtime_error(std::format("zip::zip: failed to add file '{}' to zip archive", regular_file));
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        throw std::runtime_error(std::format("zip::zip: failed to finalize zip archive at '{}'", install_zip_path));
    }

    mz_zip_writer_end(&zip);

    return install_zip_path;
}

filesystem::path_t unzip(const filesystem::path_t& zip_path, const filesystem::path_t& install_dir) {
    if (zip_path.extension() != ".zip") {
        throw std::runtime_error(std::format("zip::unzip: zip file '{}' must have .zip extension", zip_path));
    }

    if (filesystem::exists(install_dir) && !filesystem::is_directory(install_dir)) {
        throw std::runtime_error(std::format("zip::unzip: install path '{}' exists and is not a directory", install_dir));
    }

    if (!filesystem::exists(zip_path) || !filesystem::is_regular_file(zip_path)) {
        throw std::runtime_error(std::format("zip::unzip: zip file '{}' does not exist or is not a regular file", zip_path));
    }

    std::cout << std::format("unzip -d {} {}", install_dir, zip_path) << std::endl;

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);

    if (!mz_zip_reader_init_file(&zip, zip_path.string().c_str(), 0)) {
        throw std::runtime_error(std::format("zip::unzip: failed to open zip file at '{}'", zip_path));
    }

    mz_uint num_files = mz_zip_reader_get_num_files(&zip);

    for (mz_uint i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat st;
        mz_zip_reader_file_stat(&zip, i, &st);

        filesystem::path_t out_path = install_dir / filesystem::relative_path_t(st.m_filename);

        if (st.m_is_directory) {
            filesystem::create_directories(out_path);
            continue ;
        }

        const auto parent = out_path.parent();
        if (!filesystem::exists(parent)) {
            filesystem::create_directories(out_path.parent());
        }

        if (!mz_zip_reader_extract_to_file(&zip, i, out_path.string().c_str(), 0)) {
            mz_zip_reader_end(&zip);
            throw std::runtime_error(std::format("zip::unzip: failed to extract file '{}' to '{}'", st.m_filename, out_path));
        }
    }

    mz_zip_reader_end(&zip);

    return install_dir;
}

} // namespace zip
