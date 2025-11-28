#include "visualizer_space.h"

#include <cassert>

float to_view_x(float x, rec_t view_rec, rec_t world_rec) {
    const float result = (x - world_rec.left) * (view_rec.right - view_rec.left) / (world_rec.right - world_rec.left) + view_rec.left;
    return result;
}

float to_view_y(float y, rec_t view_rec, rec_t world_rec) {
    const float result = (y - world_rec.top) * (view_rec.bottom - view_rec.top) / (world_rec.bottom - world_rec.top) + view_rec.top;
    return result;
}

float from_view_x(float x, rec_t view_rec, rec_t world_rec) {
    const float result = (x - view_rec.left) * (world_rec.right - world_rec.left) / (view_rec.right - view_rec.left) + world_rec.left;
    return result;
}

float from_view_y(float y, rec_t view_rec, rec_t world_rec) {
    const float result = (y - view_rec.top) * (world_rec.bottom - world_rec.top) / (view_rec.bottom - view_rec.top) + world_rec.top;
    return result;
}

rec_t to_view(rec_t rec, rec_t view_rec, rec_t world_rec) {
    rec.left = to_view_x(rec.left, view_rec, world_rec);
    rec.right = to_view_x(rec.right, view_rec, world_rec);
    rec.top = to_view_y(rec.top, view_rec, world_rec);
    rec.bottom = to_view_y(rec.bottom, view_rec, world_rec);

    return rec;
}

rec_t to_view(function_t* function, function_t* focus_function, rec_t view_rec, rec_t world_rec) {
    assert(function);
    assert(focus_function);

    rec_t result;

    if (function == focus_function) {
        result.left = -focus_function->coordinate_system_width() / 2.0f;
        result.right = focus_function->coordinate_system_width() / 2.0f;
        result.top = -focus_function->coordinate_system_height() / 2.0f;
        result.bottom = focus_function->coordinate_system_height() / 2.0f;
    } else {
        result.left = function->left();
        result.right = function->right();
        result.top = function->top();
        result.bottom = function->bottom();

        while (function->parent() != focus_function) {
            function = function->parent();
            assert(function);
            result.left = function->from_child_x(result.left);
            result.right = function->from_child_x(result.right);
            result.top = function->from_child_y(result.top);
            result.bottom = function->from_child_y(result.bottom);
        }
        assert(function && function->parent() == focus_function);
    }

    result = to_view(result, view_rec, world_rec);

    return result;
}
