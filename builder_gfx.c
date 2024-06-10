#include "builder_gfx.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "raylib.h"
#include "math.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))
#define ARRAY_ENSURE_TOP(array_p, array_top, array_size) do { \
    if ((array_top) >= (array_size)) { \
        if ((array_size) == 0) { \
            (array_size) = 8; \
            (array_p) = malloc((array_size) * sizeof(*(array_p))); \
        } else { \
            (array_size) <<= 1; \
            (array_p) = realloc((array_p), (array_size) * sizeof(*(array_p))); \
            } \
    } \
} while (0)

static float Vector2_Dot(Vector2 v, Vector2 w) { return v.x * w.x + v.y * w.y; }
static float Vector2_Len(Vector2 v) { return sqrtf(Vector2_Dot(v, v)); }
static Vector2 Vector2_Normalize(Vector2 v) { float len = Vector2_Len(v); v.x /= len; v.y /= len; return v; }
static Vector2 Vector2_Scale(Vector2 v, float f) { v.x *= f; v.y *= f; return v; }
static Vector2 Vector2_Add(Vector2 v, Vector2 w) { v.x += w.x; v.y += w.y; return v; }
static Vector2 Vector2_Sub(Vector2 v, Vector2 w) { v.x -= w.x; v.y -= w.y; return v; }

static int Rec_IsInside(Rectangle rec, Vector2 p) { return p.x >= rec.x && p.x <= rec.x + rec.width && p.y >= rec.y && p.y <= rec.y + rec.height; }

typedef struct obj_present {
    struct obj base;

    int width;
    int height;
    float zoom;
    float min_zoom;
    float max_zoom;
    float movement_speed;
    Camera2D camera;
    Font font;
    double time_cur;
    obj_t drawn_node;

    Vector2 average_node_dims;
    Vector2 running_average_node_dims;
    size_t  running_average_node_dims_top;
} *obj_present_t;

static void obj__present_update(obj_t self, double dt);
static void obj__present_update_node_as_drawn(obj_t self, obj_t node);
static void obj__present_update_reset_camera(obj_t self);

static void      obj__present_draw(obj_t self);
static Rectangle obj__present_draw_node(obj_t self, obj_t node, Vector2 top_left_p);
static void      obj__present_draw_wire(obj_t self, Vector2 from, Vector2 to);
static void      obj__present_draw_scene(obj_t self);
static void      obj__present_draw_overlay(obj_t self);

static void obj__run_present(obj_t self);
static void obj__describe_short_present(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_present(obj_t self, char* buffer, int buffer_size);
static void obj__destroy_present(obj_t self);

static Rectangle obj__present_draw_node(obj_t self, obj_t node, Vector2 top_left_p) {
    obj_present_t present = (obj_present_t) self;

    static char buffer[512];
    obj__describe_long(node, buffer, sizeof(buffer));

    const int font_size = 10;
    Vector2 text_dims = MeasureTextEx(present->font, buffer, font_size, 1.0f);

    Vector2 margin = {
        .x = 10.0f,
        .y = 5.0f
    };
    Rectangle node_rec = {
        .x = top_left_p.x,
        .y = top_left_p.y,
        .width = text_dims.x + margin.x,
        .height = text_dims.y + margin.y
    };
    
    Vector2 text_p = {
        .x = top_left_p.x + (node_rec.width - text_dims.x) / 2,
        .y = top_left_p.y + (node_rec.height - text_dims.y) / 2
    };

    Color node_color = YELLOW;
    Vector2 mp_screen = GetMousePosition();
    Vector2 mp_world = GetScreenToWorld2D(mp_screen, present->camera);
    if (Rec_IsInside(node_rec, mp_world)) {
        node_color = RED;
        DrawRectangleLinesEx(node_rec, 1.0f, RED);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            obj__present_update_node_as_drawn(self, node);
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            obj__run(node);
        }
    }

    const double max_blink_periodicity = 0.15;

    const double time_since_last_successful_run = builder__get_time_stamp() - node->time_start;
    if (node->is_running || time_since_last_successful_run <= max_blink_periodicity) {
        node_color = PURPLE;
    }

    DrawRectangleLinesEx(node_rec, 1.0f, node_color);
    DrawTextEx(present->font, buffer, text_p, font_size, 1.0f, WHITE);

    present->running_average_node_dims.x = (present->running_average_node_dims.x * present->running_average_node_dims_top + node_rec.width) / (present->running_average_node_dims_top + 1);
    present->running_average_node_dims.y = (present->running_average_node_dims.y * present->running_average_node_dims_top + node_rec.height) / (present->running_average_node_dims_top + 1);
    ++present->running_average_node_dims_top;

    return node_rec;
}

