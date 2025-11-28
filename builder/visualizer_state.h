#ifndef VISUALIZER_STATE_H
#define VISUALIZER_STATE_H

#include "function_compound.h"
#include "function_ir_file_repository.h"
#include "function_repository.h"
#include "typesystem.h"
#include "visualizer_editor.h"
#include "visualizer_space.h"

#include <raylib.h>

struct visualizer_state_t {
    rec_t camera;
    rec_t window;
    rec_t world;
    rec_t overlay;

    function_t* current_function;
    function_t* created_function;
    function_t* dragged_function;
    int dragged_offset_x;
    int dragged_offset_y;

    visualizer_editor_t editor;
    function_repository_t function_repository;
    function_ir_file_repository_t function_ir_file_repository;
    typesystem_t typesystem;
    Font font;

    visualizer_state_t();

    void initialize_layout(float width, float height, float overlay_to_world_height_ratio);
    void initialize_canvas();
    void reset_camera_to_canvas();
};

#endif // VISUALIZER_STATE_H
