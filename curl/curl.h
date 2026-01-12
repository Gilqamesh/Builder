#ifndef BUILDER_PROJECT_CURL_CURL_H
# define BUILDER_PROJECT_CURL_CURL_H

# include <modules/builder/module/module_graph.h>
# include <modules/builder/filesystem/filesystem.h>

# include <string>

class curl_t {
public:
    static path_t download(const std::string& url, const path_t& install_path);
};

#endif // BUILDER_PROJECT_CURL_CURL_H