static void obj__present_update_node_as_drawn(obj_t self, obj_t node) {
    obj_present_t present = (obj_present_t) self;

    present->drawn_node = node;
    obj__present_update_reset_camera(self);
    if (node) {
        if (present->drawn_node->outputs_top == 0 && present->drawn_node->inputs_top > 0) {
            present->camera.offset.y = present->height / 6.0f;
        } else if (present->drawn_node->outputs_top > 0 && present->drawn_node->inputs == 0) {
            present->camera.offset.y = present->height * 4.0f / 6.0f;
        }
    }
}

static void obj__present_update_reset_camera(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    memset(&present->camera, 0, sizeof(present->camera));

    present->camera.zoom = 2.0f;
    present->camera.offset.x = present->width / 2.0f;
    present->camera.offset.y = present->height / 2.0f;
}

static void obj__present_update(obj_t self, double dt) {
    obj_present_t present = (obj_present_t) self;

    present->average_node_dims = present->running_average_node_dims;
    present->running_average_node_dims = (Vector2){ 0 };
    present->running_average_node_dims_top = 0;

    float mouse_wheel = GetMouseWheelMove();
    if (mouse_wheel > 0.0f) {
        present->camera.zoom = 1.15f * present->camera.zoom;
    }
    else if (mouse_wheel < 0.0f) {
        present->camera.zoom = 0.9f * present->camera.zoom;
    }

    if (present->camera.zoom < present->min_zoom) {
        present->camera.zoom = present->min_zoom;
    }
    if (present->camera.zoom > present->max_zoom) {
        present->camera.zoom = present->max_zoom;
    }

    const float relative_zoom = present->camera.zoom / present->max_zoom;
    const float speed = present->movement_speed * dt / relative_zoom;
    if (IsKeyDown(KEY_D)) {
        present->camera.target.x += speed;
    }
    if (IsKeyDown(KEY_A)) {
        present->camera.target.x -= speed;
    }
    if (IsKeyDown(KEY_W)) {
        present->camera.target.y -= speed;
    }
    if (IsKeyDown(KEY_S)) {
        present->camera.target.y += speed;
    }
}

static void obj__present_draw(obj_t self) {
    obj_present_t present = (obj_present_t) self;
    
    BeginDrawing();
    ClearBackground(BLACK);
    
    BeginMode2D(present->camera);
    obj__present_draw_scene(self);
    EndMode2D();

    obj__present_draw_overlay(self);

    EndDrawing();
}

static void obj__present_draw_wire(obj_t self, Vector2 from, Vector2 to) {
    obj_present_t present = (obj_present_t) self;

    const float wire_thickness = 1.5f;
    const float electron_radius = wire_thickness / 2;
    const Color electron_color = GREEN;
    const float electron_span = 10.0f;
    const Color wire_color = BLUE;
    const double time_scale = 50.0f;
    const Vector2 target_vector = Vector2_Normalize(Vector2_Sub(to, from));

    DrawLineEx(from, to, wire_thickness, wire_color);

    double t_end = fabsf(target_vector.x) > 0.01 ? (to.x - from.x) / target_vector.x : (to.y - from.y) / target_vector.y;
    double t_next = fmod(present->time_cur * time_scale, t_end);
    Vector2 next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    while (t_next > 0.0) {
        DrawCircleV(next, electron_radius, electron_color);
        t_next -= electron_span;
        next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    }
    t_next = fmod(present->time_cur * time_scale, t_end) + electron_span;
    next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    while (t_next < t_end) {
        DrawCircleV(next, electron_radius, electron_color);
        t_next += electron_span;
        next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    }
}

