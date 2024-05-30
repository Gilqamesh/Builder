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

typedef struct state {
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

    size_t nodes_size;
    size_t nodes_top;
    obj_t* nodes;
} state_t;
static state_t state;

static float Vector2_Dot(Vector2 v, Vector2 w) { return v.x * w.x + v.y * w.y; }
static float Vector2_Len(Vector2 v) { return sqrtf(Vector2_Dot(v, v)); }
static Vector2 Vector2_Normalize(Vector2 v) { float len = Vector2_Len(v); v.x /= len; v.y /= len; return v; }
static Vector2 Vector2_Scale(Vector2 v, float f) { v.x *= f; v.y *= f; return v; }
static Vector2 Vector2_Add(Vector2 v, Vector2 w) { v.x += w.x; v.y += w.y; return v; }
static Vector2 Vector2_Sub(Vector2 v, Vector2 w) { v.x -= w.x; v.y -= w.y; return v; }

static int Rec_IsInside(Rectangle rec, Vector2 p) { return p.x >= rec.x && p.x <= rec.x + rec.width && p.y >= rec.y && p.y <= rec.y + rec.height; }

static void node__fill_set_transient_flag(obj_t node, int flag);
static void nodes__push_all(obj_t node);

static void init(obj_t node, obj_t title);

static void update(double dt);
static void update_node_as_drawn(obj_t node);
static void update_reset_camera();

static void      draw();
static Rectangle draw_node(obj_t node, Vector2 top_left_p);
static void      draw_wire(Vector2 from, Vector2 to);
static void      draw_scene();
static void      draw_overlay();

static void destroy();

static Rectangle draw_node(obj_t node, Vector2 top_left_p) {
    static char buffer[512];
    obj__describe_long(node, buffer, sizeof(buffer));

    const int font_size = 10;
    Vector2 text_dims = MeasureTextEx(state.font, buffer, font_size, 1.0f);

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
    Vector2 mp_world = GetScreenToWorld2D(mp_screen, state.camera);
    if (Rec_IsInside(node_rec, mp_world)) {
        node_color = RED;
        DrawRectangleLinesEx(node_rec, 1.0f, RED);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            update_node_as_drawn(node);
        }
    }

    const double max_blink_periodicity = 0.15;
    const double time_since_last_successful_run = builder__get_time_stamp() - node->time_ran_start;
    if (node->time_ran_finish < node->time_ran_start || time_since_last_successful_run <= max_blink_periodicity) {
        node_color = PURPLE;
    }

    DrawRectangleLinesEx(node_rec, 1.0f, node_color);
    DrawTextEx(state.font, buffer, text_p, font_size, 1.0f, WHITE);

    state.running_average_node_dims.x = (state.running_average_node_dims.x * state.running_average_node_dims_top + node_rec.width) / (state.running_average_node_dims_top + 1);
    state.running_average_node_dims.y = (state.running_average_node_dims.y * state.running_average_node_dims_top + node_rec.height) / (state.running_average_node_dims_top + 1);
    ++state.running_average_node_dims_top;

    return node_rec;
}

static void update_node_as_drawn(obj_t node) {
    state.drawn_node = node;
    update_reset_camera(&state);
    if (node) {
        if (state.drawn_node->outputs_top == 0 && state.drawn_node->inputs_top > 0) {
            state.camera.offset.y = state.height / 6.0f;
        } else if (state.drawn_node->outputs_top > 0 && state.drawn_node->inputs == 0) {
            state.camera.offset.y = state.height * 4.0f / 6.0f;
        }
    }
}

static void update_reset_camera() {
    memset(&state.camera, 0, sizeof(state.camera));

    state.camera.zoom = 2.0f;
    state.camera.offset.x = state.width / 2.0f;
    state.camera.offset.y = state.height / 2.0f;
}

static void nodes__push_all(obj_t node) {
    if (node->transient_flag) {
        return ;
    }
    node->transient_flag = 1;

    ARRAY_ENSURE_TOP(state.nodes, state.nodes_top, state.nodes_size);
    state.nodes[state.nodes_top++] = node;

    for (size_t input_index = 0; input_index < node->inputs_top; ++input_index) {
        obj_t input = node->inputs[input_index];
        nodes__push_all(input);
    }
    for (size_t output_index = 0; output_index < node->outputs_top; ++output_index) {
        obj_t output = node->outputs[output_index];
        nodes__push_all(output);
    }
}

