#include "lib.h"

#include <math.h>

#define F_EPSILON 0.0001f

float next(float a) {
    return nextafterf(a, a + F_EPSILON);
}

float prev(float a) {
    return nextafterf(a, a - F_EPSILON);
}