static void obj__present_draw_scene(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    if (!present->drawn_node) {
        return ;
    }

    const Vector2 drawn_node_p = {
        .x = -present->average_node_dims.x / 2.0f,
        .y = -present->average_node_dims.y / 2.0f
    };

    Rectangle node_rec = obj__present_draw_node(self, present->drawn_node, drawn_node_p);
    
    Vector2 input_line_end = {
        node_rec.x + node_rec.width / 2,
        node_rec.y + node_rec.height,
    };

    const float vertical_wire_extension_len = 30.0f;
    const float vertical_wire_extension_span_width = 40.0f;
    const float horizontal_wire_extension_len = 30.0f;
    const float horizontal_wire_extension_span_height = 40.0f;
    const float average_node_width_expected = 150.0f;
    const float output_horizontal_spacing = horizontal_wire_extension_len + 50.0f;
    const float input_horizontal_spacing = horizontal_wire_extension_len + 50.0f;

    float total_input_horizontal_space_expected = present->drawn_node->inputs_top * average_node_width_expected;
    if (present->drawn_node->inputs_top > 1) {
        total_input_horizontal_space_expected += input_horizontal_spacing * present->drawn_node->inputs_top - 1;
    }

    Vector2 input_top_left_p_cur = {
        .x = node_rec.x + node_rec.width / 2.0f - total_input_horizontal_space_expected / 2.0f,
        .y = node_rec.y + node_rec.height + 100.0f
    };

    for (size_t input_index = 0; input_index < present->drawn_node->inputs_top; ++input_index) {
        obj_t input = present->drawn_node->inputs[input_index];
        Rectangle input_rec = obj__present_draw_node(self, input, input_top_left_p_cur);
        if (input_rec.width == 0.0f && input_rec.height == 0.0f) {
            continue ;
        }

        input_top_left_p_cur.x += input_rec.width + input_horizontal_spacing;
        
        Vector2 input_line_start = {
            input_rec.x + input_rec.width / 2,
            input_rec.y,
        };
        
        obj__present_draw_wire(self, input_line_start, input_line_end);

        if (input->outputs_top > 1) {
            const Vector2 start_p = {
                .x = input_rec.x,
                .y = input_rec.y + input_rec.height / 2
            };
            const float vertical_increment = input->outputs_top > 2 ? horizontal_wire_extension_span_height / (input->outputs_top - 2) : 0.0f;
            Vector2 end_p = {
                .x = input_rec.x - horizontal_wire_extension_len,
                .y = input_rec.y + input_rec.height / 2
            };
            if (input->outputs_top > 2) {
                end_p.y -= horizontal_wire_extension_span_height / 2.0f;
            }
            for (size_t output_index = 0; output_index < input->outputs_top; ++output_index) {
                obj_t output_of_input = input->outputs[output_index];
                if (output_of_input != present->drawn_node) {
                    obj__present_draw_wire(self, start_p, end_p);
                    end_p.y += vertical_increment;
                }
            }
        }

        if (input->inputs_top > 0) {
            const Vector2 end_p = {
                .x = input_rec.x + input_rec.width / 2,
                .y = input_rec.y + input_rec.height
            };
            const float horizontal_increment = input->inputs_top > 1 ? vertical_wire_extension_span_width / (input->inputs_top - 1) : 0.0f;
            Vector2 start_p = {
                .x = input_rec.x + input_rec.width / 2.0f,
                .y = input_rec.y + input_rec.height + vertical_wire_extension_len
            };
            if (input->inputs_top > 1) {
                start_p.x -= vertical_wire_extension_span_width / 2.0f;
            }
            for (size_t input_index = 0; input_index < input->inputs_top; ++input_index) {
                obj__present_draw_wire(self, start_p, end_p);
                start_p.x += horizontal_increment;
            }
        }
    }

    Vector2 output_line_start = {
        node_rec.x + node_rec.width / 2,
        node_rec.y,
    };

    float total_output_horizontal_space_expected = present->drawn_node->outputs_top * average_node_width_expected;
    if (present->drawn_node->outputs_top > 1) {
        total_output_horizontal_space_expected += output_horizontal_spacing * present->drawn_node->outputs_top - 1;
    }

    Vector2 output_top_left_p_cur = {
        .x = node_rec.x + node_rec.width / 2.0f - total_output_horizontal_space_expected / 2.0f,
        .y = node_rec.y - 300.0f
    };

    for (size_t output_index = 0; output_index < present->drawn_node->outputs_top; ++output_index) {
        obj_t output = present->drawn_node->outputs[output_index];
        Rectangle output_rec = obj__present_draw_node(self, output, output_top_left_p_cur);
        if (output_rec.width == 0.0f && output_rec.height == 0.0f) {
            continue ;
        }

        output_top_left_p_cur.x += output_rec.width + output_horizontal_spacing;
        
        Vector2 output_line_end = {
            output_rec.x + output_rec.width / 2,
            output_rec.y + output_rec.height,
        };

        obj__present_draw_wire(self, output_line_start, output_line_end);

        if (output->inputs_top > 1) {
            const Vector2 end_p = {
                .x = output_rec.x,
                .y = output_rec.y + output_rec.height / 2
            };
            const float vertical_increment = output->inputs_top > 2 ? horizontal_wire_extension_span_height / (output->inputs_top - 2) : 0.0f;
            Vector2 start_p = {
                .x = output_rec.x - horizontal_wire_extension_len,
                .y = output_rec.y + output_rec.height / 2
            };
            if (output->inputs_top > 2) {
                start_p.y -= horizontal_wire_extension_span_height / 2.0f;
            }
            for (size_t input_index = 0; input_index < output->inputs_top; ++input_index) {
                obj_t input_of_output = output->inputs[input_index];
                if (input_of_output != present->drawn_node) {
                    obj__present_draw_wire(self, start_p, end_p);
                    start_p.y += vertical_increment;
                }
            }
        }

        if (output->outputs_top > 0) {
            const Vector2 start_p = {
                .x = output_rec.x + output_rec.width / 2,
                .y = output_rec.y
            };
            const float horizontal_increment = output->outputs_top > 1 ? vertical_wire_extension_span_width / (output->outputs_top - 1) : 0.0f;
            Vector2 end_p = {
                .x = output_rec.x + output_rec.width / 2,
                .y = output_rec.y - vertical_wire_extension_len
            };
            if (output->outputs_top > 1) {
                end_p.x -= vertical_wire_extension_span_width / 2.0f;
            }
            for (size_t output_index = 0; output_index < output->outputs_top; ++output_index) {
                obj__present_draw_wire(self, start_p, end_p);
                end_p.x += horizontal_increment;
            }
        }
    }
}

