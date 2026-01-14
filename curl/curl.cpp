#include "curl.h"

#include <curl/curl.h>

#include <iostream>
#include <format>
#include <fstream>

struct write_result_t {
    std::ofstream& ofs;
    bool failed;
};

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    write_result_t* write_result = static_cast<write_result_t*>(userdata);
    write_result->ofs.write(ptr, size * nmemb);
    if (!write_result->ofs) {
        write_result->failed = true;
        return 0;
    }

    return size * nmemb;
}

path_t curl_t::download(const std::string& url, const path_t& install_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("curl_t::download: failed to initialize curl");
    }

    const auto install_path_parent = install_path.parent();
    if (!filesystem_t::exists(install_path_parent)) {
        filesystem_t::create_directories(install_path_parent);
    }

    std::cout << std::format("curl -L {} -o {}", url, install_path) << std::endl;

    struct guard_t {
        guard_t(CURL* c) : curl(c) {}
        ~guard_t() { if (curl) { curl_easy_cleanup(curl); } }

        CURL* curl;
    } guard(curl);

    if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK) {
        throw std::runtime_error(std::format("curl_t::download: failed to set URL '{}'", url));
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback) != CURLE_OK) {
        throw std::runtime_error(std::format("curl_t::download: failed to set write callback for URL '{}'", url));
    }

    if (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
        throw std::runtime_error(std::format("curl_t::download: failed to set follow location for URL '{}'", url));
    }

    std::ofstream ofs(install_path.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("curl_t::download: failed to open file '{}' for writing", install_path));
    }

    write_result_t write_result = {
        .ofs = ofs,
        .failed = false
    };
    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result) != CURLE_OK) {
        throw std::runtime_error(std::format("curl_t::download: failed to set write callback for URL '{}'", url));
    }

    CURLcode result = curl_easy_perform(curl);

    if (write_result.failed) {
        throw std::runtime_error(std::format("curl_t::download: failed to write downloaded data to file '{}'", install_path));
    }

    if (result != CURLE_OK) {
        throw std::runtime_error(std::format("curl_t::download: failed to download file from URL '{}': {}", url, curl_easy_strerror(result)));
    }

    return install_path;
}
