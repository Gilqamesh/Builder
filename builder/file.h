#ifndef NODE_FILE_H
# define NODE_FILE_H

# include "core/ir.h"

struct file_identifier_t {
    std::string ns;
    std::string proc;
};

struct file_locator_t {
    file_identifier_t identifier;
    std::chrono::system_clock::time_point creation_time;
};

/**
 * Represents a persistently stored node file.
*/
struct node_file_t {
    std::filesystem::path path;

    static node_file_t find(const file_locator_t& locator);
    static node_file_t find(const file_identifier_t& identifier);
    static node_file_t save(const ir_t& ir, const file_identifier_t& identifier);
    static ir_t load(const node_file_t& file);
};

#endif // NODE_FILE_H
