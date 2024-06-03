#include "simple.h"

#include <stdio.h>

typedef struct obj_greet {
    struct obj base;
} *obj_greet_t;

static void obj__run_greet(obj_t self);
static void obj__describe_short_greet(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_greet(obj_t self, char* buffer, int buffer_size);
static void obj__destroy_greet(obj_t self);

static void obj__run_greet(obj_t self) {
    const double time_cur = builder__get_time_stamp();
    obj__set_start(self, time_cur);
    obj__print(self, "Hello from example!");
    obj__set_success(self, time_cur);
    obj__set_finish(self, time_cur);
}

static void obj__describe_short_greet(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "GREET");
}

static void obj__describe_long_greet(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "GREET");
}

static void obj__destroy_greet(obj_t self) {
    (void) self;
}

obj_t obj__greet() {
    obj_greet_t result = (obj_greet_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_greet;
    result->base.describe_short = &obj__describe_short_greet;
    result->base.describe_long  = &obj__describe_long_greet;
    result->base.destroy        = &obj__destroy_greet;

    return (obj_t) result;
}
