#include "shared_lib.h"

static int state1 = 0;
static int state2 = 1;

int next_state() {
    int next = state1 + state2;
    state1 = state2;
    state2 = next;
    return next;
}
