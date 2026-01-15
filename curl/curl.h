#ifndef CURL_CURL_H
# define CURL_CURL_H

# include "../filesystem/filesystem.h"

# include <string>

namespace curl {

filesystem::path_t download(const std::string& url, const filesystem::path_t& install_path);

} // namespace curl

#endif // CURL_CURL_H
