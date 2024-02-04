#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <time.h>

static time_t shared_lib_load_time;
static const char* shared_lib_path = "./libshared_lib.so";
static void* shared_lib;
typedef int (*get_dll_state_t)();
static get_dll_state_t get_dll_state;

static int reload_shared_lib() {
    if (shared_lib) {
        dlclose(shared_lib);
    }

    shared_lib = dlopen(shared_lib_path, RTLD_LAZY);
    if (!shared_lib) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    get_dll_state = (get_dll_state_t) dlsym(shared_lib, "get_state");
    if (!get_dll_state) {
        fprintf(stderr, "%s\n", dlerror());
        return 2;
    }

    printf("Reloaded shared library.\n");

    return 0;
}

int main() {
    while (1) {
        struct stat shared_lib_info;
        if (stat(shared_lib_path, &shared_lib_info) == -1) {
            perror(0);
            exit(1);
        }
        if (shared_lib_info.st_mtime != shared_lib_load_time) {
            if (!reload_shared_lib()) {
                shared_lib_load_time = shared_lib_info.st_mtime;
            }
        }

        if (get_dll_state) {
            printf("Dll state: %d\n", get_dll_state());
        } else {
            printf("Dll is not loaded.\n");
        }
        usleep(250000);
    }

    return 0;
}
