#ifndef BUILDER_PROJECT_BUILDER_CURL_CURL_H
# define BUILDER_PROJECT_BUILDER_CURL_CURL_H

# include "../filesystem/filesystem.h"

# include <string>

class curl_t {
public:
    static path_t download(const std::string& url, const path_t& install_path);
};

#endif // BUILDER_PROJECT_BUILDER_CURL_CURL_H
