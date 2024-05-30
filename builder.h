#ifndef BUILD_H
# define BUILD_H

# include <stdarg.h>
# include <stddef.h>
# include <time.h>

typedef struct builder* builder_t;
typedef struct obj* obj_t;

// Module level

void builder__init();
double builder__get_time_stamp();
double builder__get_time_stamp_init();

// Program types

obj_t       obj__file_modified(obj_t opt_inputs, const char* path, ...);
const char* obj__file_modified_path(obj_t self);
obj_t obj__sh(obj_t opt_inputs, int success_status_code, const char* cmd_line, ...);
obj_t obj__oscillator(obj_t input, double periodicity_ms);
obj_t obj__time();
obj_t obj__thread(obj_t input_to_thread);

obj_t obj__list(obj_t obj0, ... /*, 0*/);

// Program methods

// void obj__remove_input(obj_t self, obj_t what);
// void obj__push_input(obj_t self, obj_t what);
// void obj__remove_output(obj_t self, obj_t what);
// void obj__push_output(obj_t self, obj_t what);

int  obj__run(obj_t self);
void obj__describe_short(obj_t self, char* buffer, int buffer_size);
void obj__describe_long(obj_t self, char* buffer, int buffer_size);

// Obj definition

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

    /**
     * Each program needs to set these via these setters
     *  obj__set_start
     *  obj__set_finish
     *  obj__set_success
     *  obj__set_fail
     * 
     * >=0 - time since epoch in seconds
    */
    double time_ran_success;
    double time_ran_fail;
    double time_ran_start;
    double time_ran_finish;
    size_t number_of_times_ran_total;
    size_t number_of_times_ran_failed;
    size_t number_of_times_ran_successfully;

    /**
     * =0 - no running process
     * >0 - pid of running process
    */
    pid_t pid;
    int   pid_pipe_stdout[2];
    int   pid_pipe_stderr[2];

    /**
     * =0 - default success status code
     * !0 - user defined success status code
    */
    int success_status_code;

    /**
     * Should be cleared to 0 after using it
    */
    int transient_flag;
};

#endif // BUILD_H
