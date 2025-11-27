#include "visualizer_state.h"

visualizer_state_t::visualizer_state():
    current_function(nullptr),
    created_function(nullptr),
    dragged_function(nullptr),
    dragged_offset_x(0),
    dragged_offset_y(0),
    function_ir_file_repository("functions"),
    font(GetFontDefault())
{
}

void visualizer_state_t::initialize_layout(float width, float height, float overlay_to_world_height_ratio) {
    window.left = 0.0f;
    window.top = 0.0f;
    window.right = width;
    window.bottom = height;

    overlay.left = window.left;
    overlay.right = window.right;
    overlay.top = window.top;
    overlay.bottom = window.top + (window.bottom - window.top) * overlay_to_world_height_ratio;

    world.left = window.left;
    world.right = window.right;
    world.top = overlay.bottom;
    world.bottom = window.bottom;
}

void visualizer_state_t::initialize_canvas() {
    function_t* canvas_function = function_compound_t::function(typesystem, function_ir_t {
        .function_id = function_id_t {
            .ns = "default",
            .name = "defined_function",
            .creation_time = std::chrono::system_clock::now()
        },
    });
    current_function = canvas_function;
    canvas_function->left(-10000);
    canvas_function->right(10000);
    canvas_function->top(-10000);
    canvas_function->bottom(10000);
    canvas_function->finalize_dimensions();
}

void visualizer_state_t::reset_camera_to_canvas() {
    camera.left = std::numeric_limits<int16_t>::lowest();
    camera.right = std::numeric_limits<int16_t>::max();
    camera.top = std::numeric_limits<int16_t>::lowest();
    camera.bottom = std::numeric_limits<int16_t>::max();
}
