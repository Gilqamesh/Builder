#include <function_visualizer_editor.h>
#include <function_repository.h>
#include <function_alu.h>
#include <function_compound.h>
#include <function_ir_file_repository.h>
#include <rlImGui.h>
#include <raylib.h>

struct rec_t {
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
};

rec_t camera; // in world space
rec_t window; // in view space
rec_t world; // in view space
rec_t overlay; // in view space

function_t* current_function = nullptr;

function_repository_t function_repository;
function_ir_file_repository_t function_ir_file_repository("functions");
typesystem_t typesystem;

/**
 * Transforms x world space coordinate to view space
*/
float to_view_x(float x, rec_t view_rec, rec_t world_rec) {
    const float result = (x - world_rec.left) * (view_rec.right - view_rec.left) / (world_rec.right - world_rec.left) + view_rec.left;
    return result;
}

/**
 * Transforms y world space coordinate to view space
*/
float to_view_y(float y, rec_t view_rec, rec_t world_rec) {
    const float result = (y - world_rec.top) * (view_rec.bottom - view_rec.top) / (world_rec.bottom - world_rec.top) + view_rec.top;
    return result;
}

/**
 * Transforms x view space coordinate to world space
*/
float from_view_x(float x, rec_t view_rec, rec_t world_rec) {
    const float result = (x - view_rec.left) * (world_rec.right - world_rec.left) / (view_rec.right - view_rec.left) + world_rec.left;
    return result;
}

/**
 * Transforms y view space coordinate to world space
*/
float from_view_y(float y, rec_t view_rec, rec_t world_rec) {
    const float result = (y - view_rec.top) * (world_rec.bottom - world_rec.top) / (view_rec.bottom - view_rec.top) + world_rec.top;
    return result;
}

/**
 * Transforms rectangle from world space to view space
*/
rec_t to_view(rec_t rec, rec_t view_rec, rec_t world_rec) {
    rec.left = to_view_x(rec.left, view_rec, world_rec);
    rec.right = to_view_x(rec.right, view_rec, world_rec);
    rec.top = to_view_y(rec.top, view_rec, world_rec);
    rec.bottom = to_view_y(rec.bottom, view_rec, world_rec);

    return rec;
}

/**
 * Transforms function from world space to view space
*/
rec_t to_view(function_t* function, rec_t view_rec, rec_t world_rec) {
    assert(function);

    rec_t result;

    if (function == current_function) {
        result.left = -current_function->coordinate_system_width() / 2.0f;
        result.right = current_function->coordinate_system_width() / 2.0f;
        result.top = -current_function->coordinate_system_height() / 2.0f;
        result.bottom = current_function->coordinate_system_height() / 2.0f;
    } else {
        result.left = function->left();
        result.right = function->right();
        result.top = function->top();
        result.bottom = function->bottom();

        while (function->parent() != current_function) {
            function = function->parent();
            assert(function);
            result.left = function->from_child_x(result.left);
            result.right = function->from_child_x(result.right);
            result.top = function->from_child_y(result.top);
            result.bottom = function->from_child_y(result.bottom);
        }
        assert(function && function->parent() == current_function);
    }

    result = to_view(result, view_rec, world_rec);

    return result;
}

function_t* created_function = nullptr;

function_visualizer_editor_t editor;

function_t* dragged_function = nullptr;
int dragged_offset_x = 0;
int dragged_offset_y = 0;

void update_key_event(float dt) {
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_S)) {
        assert(current_function);
        function_t* function_to_save = current_function;
        while (function_to_save->parent()) {
            function_to_save = function_to_save->parent();
        }
        assert(function_to_save);
        function_repository.save(function_to_save->function_ir(), function_to_save->function_call());
        function_ir_file_repository.save(function_to_save->function_ir());
    }
}

