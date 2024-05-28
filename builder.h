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
obj_t obj__sh(obj_t opt_inputs, obj_t opt_outputs, obj_t collector_if_async, int success_status_code, const char* cmd_line, ...);
obj_t obj__oscillator(obj_t input, double periodicity_ms);
obj_t obj__time();

obj_t obj__list(obj_t obj0, ... /*, 0*/);

// Program methods

// void obj__remove_input(obj_t self, obj_t what);
// void obj__push_input(obj_t self, obj_t what);
// void obj__remove_output(obj_t self, obj_t what);
// void obj__push_output(obj_t self, obj_t what);

void obj__run(obj_t self);
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

    void (*run)(obj_t self, obj_t parent);
    void (*describe_short)(obj_t self, char* buffer, int buffer_size); // couple characters in a single line
    void (*describe_long)(obj_t self, char* buffer, int buffer_size); // can be as long as necessary and multiline
    void (*destroy)(obj_t self);

    /**
     * Todo: create time type
     * Assertions:
     *  - must be >=0
     *  - time_t time_a = time(0)
     *  - double time_b = builder__get_time_stamp()
     *  - time_a ~= time_b
    */
    /**
     * =0 - ran at epoch
     * >0 - time of successful run since epoch
    */
    double time_ran;

    enum {
        RUN_RESULT_NO_CHANGE,
        RUN_RESULT_SUCESSS,
        RUN_RESULT_FAILED
    } last_run_result;

    enum {
        NOT_RUNNING,
        IS_RUNNING
    } is_running;
    obj_t  collector;

    /**
     * Since builder is running
    */
    size_t number_of_times_ran_total;
    size_t number_of_times_ran_failed;
    size_t number_of_times_ran_successfully;

    /**
     * <0 - undefined
     * =0 - not locked
     * >0 - locked for run
    */
    int is_locked;
    
    /**
     * <0 - undefined
     * =0 - not running
     * >0 - running, must be async program
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
