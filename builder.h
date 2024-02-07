#ifndef BUILD_H
# define BUILD_H

# include <stdarg.h>
# include <time.h>

typedef struct obj* obj_t;

void builder__init();

// file api
obj_t       obj__file(const char* path, ...);
const char* obj__file_path(obj_t self);

// process api
// todo: success status code input for processor
obj_t obj__process(obj_t opt_dep_in, obj_t opt_dep_out, const char* cmd_line, ...);

// generic api

void obj__destroy(obj_t self);

obj_t obj__dependencies(obj_t dep0, ... /*, 0*/);

void obj__print(obj_t self);
void obj__build(obj_t self);

#endif // BUILD_H