static void node__fill_set_transient_flag(obj_t node, int flag) {
    if (node->transient_flag == flag) {
        return ;
    }
    node->transient_flag = flag;

    for (size_t input_index = 0; input_index < node->inputs_top; ++input_index) {
        obj_t input = node->inputs[input_index];
        node__fill_set_transient_flag(input, flag);
    }
    for (size_t output_index = 0; output_index < node->outputs_top; ++output_index) {
        obj_t output = node->outputs[output_index];
        node__fill_set_transient_flag(output, flag);
    }
}

static void init(obj_t node, obj_t title) {
    memset(&state, 0, sizeof(state));

    char title_buffer[256];
    char program_description[128];
    obj__describe_short(title, program_description, sizeof(program_description));
    double program_version = 0.0;
    for (size_t input_index = 0; input_index < title->inputs_top; ++input_index) {
        obj_t input = title->inputs[input_index];
        if (program_version < input->time_ran_start) {
            program_version = input->time_ran_start;
        }
    }
    time_t time_ran_successfully_t = (time_t) program_version;
    struct tm* t = localtime(&time_ran_successfully_t);
    snprintf(title_buffer, sizeof(title_buffer), "%02d/%02d/%d %02d:%02d:%02d, %s", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, program_description);

    state.width = 2000;
    state.height = 1000;
    InitWindow(state.width, state.height, title_buffer);
    SetTargetFPS(60);
    state.movement_speed = 100.0;
    state.min_zoom = 0.3f;
    state.max_zoom = 14.0f;

    nodes__push_all(node);
    node__fill_set_transient_flag(node, 0);

    update_reset_camera();

    state.font = LoadFont("/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf");
    update_node_as_drawn(node);
}

static void update(double dt) {
    state.average_node_dims = state.running_average_node_dims;
    state.running_average_node_dims = (Vector2){ 0 };
    state.running_average_node_dims_top = 0;

    float mouse_wheel = GetMouseWheelMove();
    if (mouse_wheel > 0.0f) {
        state.camera.zoom = 1.15f * state.camera.zoom;
    }
    else if (mouse_wheel < 0.0f) {
        state.camera.zoom = 0.9f * state.camera.zoom;
    }

    if (state.camera.zoom < state.min_zoom) {
        state.camera.zoom = state.min_zoom;
    }
    if (state.camera.zoom > state.max_zoom) {
        state.camera.zoom = state.max_zoom;
    }

    const float relative_zoom = state.camera.zoom / state.max_zoom;
    const float speed = state.movement_speed * dt / relative_zoom;
    if (IsKeyDown(KEY_D)) {
        state.camera.target.x += speed;
    }
    if (IsKeyDown(KEY_A)) {
        state.camera.target.x -= speed;
    }
    if (IsKeyDown(KEY_W)) {
        state.camera.target.y -= speed;
    }
    if (IsKeyDown(KEY_S)) {
        state.camera.target.y += speed;
    }
}

static void draw() {
    BeginDrawing();
    ClearBackground(BLACK);
    
    BeginMode2D(state.camera);
    draw_scene();
    EndMode2D();

    draw_overlay();

    EndDrawing();
}

static void draw_wire(Vector2 from, Vector2 to) {
    const float wire_thickness = 1.5f;
    const float electron_radius = wire_thickness / 2;
    const Color electron_color = GREEN;
    const float electron_span = 10.0f;
    const Color wire_color = BLUE;
    const double time_scale = 50.0f;
    const Vector2 target_vector = Vector2_Normalize(Vector2_Sub(to, from));

    DrawLineEx(from, to, wire_thickness, wire_color);

    double t_end = fabsf(target_vector.x) > 0.01 ? (to.x - from.x) / target_vector.x : (to.y - from.y) / target_vector.y;
    double t_next = fmod(state.time_cur * time_scale, t_end);
    Vector2 next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    while (t_next > 0.0) {
        DrawCircleV(next, electron_radius, electron_color);
        t_next -= electron_span;
        next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    }
    t_next = fmod(state.time_cur * time_scale, t_end) + electron_span;
    next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    while (t_next < t_end) {
        DrawCircleV(next, electron_radius, electron_color);
        t_next += electron_span;
        next = Vector2_Add(from, Vector2_Scale(target_vector, t_next));
    }
}

