#include "proc.h"

#include <stdio.h>

void proc__child(proc_t self) {
    proc__write(self, "Allocating some resources from child...");
    for (int i = 0; i < 200; ++i) {
        proc__alloc(self, 54 + i * 4);
    }
}

int main() {
    proc_t proc = proc__create(0);
    if (!proc) {
        return 1;
    }

    if (proc__run(proc, &proc__child)) {
        proc__destroy(proc);
        return 1;
    }

    do {
        char buffer[256];
        if (0 < proc__read(proc, buffer, sizeof(buffer))) {
            printf("Read from child: %s\n", buffer);
        }
    } while (proc__wait(proc, 0));

    proc__print_shared(proc);

    proc__destroy(proc);

    return 0;
}
