#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <time.h>

static time_t shared_lib_load_time;
static void* shared_lib;
typedef int (*next_dll_state_t)();
static next_dll_state_t next_dll_state;

static void reload_shared_lib() {
    const char* shared_lib_path = "./libshared_lib.so";

    struct stat shared_lib_info;
    if (stat(shared_lib_path, &shared_lib_info) == -1) {
        perror(0);
        exit(1);
    }

    if (shared_lib_info.st_mtime == shared_lib_load_time) {
        return ;
    }

    if (shared_lib) {
        next_dll_state = 0;
        dlclose(shared_lib);
    }

    shared_lib = dlopen(shared_lib_path, RTLD_NOW);
    if (shared_lib == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return ;
    }

    shared_lib_load_time = shared_lib_info.st_mtime;

    next_dll_state = (next_dll_state_t) dlsym(shared_lib, "next_state");
    if (next_dll_state == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return ;
    }

    printf("Reloaded shared library.\n");
}

int main() {
    while (1) {
        reload_shared_lib();

        if (next_dll_state) {
            printf("Dll state: %d\n", next_dll_state());
        } else {
            printf("Dll is not loaded.\n");
        }
        usleep(250000);
    }

    return 0;
}
