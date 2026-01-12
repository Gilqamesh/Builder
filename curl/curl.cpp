#include <modules/builder/curl/curl.h>

#include <curl/curl.h>

#include <iostream>
#include <format>
#include <fstream>

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::ofstream* ofs = static_cast<std::ofstream*>(userdata);
    ofs->write(ptr, size * nmemb);
    return size * nmemb;
}

std::filesystem::path curl_t::download(const std::string& url, const std::filesystem::path& install_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("failed to initialize curl");
    }

    std::cout << std::format("curl -L {} -o {}", url, install_path.string()) << std::endl;

    struct guard_t {
        guard_t(CURL* c) : curl(c) {}
        ~guard_t() { if (curl) { curl_easy_cleanup(curl); } }

        CURL* curl;
    } guard(curl);

    if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK) {
        throw std::runtime_error(std::format("failed to set URL '{}'", url));
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback) != CURLE_OK) {
        throw std::runtime_error(std::format("failed to set write callback for URL '{}'", url));
    }

    if (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
        throw std::runtime_error(std::format("failed to set follow location for URL '{}'", url));
    }

    std::ofstream ofs(install_path, std::ios::binary | std::ios::trunc);
    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs) != CURLE_OK) {
        throw std::runtime_error(std::format("failed to set write callback for URL '{}'", url));
    }

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        throw std::runtime_error(std::format("failed to download file from URL '{}': {}", url, curl_easy_strerror(result)));
    }

    if (!ofs) {
        throw std::runtime_error(std::format("failed to write downloaded file to '{}'", install_path.string()));
    }

    return install_path;
}