void update_mouse_event(float dt) {
    if (editor.is_captured_mouse()) {
        return ;
    }

    const Vector2 mouse_p = GetMousePosition();
    const Vector2 mouse_delta = GetMouseDelta();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!editor.open()) {
            const int left = std::clamp(from_view_x(mouse_p.x, world, camera), -current_function->coordinate_system_width() / 2.0f, current_function->coordinate_system_width() / 2.0f);
            const int top = std::clamp(from_view_y(mouse_p.y, world, camera), -current_function->coordinate_system_height() / 2.0f, current_function->coordinate_system_height() / 2.0f);
            const int right = left;
            const int bottom = top;
            function_t* hit_function = nullptr;
            for (function_t* child : current_function->children()) {
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
            if (!dragged_function) {
                if (hit_function) {
                    dragged_function = hit_function;
                    dragged_offset_x = left - dragged_function->left();
                    dragged_offset_y = top - dragged_function->top();
                    assert(0 <= dragged_offset_x);
                    assert(0 <= dragged_offset_y);
                } else {
                    if (!created_function) {
                        created_function = function_compound_t::function(typesystem, function_ir_t {});
                        created_function->parent(current_function);
                        created_function->left(left);
                        created_function->top(top);
                        created_function->right(right);
                        created_function->bottom(bottom);
                    } else {
                        assert(created_function);
                        if (
                            created_function->left() < created_function->right() &&
                            created_function->top() < created_function->bottom()
                        ) {
                            created_function->finalize_dimensions();
                            current_function->children().push_back(created_function);
                            editor.create_text_editor(std::move([created_function = created_function](std::string result) {
                                created_function->function_ir().function_id.ns = "default";
                                created_function->function_ir().function_id.name = result;
                                created_function->function_ir().function_id.creation_time = std::chrono::system_clock::now();
                            }));
                        } else {
                            delete created_function;
                        }
                        created_function = nullptr;
                    }
                }
            } else {
                dragged_function = nullptr;
            }
        }
    }
    assert(!(dragged_function && created_function));

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const float camera_width = camera.right - camera.left;
        const float camera_height = camera.bottom - camera.top;
        const float drag_x = mouse_delta.x / (world.right - world.left) * (camera.right - camera.left);
        const float drag_y = mouse_delta.y / (world.bottom - world.top) * (camera.bottom - camera.top);
        camera.left -= drag_x;
        camera.right -= drag_x;
        camera.top -= drag_y;
        camera.bottom -= drag_y;
    }

    float mouse_wheel = GetMouseWheelMove();
    if (mouse_wheel != 0.0f) {
        const float factor = mouse_wheel < 0.0f ? 1.1f : 0.9f;
        const float middle_x = (camera.left + camera.right) / 2.0f;
        const float middle_y = (camera.top + camera.bottom) / 2.0f;
        const float new_left = middle_x - (middle_x - camera.left) * factor;
        const float new_right = middle_x + (camera.right - middle_x) * factor;
        const float new_top = middle_y - (middle_y - camera.top) * factor;
        const float new_bottom = middle_y + (camera.bottom - middle_y) * factor;
        const float min_dimension_threshold = 1.0f;
        const float max_dimension_threshold = (float)std::numeric_limits<int16_t>::max() - (float)std::numeric_limits<int16_t>::lowest(); // should not happen, except for the root function
        if (
            min_dimension_threshold <= new_right - new_left &&
            min_dimension_threshold <= new_bottom - new_top &&
            new_right - new_left <= max_dimension_threshold &&
            new_bottom - new_top <= max_dimension_threshold
        ) {
            camera.left = new_left;
            camera.right = new_right;
            camera.top = new_top;
            camera.bottom = new_bottom;
        }
    }

    if (dragged_function) {
        int left = std::clamp(from_view_x(mouse_p.x, world, camera) - dragged_offset_x, -current_function->coordinate_system_width() / 2.0f, current_function->coordinate_system_width() / 2.0f);
        int right = left + (dragged_function->right() - dragged_function->left());
        int top = std::clamp(from_view_y(mouse_p.y, world, camera) - dragged_offset_y, -current_function->coordinate_system_height() / 2.0f, current_function->coordinate_system_height() / 2.0f);
        int bottom = top + (dragged_function->bottom() - dragged_function->top());
        function_t* hit_functions[32];
        int hit_functions_count = 0;
        bool can_move = true;
        for (function_t* child : current_function->children()) {
            if (child == dragged_function) {
                continue ;
            }
            int dx = std::min(child->right(), right) - std::max(child->left(), left);
            int dy = std::min(child->bottom(), bottom) - std::max(child->top(), top);
            if (0 <= dx && 0 <= dy) {
                if (dy < dx) {
                    if (top < child->top()) {
                        bottom = child->top() - 1;
                        top = bottom - (dragged_function->bottom() - dragged_function->top());
                    } else {
                        top = child->bottom() + 1;
                        bottom = top + (dragged_function->bottom() - dragged_function->top());
                    }
                } else {
                    if (left < child->left()) {
                        right = child->left() - 1;
                        left = right - (dragged_function->right() - dragged_function->left());
                    } else {
                        left = child->right() + 1;
                        right = left + (dragged_function->right() - dragged_function->left());
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
                assert(hit_functions_count < sizeof(hit_functions) / sizeof(hit_functions[0]));
                hit_functions[hit_functions_count++] = child;
            }
            if (!can_move) {
                break ;
            }
        }
        if (can_move) {
            dragged_function->left(left);
            dragged_function->right(right);
            dragged_function->top(top);
            dragged_function->bottom(bottom);
        }
    } else if (created_function) {
        int right = std::clamp(from_view_x(mouse_p.x, world, camera), -current_function->coordinate_system_width() / 2.0f, current_function->coordinate_system_width() / 2.0f);
        int bottom = std::clamp(from_view_y(mouse_p.y, world, camera), -current_function->coordinate_system_height() / 2.0f, current_function->coordinate_system_height() / 2.0f);
        if (
            created_function->left() < right &&
            created_function->top() < bottom
        ) {
            created_function->right(right);
            created_function->bottom(bottom);
            // sort by distance
            for (function_t* child : current_function->children()) {
                int dx = std::min(child->right(), created_function->right()) - std::max(child->left(), created_function->left());
                int dy = std::min(child->bottom(), created_function->bottom()) - std::max(child->top(), created_function->top());
                if (0 <= dx && 0 <= dy) {
                    if (
                        child->top() <= created_function->top() &&
                        created_function->top() <= child->bottom()
                    ) {
                        created_function->right(child->left() - 1);
                    } else if (child->left() <= created_function->left()) {
                        created_function->bottom(child->top() - 1);
                    } else {
                        if (dy < dx) {
                            created_function->bottom(child->top() - 1);
                        } else {
                            created_function->right(child->left() - 1);
                        }
                    }
                }
            }
        }
    } else {
        // switch function to either parent's or child's, and adjust camera coordinates
        if (
            camera.left < -current_function->coordinate_system_width() / 2.0f ||
            current_function->coordinate_system_width() / 2.0f < camera.right ||
            camera.top < -current_function->coordinate_system_height() / 2.0f ||
            current_function->coordinate_system_height() / 2.0f < camera.bottom
        ) {
            if (current_function->parent()) {
                camera.left = current_function->from_child_x(camera.left);
                camera.right = current_function->from_child_x(camera.right);
                camera.top = current_function->from_child_y(camera.top);
                camera.bottom = current_function->from_child_y(camera.bottom);
                current_function = current_function->parent();
            }
        }
        for (function_t* child : current_function->children()) {
            if (
                child->left() <= camera.left &&
                camera.right <= child->right() &&
                child->top() <= camera.top &&
                camera.bottom <= child->bottom()
            ) {
                camera.left = child->to_child_x(camera.left);
                camera.right = child->to_child_x(camera.right);
                camera.top = child->to_child_y(camera.top);
                camera.bottom = child->to_child_y(camera.bottom);
                current_function = child;
                break ;
            }
        }
    }
}

void update(float dt) {
    update_key_event(dt);
    update_mouse_event(dt);
}

struct fitted_text_t {
    std::string wrapped;
    float font_size;
    float spacing;
};

std::string wrap_text_to_width(Font font, const std::string& text, float font_size, float spacing, float max_width) {
    std::istringstream iss(text);
    std::string word;
    std::string current_line;
    std::string result;

    while (iss >> word) {
        std::string trial = current_line.empty() ? word : current_line + " " + word;
        Vector2 size = MeasureTextEx(font, trial.c_str(), font_size, spacing);
        if (size.x > max_width && !current_line.empty()) {
            result += current_line + "\n";
            current_line = word;
        } else {
            current_line = trial;
        }
    }

    if (!current_line.empty()) {
        result += current_line;
    }

    return result;
}

fitted_text_t fit_text_wrapped(Font font, const std::string& text, rec_t rec, float spacing) {
    const float test_font_size = 10.0f;
    Vector2 char_size = MeasureTextEx(font, "M", test_font_size, spacing);

    fitted_text_t best_fit = { text, test_font_size, spacing };
    float best_font_size = 0.0f;

    float low = 2.0f;
    float high = rec.bottom - rec.top;
    for (int i = 0; i < 16; ++i) {
        float mid = (low + high) * 0.5f;
        std::string wrapped = wrap_text_to_width(font, text, mid, spacing, rec.right - rec.left);
        Vector2 dims = MeasureTextEx(font, wrapped.c_str(), mid, spacing);
        int num_lines = std::count(wrapped.begin(), wrapped.end(), '\n') + 1;
        float total_height = num_lines * char_size.y * (mid / test_font_size);

        if (dims.x <= rec.right - rec.left && total_height <= rec.bottom - rec.top) {
            best_font_size = mid;
            best_fit = { wrapped, mid, spacing };
            low = mid;
        } else {
            high = mid;
        }
    }

    return best_fit;
}

Font font;

void draw_function(function_t* function, rec_t view_rec, rec_t world_rec) {
    rec_t function_rec = to_view(function, view_rec, world_rec);
    
    rec_t clipped_rec = function_rec;
    clipped_rec.left = std::clamp(function_rec.left, view_rec.left, view_rec.right);
    clipped_rec.right = std::clamp(function_rec.right, view_rec.left, view_rec.right);
    clipped_rec.top = std::clamp(function_rec.top, view_rec.top, view_rec.bottom);
    clipped_rec.bottom = std::clamp(function_rec.bottom, view_rec.top, view_rec.bottom);

    if (
        clipped_rec.right <= clipped_rec.left ||
        clipped_rec.bottom <= clipped_rec.top
    ) {
        return ;
    }

    const double rec_area = ((double)clipped_rec.right - (double)clipped_rec.left) * ((double)clipped_rec.bottom - (double)clipped_rec.top);
    const double view_area = (double)(view_rec.right - view_rec.left) * (double)(view_rec.bottom - view_rec.top);
    const double area_ratio = rec_area / view_area;
    const float fade = std::clamp((float)(1.0 - area_ratio), 0.3f, 1.0f);

    DrawRectangleRec(
        {
            clipped_rec.left,
            clipped_rec.top,
            (clipped_rec.right - clipped_rec.left),
            (clipped_rec.bottom - clipped_rec.top)
        },
        Fade(BLUE, fade)
    );

    if (function != current_function) {
        const char* function_name = function->function_ir().function_id.name.c_str();
        fitted_text_t text_fit = fit_text_wrapped(font, function_name, function_rec, 1.0f);

        Vector2 size = MeasureTextEx(font, text_fit.wrapped.c_str(), text_fit.font_size, text_fit.spacing);
        Vector2 pos = { function_rec.left + (function_rec.right - function_rec.left - size.x) / 2,
                        function_rec.top + (function_rec.bottom - function_rec.top - size.y) / 2 };

        DrawTextEx(font, text_fit.wrapped.c_str(), pos, text_fit.font_size, text_fit.spacing, Fade(BLACK, fade));
    }
}

void draw_functions(function_t* function, rec_t view_rec, rec_t world_rec) {
    for (function_t* child : function->children()) {
        draw_functions(child, view_rec, world_rec);
    }
    draw_function(function, view_rec, world_rec);
}

void draw_overlay() {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s", current_function->function_ir().function_id.name.c_str());
    Vector2 text_dims = MeasureTextEx(GetFontDefault(), buffer, 20.0f, 1.0f);
    DrawText(buffer, (int)((overlay.right - overlay.left) / 2.0f - text_dims.x / 2.0f), (int)(overlay.top + 10.0f), 20, WHITE);
}

void draw_world() {
    if (created_function != nullptr) {
        draw_function(created_function, world, camera);
    }
    draw_functions(current_function, world, camera);
}

void draw() {
    BeginDrawing();
    rlImGuiBegin();

    BeginScissorMode((int)overlay.left, (int)overlay.top, (int)(overlay.right - overlay.left), (int)(overlay.bottom - overlay.top));
    ClearBackground(BLACK);
    draw_overlay();
    EndScissorMode();

    BeginScissorMode((int)world.left, (int)world.top, (int)(world.right - world.left), (int)(world.bottom - world.top));
    ClearBackground(WHITE);
    draw_world();
    EndScissorMode();

    editor.draw();

    rlImGuiEnd();
    EndDrawing();
}

int main() {
    window.left = 0.0f;
    window.top = 0.0f;
    window.right = 1600.0f;
    window.bottom = 900.0f;

    const float overlay_to_world_height_ratio = 0.1f;
    overlay.left = window.left;
    overlay.right = window.right;
    overlay.top = window.top;
    overlay.bottom = window.top + (window.bottom - window.top) * overlay_to_world_height_ratio;

    world.left = window.left;
    world.right = window.right;
    world.top = overlay.bottom;
    world.bottom = window.bottom;

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

    camera.left = std::numeric_limits<int16_t>::lowest();
    camera.right = std::numeric_limits<int16_t>::max();
    camera.top = std::numeric_limits<int16_t>::lowest();
    camera.bottom = std::numeric_limits<int16_t>::max();

    InitWindow((int)(window.right - window.left), (int)(window.bottom - window.top), "visualizer");

    font = GetFontDefault();

    editor.init();
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        update(dt);
        draw();
    }
    editor.deinit();
    CloseWindow();

    return 0;
}
