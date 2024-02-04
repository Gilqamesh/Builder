#ifndef BUILD_H
# define BUILD_H

# include <stdarg.h>
# include <time.h>

/********************************************************************************
 * Module level API
 ********************************************************************************/

typedef struct module* module_t;

module_t module__create();
void module__destroy(module_t self);

void module__prepend_cflag(module_t self, const char* cflag_printf_format, ...);
void module__append_cflag(module_t self, const char* cflag_printf_format, ...);
void module__prepend_lflag(module_t self, const char* lflag_printf_format, ...);
void module__append_lflag(module_t self, const char* lflag_printf_format, ...);

void module__add_dependency(module_t self, module_t dependency);

void module__compile_with_dependencies(module_t self);
void module__relink(module_t self, const char* module_linker_path, int* optional_status_code);

/********************************************************************************
 * File level API
 ********************************************************************************/

typedef struct module_file* module_file_t;

module_file_t module__add_file(module_t self, const char* opt_file_compiler_path, const char* file_path_printf_format, ...);

const char* module_file__path(module_file_t self);

void module_file__prepend_cflag(module_file_t self, const char* cflag_printf_format, ...);
void module_file__append_cflag(module_file_t self, const char* cflag_printf_format, ...);

void module_file__add_dependency(module_file_t self, module_file_t file);

/********************************************************************************
 * Continuous build API
 ********************************************************************************/

void module__rebuild_forever(module_t self, const char* linker_path, void (*opt_module__execute)(void* opt_user_data), void* opt_user_data);

#endif // BUILD_H