static void obj__present_draw_overlay(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    char text[128];
    const float font_size = 24.0f;
    const float font_spacing = 1.0f;

    const Vector2 margin = {
        .x = 10.0f,
        .y = 5.0f
    };
    Vector2 graph_rec_p = margin;
    const Vector2 mp = GetMousePosition();
    float biggest_text_rec_height = 0.0f;
    for (size_t node_index = 0; node_index < _->objects_top; ++node_index) {
        obj_t node = _->objects[node_index];
        snprintf(text, sizeof(text), "%lu", node_index);
        Vector2 text_dims = MeasureTextEx(present->font, text, font_size, font_spacing);
        Rectangle text_rec = {
            graph_rec_p.x,
            graph_rec_p.y,
            text_dims.x + margin.x,
            text_dims.y + margin.y
        };
        if (biggest_text_rec_height < text_rec.height) {
            biggest_text_rec_height = text_rec.height;
        }

        Vector2 font_p = {
            .x = text_rec.x + (text_rec.width - text_dims.x) / 2,
            .y = text_rec.y + (text_rec.height - text_dims.y) / 2
        };
        int is_hovered = Rec_IsInside(text_rec, mp);
        if (is_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            obj__present_update_node_as_drawn(self, node);
        }

        Color rec_color = is_hovered || node == present->drawn_node ? RED : YELLOW;
        DrawRectangleLinesEx(text_rec, 1.0f, rec_color);
        DrawTextEx(present->font, text, font_p, font_size, font_spacing, WHITE);
        graph_rec_p.x += text_rec.width + 10.0f;
        if (graph_rec_p.x + text_rec.width >= present->width) {
            graph_rec_p.x = margin.x;
            graph_rec_p.y += biggest_text_rec_height + margin.y;
            biggest_text_rec_height = 0.0f;
        }
    }

    snprintf(text, sizeof(text), "Time: %.2fs", present->time_cur);
    Vector2 current_time_p_text_dims = MeasureTextEx(present->font, text, font_size, font_spacing);
    Vector2 current_time_p = {
        .x = margin.x,
        .y = present->height - current_time_p_text_dims.y - margin.y
    };
    DrawTextEx(present->font, text, current_time_p, font_size, font_spacing, WHITE);
}

