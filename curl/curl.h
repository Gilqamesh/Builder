#ifndef BUILDER_PROJECT_CURL_CURL_H
# define BUILDER_PROJECT_CURL_CURL_H

# include <modules/builder/module/module_graph.h>

# include <filesystem>
# include <string>

class curl_t {
public:
    static std::filesystem::path download(const std::string& url, const std::filesystem::path& install_path);
};

#endif // BUILDER_PROJECT_CURL_CURL_H