static void draw_scene() {
    if (!state.drawn_node) {
        return ;
    }

    const Vector2 drawn_node_p = {
        .x = -state.average_node_dims.x / 2.0f,
        .y = -state.average_node_dims.y / 2.0f
    };

    Rectangle node_rec = draw_node(state.drawn_node, drawn_node_p);
    
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

    float total_input_horizontal_space_expected = state.drawn_node->inputs_top * average_node_width_expected;
    if (state.drawn_node->inputs_top > 1) {
        total_input_horizontal_space_expected += input_horizontal_spacing * state.drawn_node->inputs_top - 1;
    }

    Vector2 input_top_left_p_cur = {
        .x = node_rec.x + node_rec.width / 2.0f - total_input_horizontal_space_expected / 2.0f,
        .y = node_rec.y + node_rec.height + 100.0f
    };

    for (size_t input_index = 0; input_index < state.drawn_node->inputs_top; ++input_index) {
        obj_t input = state.drawn_node->inputs[input_index];
        Rectangle input_rec = draw_node(input, input_top_left_p_cur);
        if (input_rec.width == 0.0f && input_rec.height == 0.0f) {
            continue ;
        }

        input_top_left_p_cur.x += input_rec.width + input_horizontal_spacing;
        
        Vector2 input_line_start = {
            input_rec.x + input_rec.width / 2,
            input_rec.y,
        };
        
        draw_wire(input_line_start, input_line_end);

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
                if (output_of_input != state.drawn_node) {
                    draw_wire(start_p, end_p);
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
                draw_wire(start_p, end_p);
                start_p.x += horizontal_increment;
            }
        }
    }

    Vector2 output_line_start = {
        node_rec.x + node_rec.width / 2,
        node_rec.y,
    };

    float total_output_horizontal_space_expected = state.drawn_node->outputs_top * average_node_width_expected;
    if (state.drawn_node->outputs_top > 1) {
        total_output_horizontal_space_expected += output_horizontal_spacing * state.drawn_node->outputs_top - 1;
    }

    Vector2 output_top_left_p_cur = {
        .x = node_rec.x + node_rec.width / 2.0f - total_output_horizontal_space_expected / 2.0f,
        .y = node_rec.y - 300.0f
    };

    for (size_t output_index = 0; output_index < state.drawn_node->outputs_top; ++output_index) {
        obj_t output = state.drawn_node->outputs[output_index];
        Rectangle output_rec = draw_node(output, output_top_left_p_cur);
        if (output_rec.width == 0.0f && output_rec.height == 0.0f) {
            continue ;
        }

        output_top_left_p_cur.x += output_rec.width + output_horizontal_spacing;
        
        Vector2 output_line_end = {
            output_rec.x + output_rec.width / 2,
            output_rec.y + output_rec.height,
        };

        draw_wire(output_line_start, output_line_end);

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
                if (input_of_output != state.drawn_node) {
                    draw_wire(start_p, end_p);
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
                draw_wire(start_p, end_p);
                end_p.x += horizontal_increment;
            }
        }
    }
}

static void draw_overlay() {
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
    for (size_t node_index = 0; node_index < state.nodes_top; ++node_index) {
        obj_t node = state.nodes[node_index];
        snprintf(text, sizeof(text), "%lu", node_index);
        Vector2 text_dims = MeasureTextEx(state.font, text, font_size, font_spacing);
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
            update_node_as_drawn(node);
        }

        Color rec_color = is_hovered || node == state.drawn_node ? RED : YELLOW;
        DrawRectangleLinesEx(text_rec, 1.0f, rec_color);
        DrawTextEx(state.font, text, font_p, font_size, font_spacing, WHITE);
        graph_rec_p.x += text_rec.width + 10.0f;
        if (graph_rec_p.x + text_rec.width >= state.width) {
            graph_rec_p.x = margin.x;
            graph_rec_p.y += biggest_text_rec_height + margin.y;
            biggest_text_rec_height = 0.0f;
        }
    }

    snprintf(text, sizeof(text), "Time: %.2fs", state.time_cur);
    Vector2 current_time_p_text_dims = MeasureTextEx(state.font, text, font_size, font_spacing);
    Vector2 current_time_p = {
        .x = margin.x,
        .y = state.height - current_time_p_text_dims.y - margin.y
    };
    DrawTextEx(state.font, text, current_time_p, font_size, font_spacing, WHITE);
}

static void destroy() {
    CloseWindow();
}

void builder_gfx__exec(obj_t obj, obj_t title) {
    init(obj, title);

    double time_prev = GetTime();
    double time_accumulated_between_builder_updates = 0.0;
    while (!WindowShouldClose()) {
        double time_cur = GetTime();
        const double dt = time_cur - time_prev;
        time_prev = time_cur;
        state.time_cur = time_cur;

        time_accumulated_between_builder_updates += dt;
        if (time_accumulated_between_builder_updates > 0.1) {
            time_accumulated_between_builder_updates -= 0.1;
            obj__run(obj);
        }
        
        update(dt);
        draw();
    }

    destroy();
}
