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
    float min_zoom;
    float max_zoom;
    float movement_speed;
    Camera2D camera;
    Font font;
    double time_cur;
    obj_t drawn_node;

    RenderTexture2D scene_render_texture;
    RenderTexture2D overlay_render_texture;
} *obj_present_t;

static void obj__present_update(obj_t self, double dt);
static void obj__present_update_node_as_drawn(obj_t self, obj_t node);
static void obj__present_update_reset_camera(obj_t self);

static void obj__present_draw(obj_t self);
static void obj__present_draw_node(obj_t self, obj_t node, Rectangle rec, int depth);
static void obj__present_draw_scene(obj_t self);
static void obj__present_draw_overlay(obj_t self);

static void obj__run_present(obj_t self);
static void obj__describe_short_present(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_present(obj_t self, char* buffer, int buffer_size);
static void obj__destroy_present(obj_t self);

static void obj__present_draw_node(obj_t self, obj_t node, Rectangle rec, int depth) {
    obj_present_t present = (obj_present_t) self;

    static char buffer[512];
    obj__describe_long(node, buffer, sizeof(buffer));

    float font_size = 10.0f;
    const float font_size_min = 5.0f;
    // fit rec as best as we can
    Vector2 text_dims = MeasureTextEx(present->font, buffer, font_size, 1.0f);
    while (
        text_dims.x < rec.width &&
        text_dims.y < rec.height
    ) {
        font_size += 1.0f;
        text_dims = MeasureTextEx(present->font, buffer, font_size, 1.0f);
    }
    while (
        font_size_min < font_size &&
        (rec.width < text_dims.x ||
        rec.height < text_dims.y)
    ) {
        font_size -= 1.0f;
        text_dims = MeasureTextEx(present->font, buffer, font_size, 1.0f);
    }
    if (font_size_min < font_size) {
        // Vector2 text_p = {
        //     .x = rec.x + (rec.width - text_dims.x) / 2.0f,
        //     .y = rec.y + (rec.height - text_dims.y) / 2.0f
        // };
        // DrawTextEx(present->font, buffer, text_p, font_size, 1.0f, WHITE);
    } else {
        // todo: describe short
    }

    Color node_color = YELLOW;
    Vector2 mp_screen = GetMousePosition();
    Vector2 mp_world = GetScreenToWorld2D(mp_screen, present->camera);
    if (Rec_IsInside(rec, mp_world)) {
        node_color = RED;
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
    
    const float line_thickness_min = 0.7f;
    const float line_thickness_max = 10.0f;
    float line_thickness = present->max_zoom * 0.2f / present->camera.zoom;
    if (line_thickness < line_thickness_min) {
        line_thickness = line_thickness_min;
    }
    if (line_thickness_max < line_thickness) {
        line_thickness = line_thickness_max;
    }
    if (5 < depth) {
        node_color.a = 0;
    } else {
        node_color.a = 255 - (unsigned char) (((float) depth / 5.0f) * 255.0f);
    }
    DrawRectangleLinesEx(rec, line_thickness, node_color);
}

static void obj__present_update_node_as_drawn(obj_t self, obj_t node) {
    obj_present_t present = (obj_present_t) self;

    present->drawn_node = node;
    obj__present_update_reset_camera(self);
}

static void obj__present_update_reset_camera(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    memset(&present->camera, 0, sizeof(present->camera));

    present->camera.zoom = 1.0f;
    present->camera.offset.x = present->width / 2.0f;
    present->camera.offset.y = present->height / 2.0f;
}

static void obj__present_update(obj_t self, double dt) {
    obj_present_t present = (obj_present_t) self;

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

    obj__present_draw_scene(self);
    obj__present_draw_overlay(self);

    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode2D(present->camera);
    DrawTexturePro(
        present->scene_render_texture.texture,
        (Rectangle){ 0, 0, (float) present->scene_render_texture.texture.width, (float) -present->scene_render_texture.texture.height },
        (Rectangle){ 0, 0, present->width, present->height },
        (Vector2) { present->width / 2, present->height / 2 },
        0,
        WHITE
    );
    EndMode2D();

    DrawTexturePro(
        present->overlay_render_texture.texture,
        (Rectangle){ 0, 0, (float) present->overlay_render_texture.texture.width, (float) -present->overlay_render_texture.texture.height },
        (Rectangle){ 0, 0, present->width, present->height },
        (Vector2) { 0, 0 },
        0,
        WHITE
    );

    EndDrawing();
}

static void obj__present_draw_inputs(obj_t self, obj_t node, Rectangle rec, int depth) {
    if (node->inputs_top == 0) {
        return ;
    }

    struct state {
        Rectangle rec;
        int n;
        int depth;
    } states_stack[32];
    int states_stack_top = 0;
    states_stack[states_stack_top++] = (struct state) {
        .rec   = rec,
        .n     = node->inputs_top,
        .depth = 0
    };
    size_t drawn_input_index = 0;
    while (0 < states_stack_top) {
        struct state state = states_stack[--states_stack_top];
        if (1 < state.n) {
            Rectangle rec_left  = state.rec;
            Rectangle rec_right = state.rec;
            int n_left = state.n >> 1;
            int n_right = state.n - n_left;
            if (state.depth & 1) {
                rec_right.x += rec_right.width / 2;
                rec_right.width /= 2;
                rec_left.width /= 2;
            } else {
                rec_right.y += rec_right.height / 2;
                rec_right.height /= 2;
                rec_left.height /= 2;
            }

            states_stack[states_stack_top++] = (struct state) {
                .rec = rec_left,
                .n = n_left,
                .depth = state.depth + 1
            };
            if (states_stack_top == ARRAY_SIZE(states_stack)) {
                break ;
            }
            states_stack[states_stack_top++] = (struct state) {
                .rec = rec_right,
                .n = n_right,
                .depth = state.depth + 1
            };
        } else {
            assert(drawn_input_index < node->inputs_top);
            obj_t input = node->inputs[drawn_input_index++];
            Rectangle input_rec = {
                .x = state.rec.x + state.rec.width * 0.2,
                .y = state.rec.y + state.rec.height * 0.2,
                .width = state.rec.width * 0.6,
                .height = state.rec.height * 0.6
            };
            obj__present_draw_node(self, input, input_rec, depth);
            obj__present_draw_inputs(self, input, input_rec, depth + 1);
        }
    }
}

static void obj__present_draw_scene(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    if (!present->drawn_node) {
        return ;
    }

    BeginTextureMode(present->scene_render_texture);
    ClearBackground(BLANK);
    
    // BeginMode2D(present->camera);

    Rectangle starting_rec = {
        .x = 0,
        .y = 0,
        .width = present->scene_render_texture.texture.width,
        .height = present->scene_render_texture.texture.height
    };
    obj__present_draw_node(self, present->drawn_node, starting_rec, 0);
    if (0 < present->drawn_node->inputs_top) {
        obj__present_draw_inputs(self, present->drawn_node, starting_rec, 1);
    }

    // EndMode2D();

    EndTextureMode();
}

static void obj__present_draw_overlay(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    BeginTextureMode(present->overlay_render_texture);

    ClearBackground(BLANK);

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
    Vector2 text_dims = MeasureTextEx(present->font, text, font_size, font_spacing);
    Vector2 text_p = {
        .x = margin.x,
        .y = present->height - text_dims.y - margin.y
    };
    DrawTextEx(present->font, text, text_p, font_size, font_spacing, WHITE);

    snprintf(text, sizeof(text), "Zoom: %.2fs", present->camera.zoom);
    text_dims = MeasureTextEx(present->font, text, font_size, font_spacing);
    text_p.y -= text_dims.y + margin.y;
    DrawTextEx(present->font, text, text_p, font_size, font_spacing, WHITE);

    EndTextureMode();
}

static void obj__present_init(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    char title_buffer[256];
    obj__describe_short(self, title_buffer, sizeof(title_buffer));

    present->width = 2000;
    present->height = 1000;
    InitWindow(present->width, present->height, title_buffer);
    SetTargetFPS(60);
    present->movement_speed = 100.0;
    present->min_zoom = 0.1f;
    present->max_zoom = 20.0f;

    obj__present_update_reset_camera(self);

    present->font = LoadFont("/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf");
    present->scene_render_texture = LoadRenderTexture(present->width, present->height);
    present->overlay_render_texture = LoadRenderTexture(present->width, present->height);

    obj__present_update_node_as_drawn(self, self);
}

static void obj__present_destroy(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    UnloadFont(present->font);
    UnloadRenderTexture(present->scene_render_texture);
    UnloadRenderTexture(present->overlay_render_texture);

    CloseWindow();
}

static void obj__run_present(obj_t self) {
    obj_present_t present = (obj_present_t) self;

    obj__set_start(self, builder__get_time_stamp());

    obj__present_init(self);

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

    obj__present_destroy(self);
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
