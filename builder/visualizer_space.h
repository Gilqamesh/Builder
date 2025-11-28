#ifndef VISUALIZER_SPACE_H
#define VISUALIZER_SPACE_H

#include "function.h"

struct rec_t {
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
};

float to_view_x(float x, rec_t view_rec, rec_t world_rec);
float to_view_y(float y, rec_t view_rec, rec_t world_rec);
float from_view_x(float x, rec_t view_rec, rec_t world_rec);
float from_view_y(float y, rec_t view_rec, rec_t world_rec);
rec_t to_view(rec_t rec, rec_t view_rec, rec_t world_rec);
rec_t to_view(function_t* function, function_t* focus_function, rec_t view_rec, rec_t world_rec);

#endif // VISUALIZER_SPACE_H
