#include "visualizer_interaction.h"
#include "visualizer_render.h"
#include "visualizer_state.h"

#include <raylib.h>

int main() {
    visualizer_state_t state;

    state.initialize_layout(1600.0f, 900.0f, 0.1f);
    state.initialize_canvas();
    state.reset_camera_to_canvas();

    InitWindow((int)(state.window.right - state.window.left), (int)(state.window.bottom - state.window.top), "visualizer");

    state.editor.init();
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        update_visualizer(state, dt);
        draw_visualizer(state);
    }
    state.editor.deinit();
    CloseWindow();

    return 0;
}
