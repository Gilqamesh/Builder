#include <visualizer_interaction.h>
#include <visualizer_space.h>
#include <raylib.h>

namespace {

void update_key_event(visualizer_state_t& state, float /*dt*/) {
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_S)) {
        assert(state.current_function);
        function_t* function_to_save = state.current_function;
        while (function_to_save->parent()) {
            function_to_save = function_to_save->parent();
        }
        assert(function_to_save);
        state.function_repository.save(function_to_save->function_ir(), function_to_save->function_call());
        state.function_ir_file_repository.save(function_to_save->function_ir());
    }
}

void update_mouse_event(visualizer_state_t& state, float /*dt*/) {
    if (state.editor.is_captured_mouse()) {
        return ;
    }

    const Vector2 mouse_p = GetMousePosition();
    const Vector2 mouse_delta = GetMouseDelta();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!state.editor.open()) {
            const int left = std::clamp(from_view_x(mouse_p.x, state.world, state.camera), -state.current_function->coordinate_system_width() / 2.0f, state.current_function->coordinate_system_width() / 2.0f);
            const int top = std::clamp(from_view_y(mouse_p.y, state.world, state.camera), -state.current_function->coordinate_system_height() / 2.0f, state.current_function->coordinate_system_height() / 2.0f);
            const int right = left;
            const int bottom = top;
            function_t* hit_function = nullptr;
            for (function_t* child : state.current_function->children()) {
                if (
                    child->left() <= left &&
                    left <= child->right() &&
                    child->top() <= top &&
                    top <= child->bottom()
                ) {
                    hit_function = child;
                    break ;
                }
            }
            if (!state.dragged_function) {
                if (hit_function) {
                    state.dragged_function = hit_function;
                    state.dragged_offset_x = left - state.dragged_function->left();
                    state.dragged_offset_y = top - state.dragged_function->top();
                    assert(0 <= state.dragged_offset_x);
                    assert(0 <= state.dragged_offset_y);
                } else {
                    if (!state.created_function) {
                        state.created_function = function_compound_t::function(state.typesystem, function_ir_t {});
                        state.created_function->parent(state.current_function);
                        state.created_function->left(left);
                        state.created_function->top(top);
                        state.created_function->right(right);
                        state.created_function->bottom(bottom);
                    } else {
                        assert(state.created_function);
                        if (
                            state.created_function->left() < state.created_function->right() &&
                            state.created_function->top() < state.created_function->bottom()
                        ) {
                            state.created_function->finalize_dimensions();
                            state.current_function->children().push_back(state.created_function);
                            state.editor.create_text_editor(std::move([created_function = state.created_function](std::string result) {
                                created_function->function_ir().function_id.ns = "default";
                                created_function->function_ir().function_id.name = result;
                                created_function->function_ir().function_id.creation_time = std::chrono::system_clock::now();
                            }));
                        } else {
                            delete state.created_function;
                        }
                        state.created_function = nullptr;
                    }
                }
            } else {
                state.dragged_function = nullptr;
            }
        }
    }
    assert(!(state.dragged_function && state.created_function));

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const float camera_width = state.camera.right - state.camera.left;
        const float camera_height = state.camera.bottom - state.camera.top;
        const float drag_x = mouse_delta.x / (state.world.right - state.world.left) * (state.camera.right - state.camera.left);
        const float drag_y = mouse_delta.y / (state.world.bottom - state.world.top) * (state.camera.bottom - state.camera.top);
        state.camera.left -= drag_x;
        state.camera.right -= drag_x;
        state.camera.top -= drag_y;
        state.camera.bottom -= drag_y;
    }

    float mouse_wheel = GetMouseWheelMove();
    if (mouse_wheel != 0.0f) {
        const float factor = mouse_wheel < 0.0f ? 1.1f : 0.9f;
        const float middle_x = (state.camera.left + state.camera.right) / 2.0f;
        const float middle_y = (state.camera.top + state.camera.bottom) / 2.0f;
        const float new_left = middle_x - (middle_x - state.camera.left) * factor;
        const float new_right = middle_x + (state.camera.right - middle_x) * factor;
        const float new_top = middle_y - (middle_y - state.camera.top) * factor;
        const float new_bottom = middle_y + (state.camera.bottom - middle_y) * factor;
        const float min_dimension_threshold = 1.0f;
        const float max_dimension_threshold = (float)std::numeric_limits<int16_t>::max() - (float)std::numeric_limits<int16_t>::lowest();
        if (
            min_dimension_threshold <= new_right - new_left &&
            min_dimension_threshold <= new_bottom - new_top &&
            new_right - new_left <= max_dimension_threshold &&
            new_bottom - new_top <= max_dimension_threshold
        ) {
            state.camera.left = new_left;
            state.camera.right = new_right;
            state.camera.top = new_top;
            state.camera.bottom = new_bottom;
        }
    }

    if (state.dragged_function) {
        int left = std::clamp(from_view_x(mouse_p.x, state.world, state.camera) - state.dragged_offset_x, -state.current_function->coordinate_system_width() / 2.0f, state.current_function->coordinate_system_width() / 2.0f);
        int right = left + (state.dragged_function->right() - state.dragged_function->left());
        int top = std::clamp(from_view_y(mouse_p.y, state.world, state.camera) - state.dragged_offset_y, -state.current_function->coordinate_system_height() / 2.0f, state.current_function->coordinate_system_height() / 2.0f);
        int bottom = top + (state.dragged_function->bottom() - state.dragged_function->top());
        function_t* hit_functions[32];
        int hit_functions_count = 0;
        bool can_move = true;
        for (function_t* child : state.current_function->children()) {
            if (child == state.dragged_function) {
                continue ;
            }
            int dx = std::min(child->right(), right) - std::max(child->left(), left);
            int dy = std::min(child->bottom(), bottom) - std::max(child->top(), top);
            if (0 <= dx && 0 <= dy) {
                if (dy < dx) {
                    if (top < child->top()) {
                        bottom = child->top() - 1;
                        top = bottom - (state.dragged_function->bottom() - state.dragged_function->top());
                    } else {
                        top = child->bottom() + 1;
                        bottom = top + (state.dragged_function->bottom() - state.dragged_function->top());
                    }
                } else {
                    if (left < child->left()) {
                        right = child->left() - 1;
                        left = right - (state.dragged_function->right() - state.dragged_function->left());
                    } else {
                        left = child->right() + 1;
                        right = left + (state.dragged_function->right() - state.dragged_function->left());
                    }
                }
                for (int i = 0; i < hit_functions_count; ++i) {
                    function_t* hit_function = hit_functions[i];
                    int ddx = std::min(hit_function->right(), right) - std::max(hit_function->left(), left);
                    int ddy = std::min(hit_function->bottom(), bottom) - std::max(hit_function->top(), top);
                    if (0 <= ddx && 0 <= ddy) {
                        can_move = false;
                        break ;
                    }
                }
                assert(hit_functions_count < (int)(sizeof(hit_functions) / sizeof(hit_functions[0])));
                hit_functions[hit_functions_count++] = child;
            }
            if (!can_move) {
                break ;
            }
        }
        if (can_move) {
            state.dragged_function->left(left);
            state.dragged_function->right(right);
            state.dragged_function->top(top);
            state.dragged_function->bottom(bottom);
        }
    } else if (state.created_function) {
        int right = std::clamp(from_view_x(mouse_p.x, state.world, state.camera), -state.current_function->coordinate_system_width() / 2.0f, state.current_function->coordinate_system_width() / 2.0f);
        int bottom = std::clamp(from_view_y(mouse_p.y, state.world, state.camera), -state.current_function->coordinate_system_height() / 2.0f, state.current_function->coordinate_system_height() / 2.0f);
        if (
            state.created_function->left() < right &&
            state.created_function->top() < bottom
        ) {
            state.created_function->right(right);
            state.created_function->bottom(bottom);
            for (function_t* child : state.current_function->children()) {
                int dx = std::min(child->right(), state.created_function->right()) - std::max(child->left(), state.created_function->left());
                int dy = std::min(child->bottom(), state.created_function->bottom()) - std::max(child->top(), state.created_function->top());
                if (0 <= dx && 0 <= dy) {
                    if (
                        child->top() <= state.created_function->top() &&
                        state.created_function->top() <= child->bottom()
                    ) {
                        state.created_function->right(child->left() - 1);
                    } else if (child->left() <= state.created_function->left()) {
                        state.created_function->bottom(child->top() - 1);
                    } else {
                        if (dy < dx) {
                            state.created_function->bottom(child->top() - 1);
                        } else {
                            state.created_function->right(child->left() - 1);
                        }
                    }
                }
            }
        }
    } else {
        if (
            state.camera.left < -state.current_function->coordinate_system_width() / 2.0f ||
            state.current_function->coordinate_system_width() / 2.0f < state.camera.right ||
            state.camera.top < -state.current_function->coordinate_system_height() / 2.0f ||
            state.current_function->coordinate_system_height() / 2.0f < state.camera.bottom
        ) {
            if (state.current_function->parent()) {
                state.camera.left = state.current_function->from_child_x(state.camera.left);
                state.camera.right = state.current_function->from_child_x(state.camera.right);
                state.camera.top = state.current_function->from_child_y(state.camera.top);
                state.camera.bottom = state.current_function->from_child_y(state.camera.bottom);
                state.current_function = state.current_function->parent();
            }
        }
        for (function_t* child : state.current_function->children()) {
            if (
                child->left() <= state.camera.left &&
                state.camera.right <= child->right() &&
                child->top() <= state.camera.top &&
                state.camera.bottom <= child->bottom()
            ) {
                state.camera.left = child->to_child_x(state.camera.left);
                state.camera.right = child->to_child_x(state.camera.right);
                state.camera.top = child->to_child_y(state.camera.top);
                state.camera.bottom = child->to_child_y(state.camera.bottom);
                state.current_function = child;
                break ;
            }
        }
    }
}

} // namespace

void update_visualizer(visualizer_state_t& state, float dt) {
    update_key_event(state, dt);
    update_mouse_event(state, dt);
}