static void obj__run_present(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    obj__set_start(self, builder__get_time_stamp());

    char title_buffer[256];
    obj__describe_short(self, title_buffer, sizeof(title_buffer));

    present->width = 2000;
    present->height = 1000;
    InitWindow(present->width, present->height, title_buffer);
    SetTargetFPS(60);
    present->movement_speed = 100.0;
    present->min_zoom = 0.3f;
    present->max_zoom = 14.0f;

    obj__present_update_reset_camera(self);

    present->font = LoadFont("/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf");
    obj__present_update_node_as_drawn(self, self);

    double time_prev = builder__get_time_stamp() - builder__get_time_stamp_init();
    while (!WindowShouldClose()) {
        double time_cur = builder__get_time_stamp() - builder__get_time_stamp_init();
        const double dt = time_cur - time_prev;
        time_prev = time_cur;
        present->time_cur = time_cur;

        obj__present_update(self, dt);
        obj__present_draw(self);
    }

    obj__set_success(self, builder__get_time_stamp());
    obj__set_finish(self, builder__get_time_stamp());

    CloseWindow();
}

static void obj__describe_short_present(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "PRESENT");
}

static void obj__describe_long_present(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "PRESENT");
}

static void obj__destroy_present(obj_t self) {
    (void) self;
}

obj_t obj__present() {
    obj_present_t result = (obj_present_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_present;
    result->base.describe_short = &obj__describe_short_present;
    result->base.describe_long  = &obj__describe_long_present;
    result->base.destroy        = &obj__destroy_present;

    // obj_t builder_gfx_h = obj__file_modified(oscillator_200ms, "builder_gfx.h");
    // obj_t builder_gfx_c = obj__file_modified(oscillator_200ms, "builder_gfx.c");
    // obj_t builder_gfx_o = obj__file_modified(
    //     obj__list(
    //         obj__process(
    //             obj__exec(
    //                 obj__list(c_compiler, builder_h, builder_gfx_h, builder_gfx_c, 0),
    //                 "%s -g -I. -c %s -o builder_gfx.o -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(builder_gfx_c)
    //             ),
    //             0,
    //             oscillator_200ms
    //         ),
    //         oscillator_200ms,
    //         0
    //     ),
    //     "builder_gfx.o"
    // );

    // obj__push_input((obj_t) result, builder_gfx_o);

    return obj__process((obj_t) result, 0, _->oscillator_200ms);
}
