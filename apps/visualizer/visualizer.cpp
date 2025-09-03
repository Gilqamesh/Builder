#include "builder.h"
#include "editor.h"

#include "rlImGui.h"
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

struct transform_result_t {
    rec_t rec;
};

node_t* current_node = nullptr;

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
 * Transforms node from world space to view space
*/
rec_t to_view(node_t* node, rec_t view_rec, rec_t world_rec) {
    assert(node);

    rec_t result;

    if (node == current_node) {
        result.left = -current_node->coordinate_system_width() / 2.0f;
        result.right = current_node->coordinate_system_width() / 2.0f;
        result.top = -current_node->coordinate_system_height() / 2.0f;
        result.bottom = current_node->coordinate_system_height() / 2.0f;
    } else {
        result.left = node->left();
        result.right = node->right();
        result.top = node->top();
        result.bottom = node->bottom();

        while (node->parent() != current_node) {
            node = node->parent();
            assert(node);
            result.left = node->from_child_x(result.left);
            result.right = node->from_child_x(result.right);
            result.top = node->from_child_y(result.top);
            result.bottom = node->from_child_y(result.bottom);
        }
        assert(node && node->parent() == current_node);
    }

    result = to_view(result, view_rec, world_rec);

    return result;
}

node_t* created_node = nullptr;

editor_t editor;

node_t* dragged_node = nullptr;
int dragged_offset_x = 0;
int dragged_offset_y = 0;

void update_mouse_event(float dt) {
    if (editor.is_captured_mouse()) {
        return ;
    }

    const Vector2 mouse_p = GetMousePosition();
    const Vector2 mouse_delta = GetMouseDelta();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!editor.open()) {
            const int left = std::clamp(from_view_x(mouse_p.x, world, camera), -current_node->coordinate_system_width() / 2.0f, current_node->coordinate_system_width() / 2.0f);
            const int top = std::clamp(from_view_y(mouse_p.y, world, camera), -current_node->coordinate_system_height() / 2.0f, current_node->coordinate_system_height() / 2.0f);
            const int right = left;
            const int bottom = top;
            node_t* hit_node = nullptr;
            for (node_t* child : current_node->inner_nodes()) {
                if (
                    child->left() <= left &&
                    left <= child->right() &&
                    child->top() <= top &&
                    top <= child->bottom()
                ) {
                    hit_node = child;
                    break ;
                }
            }
            if (!dragged_node) {
                if (hit_node) {
                    dragged_node = hit_node;
                    dragged_offset_x = left - dragged_node->left();
                    dragged_offset_y = top - dragged_node->top();
                    assert(0 <= dragged_offset_x);
                    assert(0 <= dragged_offset_y);
                } else {
                    if (!created_node) {
                        created_node = new node_t;
                        created_node->parent(current_node);
                        created_node->left(left);
                        created_node->top(top);
                        created_node->right(right);
                        created_node->bottom(bottom);
                    } else {
                        assert(created_node);
                        if (
                            created_node->left() < created_node->right() &&
                            created_node->top() < created_node->bottom()
                        ) {
                            created_node->finalize_dimensions();
                            current_node->inner_nodes().push_back(created_node);
                            editor.create_text_editor(std::move([created_node = created_node](std::string result) {
                                created_node->name(result);
                            }));
                        } else {
                            delete created_node;
                        }
                        created_node = nullptr;
                    }
                }
            } else {
                dragged_node = nullptr;
            }
        }
    }
    assert(!(dragged_node && created_node));

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
        const float max_dimension_threshold = (float)std::numeric_limits<int16_t>::max() - (float)std::numeric_limits<int16_t>::lowest(); // should not happen, except for the root node
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

    if (dragged_node) {
        int left = std::clamp(from_view_x(mouse_p.x, world, camera) - dragged_offset_x, -current_node->coordinate_system_width() / 2.0f, current_node->coordinate_system_width() / 2.0f);
        int right = left + (dragged_node->right() - dragged_node->left());
        int top = std::clamp(from_view_y(mouse_p.y, world, camera) - dragged_offset_y, -current_node->coordinate_system_height() / 2.0f, current_node->coordinate_system_height() / 2.0f);
        int bottom = top + (dragged_node->bottom() - dragged_node->top());
        node_t* hit_nodes[32];
        int hit_nodes_count = 0;
        bool can_move = true;
        for (node_t* child : current_node->inner_nodes()) {
            if (child == dragged_node) {
                continue ;
            }
            int dx = std::min(child->right(), right) - std::max(child->left(), left);
            int dy = std::min(child->bottom(), bottom) - std::max(child->top(), top);
            if (0 <= dx && 0 <= dy) {
                if (dy < dx) {
                    if (top < child->top()) {
                        bottom = child->top() - 1;
                        top = bottom - (dragged_node->bottom() - dragged_node->top());
                    } else {
                        top = child->bottom() + 1;
                        bottom = top + (dragged_node->bottom() - dragged_node->top());
                    }
                } else {
                    if (left < child->left()) {
                        right = child->left() - 1;
                        left = right - (dragged_node->right() - dragged_node->left());
                    } else {
                        left = child->right() + 1;
                        right = left + (dragged_node->right() - dragged_node->left());
                    }
                }
                for (int i = 0; i < hit_nodes_count; ++i) {
                    node_t* hit_node = hit_nodes[i];
                    int ddx = std::min(hit_node->right(), right) - std::max(hit_node->left(), left);
                    int ddy = std::min(hit_node->bottom(), bottom) - std::max(hit_node->top(), top);
                    if (0 <= ddx && 0 <= ddy) {
                        can_move = false;
                        break ;
                    }
                }
                assert(hit_nodes_count < sizeof(hit_nodes) / sizeof(hit_nodes[0]));
                hit_nodes[hit_nodes_count++] = child;
            }
            if (!can_move) {
                break ;
            }
        }
        if (can_move) {
            dragged_node->left(left);
            dragged_node->right(right);
            dragged_node->top(top);
            dragged_node->bottom(bottom);
        }
    } else if (created_node) {
        int right = std::clamp(from_view_x(mouse_p.x, world, camera), -current_node->coordinate_system_width() / 2.0f, current_node->coordinate_system_width() / 2.0f);
        int bottom = std::clamp(from_view_y(mouse_p.y, world, camera), -current_node->coordinate_system_height() / 2.0f, current_node->coordinate_system_height() / 2.0f);
        if (
            created_node->left() < right &&
            created_node->top() < bottom
        ) {
            created_node->right(right);
            created_node->bottom(bottom);
            // sort by distance
            for (node_t* child : current_node->inner_nodes()) {
                int dx = std::min(child->right(), created_node->right()) - std::max(child->left(), created_node->left());
                int dy = std::min(child->bottom(), created_node->bottom()) - std::max(child->top(), created_node->top());
                if (0 <= dx && 0 <= dy) {
                    if (
                        child->top() <= created_node->top() &&
                        created_node->top() <= child->bottom()
                    ) {
                        created_node->right(child->left() - 1);
                    } else if (child->left() <= created_node->left()) {
                        created_node->bottom(child->top() - 1);
                    } else {
                        if (dy < dx) {
                            created_node->bottom(child->top() - 1);
                        } else {
                            created_node->right(child->left() - 1);
                        }
                    }
                }
            }
        }
    } else {
        // switch node to either parent's or child's, and adjust camera coordinates
        if (
            camera.left < -current_node->coordinate_system_width() / 2.0f ||
            current_node->coordinate_system_width() / 2.0f < camera.right ||
            camera.top < -current_node->coordinate_system_height() / 2.0f ||
            current_node->coordinate_system_height() / 2.0f < camera.bottom
        ) {
            if (current_node->parent()) {
                camera.left = current_node->from_child_x(camera.left);
                camera.right = current_node->from_child_x(camera.right);
                camera.top = current_node->from_child_y(camera.top);
                camera.bottom = current_node->from_child_y(camera.bottom);
                current_node = current_node->parent();
            }
        }
        for (node_t* child : current_node->inner_nodes()) {
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
                current_node = child;
                break ;
            }
        }
    }
}

