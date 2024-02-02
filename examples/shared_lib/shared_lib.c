#include "shared_lib.h"

static int state = 0;

int get_state() {
    return state++;
}
