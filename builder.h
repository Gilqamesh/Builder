#ifndef BUILD_H
# define BUILD_H

# define _GNU_SOURCE
# include <stdarg.h>
# include <stddef.h>
# include <time.h>
# include <pthread.h>

typedef struct builder* builder_t;
typedef struct obj* obj_t;

// Module level

int    builder__init();
void   builder__deinit();
double builder__get_time_stamp();
double builder__get_time_stamp_init();

// Program types

obj_t       obj__file_modified(obj_t opt_inputs, const char* path, ...);
const char* obj__file_modified_path(obj_t self);
obj_t obj__sh(obj_t opt_inputs, int success_status_code, const char* cmd_line, ...);
obj_t obj__oscillator(obj_t input, double periodicity_ms);
obj_t obj__time();
obj_t obj__thread(obj_t input_to_thread, obj_t collector);

obj_t obj__list(obj_t obj0, ... /*, 0*/);

// Program methods

struct attr obj__get_attr(obj_t self);

// void obj__remove_input(obj_t self, obj_t what);
// void obj__push_input(obj_t self, obj_t what);
// void obj__remove_output(obj_t self, obj_t what);
// void obj__push_output(obj_t self, obj_t what);

int  obj__run(obj_t self);
void obj__describe_short(obj_t self, char* buffer, int buffer_size);
void obj__describe_long(obj_t self, char* buffer, int buffer_size);

struct obj {
    size_t inputs_top;
    size_t inputs_size;
    obj_t* inputs;

    size_t outputs_top;
    size_t outputs_size;
    obj_t* outputs;

    void (*run)(obj_t self);
    void (*describe_short)(obj_t self, char* buffer, int buffer_size); // couple characters in a single line
    void (*describe_long)(obj_t self, char* buffer, int buffer_size); // can be as long as necessary and multiline
    void (*destroy)(obj_t self);

    struct attr {
        int    is_running;
        double time_start; // [s] time since unix epoch
        double time_success;
        double time_fail;
        double time_finish;
        size_t n_run;
        size_t n_success;
        size_t n_fail;
    } attr;
    pthread_mutex_t mutex_attr;

    pthread_t thread;
    obj_t     thread_watcher;
    int       is_thread_watcher;

    int transient_flag; // Should be cleared to 0 after using it
};

#endif // BUILD_H
