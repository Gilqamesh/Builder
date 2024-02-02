#ifndef BUILD_H
# define BUILD_H

# include <stdarg.h>

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
void module__wait_for_compilation(module_t self);

void module__link(module_t self, const char* module_linker_path, int* optional_status_code);

/********************************************************************************
 * File level API
 ********************************************************************************/

typedef struct module_file* module_file_t;

module_file_t module__add_file(module_t self, const char* file_compiler_path);

void module_file__prepend_cflag(module_file_t self, const char* cflag_printf_format, ...);
void module_file__append_cflag(module_file_t self, const char* cflag_printf_format, ...);

#endif // BUILD_H
