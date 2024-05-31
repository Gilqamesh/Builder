#include "example.h"

#include <stdio.h>

static int _;

void greet() {
    printf("Hello from simple example!\n");
}

int get() {
    return _;
}

void set(int value) {
    _ = value;
}