void update(float dt) {
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

void draw_node(node_t* node, rec_t view_rec, rec_t world_rec) {
    rec_t node_rec = to_view(node, view_rec, world_rec);
    
    rec_t clipped_rec = node_rec;
    clipped_rec.left = std::clamp(node_rec.left, view_rec.left, view_rec.right);
    clipped_rec.right = std::clamp(node_rec.right, view_rec.left, view_rec.right);
    clipped_rec.top = std::clamp(node_rec.top, view_rec.top, view_rec.bottom);
    clipped_rec.bottom = std::clamp(node_rec.bottom, view_rec.top, view_rec.bottom);

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

    if (node != current_node) {
        const char* node_name = node->name().c_str();
        fitted_text_t text_fit = fit_text_wrapped(font, node_name, node_rec, 1.0f);

        Vector2 size = MeasureTextEx(font, text_fit.wrapped.c_str(), text_fit.font_size, text_fit.spacing);
        Vector2 pos = { node_rec.left + (node_rec.right - node_rec.left - size.x) / 2,
                        node_rec.top + (node_rec.bottom - node_rec.top - size.y) / 2 };

        DrawTextEx(font, text_fit.wrapped.c_str(), pos, text_fit.font_size, text_fit.spacing, Fade(BLACK, fade));
    }
}

void draw_nodes(node_t* node, rec_t view_rec, rec_t world_rec) {
    for (node_t* child : node->inner_nodes()) {
        draw_nodes(child, view_rec, world_rec);
    }
    draw_node(node, view_rec, world_rec);
}

void draw_overlay() {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s", current_node->name().c_str());
    Vector2 text_dims = MeasureTextEx(GetFontDefault(), buffer, 20.0f, 1.0f);
    DrawText(buffer, (int)((overlay.right - overlay.left) / 2.0f - text_dims.x / 2.0f), (int)(overlay.top + 10.0f), 20, WHITE);
}

void draw_world() {
    if (created_node != nullptr) {
        draw_node(created_node, world, camera);
    }
    draw_nodes(current_node, world, camera);
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
    compiler_t compiler;
    compiler.register_node<if_node_t>("if");
    compiler.register_node<sub_t>("sub");
    compiler.register_node<mul_node_t>("mul");
    compiler.register_node<is_zero_t>("is_zero");
    compiler.register_node<int_node_t>("int");
    compiler.register_node<logger_node_t>("logger");
    compiler.register_node<pin_node_t>("pin");

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

    node_t canvas_node;
    current_node = &canvas_node;
    canvas_node.left(-10000);
    canvas_node.right(10000);
    canvas_node.top(-10000);
    canvas_node.bottom(10000);
    canvas_node.finalize_dimensions();

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
