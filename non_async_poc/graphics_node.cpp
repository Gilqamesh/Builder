#include "graphics_node.h"

#include "raylib.h"
#include "portaudio.h"

#include <cassert>
#include <cmath>
#include <unordered_set>
#include <iostream>
#include <thread>
#include <cfloat>
#include <cstring>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

// todo: add zoom
Vector2 world_to_screen(Vector2 v, Camera2D camera) {
    v.x += camera.offset.x;
    v.y += camera.offset.y;

    return v;
}

Vector2 screen_to_world(Vector2 v, Camera2D camera) {
    return Vector2{
        (v.x - camera.offset.x) / camera.zoom + camera.target.x,
        (v.y - camera.offset.y) / camera.zoom + camera.target.y
    };
}

void draw_text(const string& str, Vector2 middle, Color color, Vector2 max_dims, float font_size, float font_spacing) {
    float font_size_cur = font_size;
    Vector2 text_dims = MeasureTextEx(GetFontDefault(), str.data(), font_size_cur, font_spacing);
    while (
        max_dims.x < text_dims.x &&
        max_dims.x < text_dims.y
    ) {
        font_size_cur *= 0.9;
        text_dims = MeasureTextEx(GetFontDefault(), str.data(), font_size_cur, font_spacing);
    }
    DrawTextEx(GetFontDefault(), str.data(), Vector2{ middle.x - text_dims.x / 2.0f, middle.y - text_dims.y / 2.0f }, font_size_cur, font_spacing, color);
}

struct graphics_engine_node_data_t {
    int w_width;
    int w_height;
    float font_size;
    float font_spacing;
    Camera2D camera;
    Vector2 mp;
    unordered_set<node_t*> time_nodes;
    time_type_t t_frame_begin;
    bool should_update_thread_run;
    bool should_update;
    thread update_thread;
    unordered_map<string, function<graphics_node_t*()>> generator;
};

struct graphics_node_data_t {
    Vector2 p;
    float r;
    Color c;
    bool selected;
    bool dragged;
};

static int n_graphics_engine_nodes = 0;
graphics_engine_node_t::graphics_engine_node_t():
    node_t(
        [](node_t& self) -> bool {
            graphics_engine_node_data_t* graphics_engine_node_data = self.add_state<graphics_engine_node_data_t>();
            graphics_engine_node_data->generator["const"] = []() { return new constant_node_t; };
            graphics_engine_node_data->generator["sum"] = []() { return new sum_node_t; };
            graphics_engine_node_data->generator["acc"] = []() { return new accumulate_node_t; };
            graphics_engine_node_data->generator["dec"] = []() { return new decrement_node_t; };
            graphics_engine_node_data->generator["time"] = []() { return new time_node_t; };
            graphics_engine_node_data->generator["sin"] = []() { return new sin_node_t; };
            graphics_engine_node_data->generator["mul"] = []() { return new mul_node_t; };
            graphics_engine_node_data->generator["floor"] = []() { return new floor_node_t; };
            graphics_engine_node_data->generator["sine wave"] = []() { return new sine_wave_node_t; };
            graphics_engine_node_data->generator["audio mixer"] = []() { return new audio_mixer_node_t; };
            graphics_engine_node_data->w_width = 1800;
            graphics_engine_node_data->w_height = 1000;
            graphics_engine_node_data->font_size = 20.0f;
            graphics_engine_node_data->font_spacing = 1.0f;
            graphics_engine_node_data->camera.offset.x = graphics_engine_node_data->w_width / 2.0f;
            graphics_engine_node_data->camera.offset.y = graphics_engine_node_data->w_height / 2.0f;
            graphics_engine_node_data->camera.rotation = 0.0f;
            graphics_engine_node_data->camera.target.x = 0.0f;
            graphics_engine_node_data->camera.target.y = 0.0f;
            graphics_engine_node_data->camera.zoom = 1.0f;

            if (n_graphics_engine_nodes++ == 0) {
                InitWindow(graphics_engine_node_data->w_width, graphics_engine_node_data->w_height, "The Window");
                SetTargetFPS(60);
            }

            // this is either interface or state.. hmm
            self.add_slot<graphics_node_data_t>();

            graphics_engine_node_data->should_update_thread_run = true;
            graphics_engine_node_data->should_update = false;
            graphics_engine_node_data->update_thread = thread([](node_t* self) {
                graphics_engine_node_data_t* graphics_engine_node_data = self->write_state<graphics_engine_node_data_t>();
                auto update_fn = self->find_interface<function<void(node_t&)>>("update");
                assert(update_fn);
                while (graphics_engine_node_data->should_update_thread_run) {
                    if (graphics_engine_node_data->should_update) {
                        update_fn(*self);
                        graphics_engine_node_data->should_update = false;
                    }
                }
            }, &self);
        },
        [](node_t& self) -> bool {
            // run the game loop here
        },
        [](const node_t& self) -> string {
        },
        [](node_t& self) {
            if (--n_graphics_engine_nodes == 0) {
            }
        }
    )
{
    struct graphics_engine_t {
        bool window_should_close();
        void update();

        Camera2D camera;
        Vector2 mp;
    };
    add_state<graphics_engine_t>();

    m_interface.add<function<bool()>>("window-should-close", []() {
        return WindowShouldClose();
    });
    m_interface.add<function<void(node_t&)>>("update", [](node_t& self) {
        graphics_engine_node_data_t* graphics_engine_node_data = self.write_state<graphics_engine_node_data_t>();

        graphics_engine_node_data->mp = GetMousePosition();

        time_type_t t_propagate = get_time();
        for (node_t* node : graphics_engine_node_data->time_nodes) {
            node->run_force(t_propagate);
        }

        // while (!event_queue.empty()) {
        //     const event_t& event = event_queue.top();
        //     if (t_frame_begin < event.t) {
        //         break ;
        //     }
        //     event_queue.pop();

        //     if (event.f) {
        //         cout << "Running callback for event.." << endl;
        //         event.f(event.context);
        //     } else {
        //         cout << "No callback for event!" << endl;
        //     }
        // }

        bool resolved_selected = false;
        
        auto& inputs = self.write_inputs();
        auto node_pair_it = inputs.begin();
        while (node_pair_it != inputs.end()) {
            graphics_node_data_t* graphics_node_data = node_pair_it->first->write_state<graphics_node_data_t>();
            if (IsKeyPressed(KEY_Q)) {
                graphics_node_data->selected = false;
            }

            auto is_hovered_fn = node_pair_it->first->find_interface<function<bool(node_t&)>>("is-hovered");
            if (is_hovered_fn && is_hovered_fn(*node_pair_it->first)) {
                if (IsKeyPressed(KEY_DELETE)) {
                    delete node_pair_it->first;
                    node_pair_it = inputs.erase(node_pair_it);
                    continue ;
                }

                if (IsKeyPressed(KEY_S) && !resolved_selected) {
                    resolved_selected = true;
                    auto other_node_pair_it = inputs.begin();
                    while (other_node_pair_it != inputs.end()) {
                        if (other_node_pair_it != node_pair_it) {
                            graphics_node_data_t* other_graphics_node_data = other_node_pair_it->first->write_state<graphics_node_data_t>();
                            if (other_graphics_node_data->selected) {
                                other_graphics_node_data->selected = false;
                                node_pair_it->first->add_input(other_node_pair_it->first);
                            }
                        }
                        ++other_node_pair_it;
                    }
                }

                if (IsKeyPressed(KEY_A)) {
                    graphics_node_data->selected = !graphics_node_data->selected;
                }

                if (IsKeyPressed(KEY_D)) {
                    node_pair_it->first->isolate();
                }
                
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    (void) node_pair_it->first->run_force(get_time());
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    graphics_node_data->dragged = !graphics_node_data->dragged;
                }
            }

            ++node_pair_it;
        }
    });

    auto draw_palette_fn = m_interface.add<function<void(node_t&)>>("draw-palette", [](node_t& self) {
        graphics_engine_node_data_t* graphics_engine_node_data = self.write_state<graphics_engine_node_data_t>();

        assert(graphics_engine_node_data->w_width % 10 == 0);
        assert(graphics_engine_node_data->w_height % 10 == 0);
        float rec_width = graphics_engine_node_data->w_width / 10.0f;
        float rec_height = graphics_engine_node_data->w_height / 10.0f;
        float rec_x = 0.0f;
        float rec_y = 0.0f;

        Rectangle rec_cur = { rec_x, rec_y, rec_width, rec_height };

        for (const auto& p : graphics_engine_node_data->generator) {
            DrawRectangleLinesEx(rec_cur, 2.0f, RED);

            if (
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                rec_cur.x <= graphics_engine_node_data->mp.x && graphics_engine_node_data->mp.x <= rec_cur.x + rec_cur.width &&
                rec_cur.y <= graphics_engine_node_data->mp.y && graphics_engine_node_data->mp.y <= rec_cur.y + rec_cur.height
            ) {
                graphics_node_t* node = p.second();
                graphics_node_data_t* graphics_node_data = node->write_state<graphics_node_data_t>();
                graphics_node_data->p = screen_to_world(graphics_engine_node_data->mp, graphics_engine_node_data->camera);
                graphics_node_data->dragged = true;
            }

            draw_text(
                p.first,
                Vector2{ rec_cur.x + rec_cur.width / 2.0f, rec_cur.y + rec_cur.height / 2.0f },
                BLACK,
                Vector2{ rec_cur.width, rec_cur.height },
                graphics_engine_node_data->font_size,
                graphics_engine_node_data->font_spacing
            );

            rec_cur.x += rec_width;
            if (graphics_engine_node_data->w_width <= rec_cur.x) {
                rec_cur.x = 0;
                rec_cur.y += rec_height;
            }
        }
    });
    m_interface.add<function<void(node_t&)>>("draw", [draw_palette_fn](node_t& self) {
        graphics_engine_node_data_t* graphics_engine_node_data = self.write_state<graphics_engine_node_data_t>();

        draw_palette_fn(self);
        BeginMode2D(graphics_engine_node_data->camera);

        auto& inputs = self.write_inputs();
        auto node_pair_it = inputs.begin();
        while (node_pair_it != inputs.end()) {
            auto draw_fn = node_pair_it->first->find_interface<function<void(node_t&)>>("draw");
            if (draw_fn) {
                draw_fn(*node_pair_it->first);
            }
            ++node_pair_it;
        }
        EndMode2D();
    });
}

graphics_engine_node_t::~graphics_engine_node_t() {
}

graphics_node_t::graphics_node_t(
    const function<bool(node_t&)>& init_fn,
    const function<bool(node_t&)>& run_fn,
    const function<string(const node_t&)>& describe_fn,
    const function<void(node_t&)>& deinit_fn,
    float x, float y
): node_t(init_fn, run_fn, describe_fn, deinit_fn)
{
    graphics_node_data_t* graphics_node_data = add_state<graphics_node_data_t>();
    graphics_node_data->selected = false;
    graphics_node_data->dragged = false;
    graphics_node_data->p = Vector2{ x, y };
    graphics_node_data->r = 100.0f;
    graphics_node_data->c = BLUE;

    m_interface.add<function<bool(node_t&, graphics_engine_node_data_t*)>>("is-hovered", [](node_t& self, graphics_engine_node_data_t* graphics_engine_node_data) {
        const graphics_node_data_t* graphics_node_data = self.read_state<graphics_node_data_t>();
        if (!graphics_node_data) {
            return false;
        }

        Vector2 screen_p = world_to_screen(graphics_node_data->p, graphics_engine_node_data->camera);
        float dx = graphics_engine_node_data->mp.x - screen_p.x;
        float dy = graphics_engine_node_data->mp.y - screen_p.y;
        return dx * dx + dy * dy < graphics_node_data->r * graphics_node_data->r;
    });

    m_interface.add<function<void(node_t&, graphics_engine_node_data_t*)>>("draw", [](node_t& self, graphics_engine_node_data_t* graphics_engine_node_data) {
        graphics_node_data_t* graphics_node_data = self.write_state<graphics_node_data_t>();

        if (graphics_node_data->dragged) {
            // note: this is more of an interface provided by the graphics engine 'screen_to_world' aka translate from view to world space
            graphics_node_data->p = screen_to_world(graphics_engine_node_data->mp, graphics_engine_node_data->camera);
        }

        if (graphics_node_data->selected) {
            graphics_node_data->c = YELLOW;
        } else if (t_frame_begin < self.m_t_success + chrono::duration_cast<clock_type_t::duration>(chrono::duration<double, std::milli>(250.0))) {
            graphics_node_data->c = GREEN;
        } else if (t_frame_begin < self.m_t_failure + chrono::duration_cast<clock_type_t::duration>(chrono::duration<double, std::milli>(250.0))) {
            graphics_node_data->c = RED;
        } else {
            graphics_node_data->c = BLUE;
        }

        DrawCircleV(graphics_node_data->p, graphics_node_data->r, graphics_node_data->c);

        if (graphics_node_data->selected) {
            Vector2 vmp = screen_to_world(graphics_engine_node_data->mp, graphics_engine_node_data->camera);
            // Vector2 cp = world_to_screen(graphics_node_data->p);
            Vector2 dir = { vmp.x - graphics_node_data->p.x, vmp.y - graphics_node_data->p.y };
            float s = sqrtf(dir.x * dir.x + dir.y * dir.y);
            dir.x /= s;
            dir.y /= s;

            DrawLineV(
                Vector2{ graphics_node_data->p.x + dir.x * graphics_node_data->r, graphics_node_data->p.y + dir.y * graphics_node_data->r },
                Vector2{ vmp.x, vmp.y },
                ORANGE
            );
        }

        draw_text(
            self.describe(),
            graphics_node_data->p,
            BLACK,
            Vector2{ graphics_node_data->r / sqrtf(2.0f), graphics_node_data->r / sqrtf(2.0f) },
            graphics_engine_node_data->font_size,
            graphics_engine_node_data->font_spacing
        );

        const auto& inputs = self.read_inputs();
        auto input_pair_it = inputs.begin();
        while (input_pair_it != inputs.end()) {
            graphics_node_data_t* gfx_node_graphics_node_data = input_pair_it->first->write_state<graphics_node_data_t>();
            if (!gfx_node_graphics_node_data) {
                ++input_pair_it;
                continue ;
            }

            Vector2 dir = { gfx_node_graphics_node_data->p.x - graphics_node_data->p.x, gfx_node_graphics_node_data->p.y - graphics_node_data->p.y };
            float dirs = sqrtf(dir.x * dir.x + dir.y * dir.y);
            dir.x /= dirs;
            dir.y /= dirs;

            float angle = 3.1415f / 2.0f;
            Vector2 r_dir = {
                dir.x * cos(angle) - dir.y * sin(angle),
                dir.x * sin(angle) + dir.y * cos(angle)
            };
            float r_dirs = sqrtf(r_dir.x * r_dir.x + r_dir.y * r_dir.y);
            r_dir.x /= r_dirs;
            r_dir.y /= r_dirs;
            
            Vector2 shared_circle_p = Vector2{ graphics_node_data->p.x + (gfx_node_graphics_node_data->p.x - graphics_node_data->p.x) / 2.0f, graphics_node_data->p.y + (gfx_node_graphics_node_data->p.y - graphics_node_data->p.y) / 2.0f };
            Color shared_circle_c = MAGENTA;
            float shared_circle_r = dirs / 25.0f;

            DrawLineV(
                Vector2{ graphics_node_data->p.x + dir.x * graphics_node_data->r, graphics_node_data->p.y + dir.y * graphics_node_data->r },
                Vector2{ gfx_node_graphics_node_data->p.x - dir.x * gfx_node_graphics_node_data->r, gfx_node_graphics_node_data->p.y - dir.y * gfx_node_graphics_node_data->r },
                YELLOW
            );

            float base_half = dirs / 10.0f;
            Vector2 tr_a = { shared_circle_p.x + r_dir.x * base_half, shared_circle_p.y + r_dir.y * base_half };
            Vector2 tr_b = { shared_circle_p.x - r_dir.x * base_half, shared_circle_p.y - r_dir.y * base_half };
            Vector2 tr_c = { shared_circle_p.x - dir.x * base_half * 1.5f, shared_circle_p.y - dir.y * base_half * 1.5f };
            DrawLineV(
                tr_a,
                tr_c,
                BLUE
            );
            DrawLineV(
                tr_b,
                tr_c,
                BLUE
            );

            if (input_pair_it->first->m_t_failure < input_pair_it->first->m_t_success) {
                shared_circle_c = GREEN;
            } else if (input_pair_it->first->m_t_success < input_pair_it->first->m_t_failure) {
                shared_circle_c = RED;
            } else {
                shared_circle_c = WHITE;
            }
            DrawCircleV(
                shared_circle_p,
                shared_circle_r,
                shared_circle_c
            );
            draw_text(
                to_string(input_pair_it->second),
                shared_circle_p,
                BLACK,
                Vector2{ graphics_node_data->r / sqrtf(2.0f), graphics_node_data->r / sqrtf(2.0f) },
                graphics_engine_node_data->font_size,
                graphics_engine_node_data->font_spacing
            );

            ++input_pair_it;
        }
    });
}

graphics_node_t::~graphics_node_t() {
    remove_state<graphics_node_data_t>();
}

struct double_data_t {
    double d;
};

constant_node_t::constant_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();
            double_data->d = 42.0;

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "constant\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

sum_node_t::sum_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();
            double_data->d = 0.0;

            const auto& inputs = self.read_inputs();
            for (const auto& input_pair : inputs) {
                const double_data_t* input_double_data = input_pair.first->read_state<double_data_t>();
                if (input_double_data) {
                    double_data->d += input_double_data->d * input_pair.second;
                }
            }
            double_data->d = sin(double_data->d);

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "sum\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

accumulate_node_t::accumulate_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();

            const auto& inputs = self.read_inputs();
            for (const auto& input_pair : inputs) {
                const double_data_t* input_double_data = input_pair.first->read_state<double_data_t>();
                if (input_double_data) {
                    double_data->d += input_double_data->d * input_pair.second;
                }
            }

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "accumulate\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

decrement_node_t::decrement_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();
            if (double_data->d <= 0.0) {
                return false;
            }

            double_data->d -= 1.0;

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "decrement\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

struct time_node_data_t {
    time_type_t t;
};

time_node_t::time_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            time_node_data_t* time_node_data = self.add_state<time_node_data_t>();
            time_node_data->t = get_time();

            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = chrono::duration<float>(time_node_data->t - get_time_init()).count();

            return true;
        },
        [](node_t& self) -> bool {
            time_node_data_t* time_node_data = self.write_state<time_node_data_t>();
            time_node_data->t = get_time();

            double_data_t* double_data = self.write_state<double_data_t>();
            double_data->d = chrono::duration<float>(time_node_data->t - get_time_init()).count();

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "time [s]\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<time_node_data_t>();
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

sin_node_t::sin_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();
            double_data->d = 0.0;

            const auto& inputs = self.read_inputs();
            for (const auto& input_pair : inputs) {
                const double_data_t* input_double_data = input_pair.first->read_state<double_data_t>();
                if (input_double_data) {
                    double_data->d += input_double_data->d * input_pair.second;
                }
            }
            double_data->d = sin(double_data->d);

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "sin\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

mul_node_t::mul_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();
            double_data->d = 1.0;

            const auto& inputs = self.read_inputs();
            for (const auto& input_pair : inputs) {
                const double_data_t* input_double_data = input_pair.first->read_state<double_data_t>();
                if (input_double_data) {
                    double_data->d *= input_double_data->d * input_pair.second;
                }
            }

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "mul\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

floor_node_t::floor_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            double_data_t* double_data = self.add_state<double_data_t>();
            double_data->d = 0.0f;
            return true;
        },
        [](node_t& self) -> bool {
            double_data_t* double_data = self.write_state<double_data_t>();
            double_data->d = 0.0;

            const auto& inputs = self.read_inputs();
            for (const auto& input_pair : inputs) {
                const double_data_t* input_double_data = input_pair.first->read_state<double_data_t>();
                if (input_double_data) {
                    double_data->d += input_double_data->d * input_pair.second;
                }
            }
            double_data->d = floor(double_data->d);

            return true;
        },
        [](const node_t& self) -> string {
            const double_data_t* double_data = self.read_state<double_data_t>();
            return "floor\n" + to_string(double_data->d);
        },
        [](node_t& self) -> void {
            self.remove_state<double_data_t>();
        },
        0.0f, 0.0f
    )
{
}

struct sine_wave_node_data_t {
    double frequency;
    double amplitude;
    double phase;
};

sine_wave_node_t::sine_wave_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            sine_wave_node_data_t* sine_wave_node_data = self.add_state<sine_wave_node_data_t>();
            return true;
        },
        [](node_t& self) -> bool {
            sine_wave_node_data_t* sine_wave_node_data = self.write_state<sine_wave_node_data_t>();

            // how do I even get this number.. well, I need something convertible to double_package_t::double_t I think

            const auto& inputs = self.read_inputs();
            if (0 < inputs.size()) {
                const double_data_t* double_data = inputs[0]->read_state<double_data_t>();
                if (double_data) {
                    sine_wave_node_data->frequency = double_data->d;
                } else {
                    sine_wave_node_data->frequency = 440.0;
                }
            } else {
                sine_wave_node_data->frequency = 440.0;
            }

            if (1 < inputs.size()) {
                const double_data_t* double_data = inputs[0]->read_state<double_data_t>();
                if (double_data) {
                    sine_wave_node_data->amplitude = double_data->d;
                } else {
                    sine_wave_node_data->amplitude = 0.5;
                }
            } else {
                sine_wave_node_data->amplitude = 0.5;
            }

            if (2 < inputs.size()) {
                const double_data_t* double_data = inputs[0]->read_state<double_data_t>();
                if (double_data) {
                    sine_wave_node_data->phase = double_data->d;
                } else {
                    sine_wave_node_data->phase = 0.0;
                }
            } else {
                sine_wave_node_data->phase = 0.0;
            }

            return true;
        },
        [](const node_t& self) -> string {
            const sine_wave_node_data_t* sine_wave_node_data = self.read_state<sine_wave_node_data_t>();
            return "frequency [Hz]: " + to_string(sine_wave_node_data->frequency) + ", amplitude: " + to_string(sine_wave_node_data->amplitude) + ", phase [rad]: " + to_string(sine_wave_node_data->phase);
        },
        [](node_t& self) -> void {
            self.remove_state<sine_wave_node_data_t>();
        },
        0.0f, 0.0f
    )
{
}

struct audio_mixer_node_data_t {
    PaStream* stream;
    PaHostApiIndex host_api_index;
    const PaHostApiInfo* host_api_info;
    const PaDeviceInfo* device_info;
    size_t buffer_write;
    size_t buffer_read; // ensure only 1 consumer advances this
    float buffer[4096];
    thread mixer_thread;
    bool mixer_thread_is_running;
};

static bool check_pa_error(PaError err) {
    if (err == paNoError) {
        return false;
    }

    if (err == paUnanticipatedHostError) {
        const PaHostErrorInfo* host_error_info = Pa_GetLastHostErrorInfo();
        cerr << "Host error occured:" << endl;
        cerr << "  host API type: " << host_error_info->hostApiType << endl;
        cerr << "  error code: " << host_error_info->errorCode << endl;
        cerr << "  text: " << host_error_info->errorText << endl;
    } else {
        cerr << "Error occured: " << Pa_GetErrorText(err) << " (" << err << ")" << endl;
    }

    return true;
}

static int n_audio_mixer_nodes = 0;
audio_mixer_node_t::audio_mixer_node_t():
    graphics_node_t(
        [](node_t& self) -> bool {
            audio_mixer_node_data_t* audio_mixer_node_data = self.add_state<audio_mixer_node_data_t>();
            if (n_audio_mixer_nodes == 0) {
                if (check_pa_error(Pa_Initialize())) {
                    return false;
                }
            }
            audio_mixer_node_data->stream = 0;
            audio_mixer_node_data->host_api_index = Pa_GetDefaultHostApi();
            audio_mixer_node_data->host_api_info = Pa_GetHostApiInfo(audio_mixer_node_data->host_api_index);
            audio_mixer_node_data->device_info = Pa_GetDeviceInfo(audio_mixer_node_data->host_api_info->defaultOutputDevice);
            audio_mixer_node_data->buffer_write = 0;
            audio_mixer_node_data->buffer_read = 0;
            memset(audio_mixer_node_data->buffer, 0, sizeof(audio_mixer_node_data->buffer));
            audio_mixer_node_data->mixer_thread_is_running = false;

            PaStreamParameters stream_parameters = {
                .device = audio_mixer_node_data->host_api_info->defaultOutputDevice,
                .channelCount = audio_mixer_node_data->device_info->maxOutputChannels,
                .sampleFormat = paFloat32,
                .suggestedLatency = audio_mixer_node_data->device_info->defaultLowOutputLatency,
                .hostApiSpecificStreamInfo = 0
            };

            if (check_pa_error(Pa_OpenStream(
                &audio_mixer_node_data->stream,
                0,
                &stream_parameters,
                audio_mixer_node_data->device_info->defaultSampleRate,
                paFramesPerBufferUnspecified,
                paNoFlag,
                [](
                    const void* input,
                    void* output,
                    unsigned long frame_count,
                    const PaStreamCallbackTimeInfo* time_info,
                    PaStreamCallbackFlags status_flags,
                    void* user_data
                ) -> int {
                    audio_mixer_node_data_t* audio_mixer_node_data = (audio_mixer_node_data_t*) user_data;

                    // LOG("  CPU load to maintain real-time operations [%]: " << Pa_GetStreamCpuLoad(callback_data->stream) * 100.0);

                    // cout << "  Input: " << input << endl;
                    // cout << "  Output: " << output << endl;

                    // cout << "  Frame count: " << frame_count << endl;

                    // cout << "  Time of invocation: " << time_info->currentTime << endl;
                    // cout << "  Time of first ADC sample: " << time_info->inputBufferAdcTime << endl;
                    // cout << "  Expected time of first DAC sample: " << time_info->outputBufferDacTime << endl;

                    // cout << "  Status flags:" << endl;
                    // cout << "    underflow: ";
                    // if (paOutputUnderflow & status_flags) {
                    //     cout << "output data (or a gap) was inserted, possibly stream is using too much CPU time";
                    // } else {
                    //     cout << "output data (or a gap) wasn't inserted, callback is doing fine";
                    // }
                    // cout << endl;

                    // cout << "    overflow: ";
                    // if (paOutputOverflow & status_flags) {
                    //     cout << "output data will be discarded, because no room is available";
                    // } else {
                    //     cout << "output data won't be discard, there is enough room";
                    // }
                    // cout << endl;

                    // cout << "    priming output: ";
                    // if (paPrimingOutput & status_flags) {
                    //     cout << "some of all of the output data will be used to prime the stream, input data may be zero";
                    // } else {
                    //     cout << "none of the output data will be used to prime the stream";
                    // }
                    // cout << endl;

                    // cout << "  User_data: " << user_data << endl;

                    // paAbort - stream will finish asap
                    // paComplete - stream will finish once all buffers generated are played
                    // these are not necessary tho, as Pa_StopStream(), Pa_AbortStream() or Pa_CloseStream() can also be used

                    {
                        size_t available_to_read = 0;
                        size_t write_p = audio_mixer_node_data->buffer_write;
                        size_t read_p = audio_mixer_node_data->buffer_read;
                        if (read_p <= write_p) {
                            available_to_read = write_p - read_p;
                        } else {
                            available_to_read = (ARRAY_SIZE(audio_mixer_node_data->buffer) + write_p - read_p) % ARRAY_SIZE(audio_mixer_node_data->buffer);
                        }
                        // LOG("[CALLBACK] before, write: " << write_p << ", read: " << read_p << ", available to read: " << available_to_read);
                        bool write_zeros = available_to_read < frame_count;

                        assert(output);
                        float* buffer = (float*) output;
                        // interleaved data: 1st sample: [channel0 channel1 ..], 2nd sample: [channel0 channel1 ..], ...
                        if (write_zeros) {
                            for (size_t i = 0; i < frame_count; ++i) {
                                for (size_t channel_index = 0; channel_index < audio_mixer_node_data->device_info->maxOutputChannels; ++channel_index) {
                                    *buffer++ = 0.0f;
                                }
                            }
                        } else {
                            for (size_t i = 0; i < frame_count; ++i) {
                                float read_val = audio_mixer_node_data->buffer[audio_mixer_node_data->buffer_read++];
                                if (ARRAY_SIZE(audio_mixer_node_data->buffer) <= audio_mixer_node_data->buffer_read) {
                                    audio_mixer_node_data->buffer_read = 0;
                                }
                                for (size_t channel_index = 0; channel_index < audio_mixer_node_data->device_info->maxOutputChannels; ++channel_index) {
                                    *buffer++ = read_val;
                                }
                            }
                        }

                        // write_p = audio_mixer_node_data->buffer_write;
                        // read_p = audio_mixer_node_data->buffer_read;
                        // if (read_p <= write_p) {
                        //     available_to_read = write_p - read_p;
                        // } else {
                        //     available_to_read = (ARRAY_SIZE(audio_mixer_node_data->buffer) + write_p - read_p) % ARRAY_SIZE(audio_mixer_node_data->buffer);
                        // }
                        // LOG("[CALLBACK] after, write: " << write_p << ", read: " << read_p << ", available to read: " << available_to_read);
                    }

                    return paContinue;
                },
                (void*) audio_mixer_node_data
            ))) {
                return false;
            }
            if (check_pa_error(Pa_StartStream(audio_mixer_node_data->stream))) {
                return false;
            }

            audio_mixer_node_data->mixer_thread_is_running = true;
            audio_mixer_node_data->mixer_thread = thread([](node_t* node) {
                audio_mixer_node_data_t* audio_mixer_node_data = node->write_state<audio_mixer_node_data_t>();
                double t = 0.0;
                const double time_step = 1.0f / audio_mixer_node_data->device_info->defaultSampleRate;

                while (audio_mixer_node_data->mixer_thread_is_running) {
                    size_t available_to_write = 0;
                    size_t write_p = audio_mixer_node_data->buffer_write;
                    size_t read_p = audio_mixer_node_data->buffer_read;
                    if (read_p <= write_p) {
                        available_to_write = (ARRAY_SIZE(audio_mixer_node_data->buffer) - (write_p - read_p)) - 1;
                    } else {
                        available_to_write = (read_p - write_p) - 1;
                    }
                    // LOG("[MIXER] before, write: " << write_p << ", read: " << read_p << ", available_to_write: " << available_to_write);

                    if (!available_to_write) {
                        Pa_Sleep(1);
                        continue ;
                    }

                    {
                        // first pass to get informations on the resulting wave
                        constexpr float desired_min_mixed_value = -1.0f;
                        constexpr float desired_max_mixed_value = 1.0;
                        float min_mixed_value = FLT_MAX;
                        float max_mixed_value = -FLT_MAX;
                        double tmp_t = t;

                        const vector<node_t*>& inputs = node->read_inputs();
                        for (size_t i = 0; i < available_to_write; ++i) {
                            float mixed_value = 0.0f;
                            for (size_t sine_wave_index = 0; sine_wave_index < inputs.size(); ++sine_wave_index) {
                                const sine_wave_node_data_t* sine_wave_node_data = inputs[i]->read_state<sine_wave_node_data_t>();
                                if (sine_wave_node_data) {
                                    mixed_value += (float) (sine_wave_node_data->amplitude * sin(2.0 * M_PI * sine_wave_node_data->frequency * tmp_t + sine_wave_node_data->phase));
                                }
                            }
                            min_mixed_value = min(min_mixed_value, mixed_value);
                            max_mixed_value = max(max_mixed_value, mixed_value);
                            tmp_t += time_step;
                        }

                        // second pass to commit the changes
                        float mixed_value_multiplier = 1.0f;
                        if (min_mixed_value < desired_min_mixed_value) {
                            mixed_value_multiplier = desired_min_mixed_value / min_mixed_value;
                        }
                        if (desired_max_mixed_value < max_mixed_value) {
                            mixed_value_multiplier = min(mixed_value_multiplier, desired_max_mixed_value / max_mixed_value);
                        }
                        for (size_t i = 0; i < available_to_write; ++i) {
                            float mixed_value = 0.0f;
                            for (size_t sine_wave_index = 0; sine_wave_index < inputs.size(); ++sine_wave_index) {
                                const sine_wave_node_data_t* sine_wave_node_data = inputs[i]->read_state<sine_wave_node_data_t>();
                                if (sine_wave_node_data) {
                                    mixed_value += (float) (sine_wave_node_data->amplitude * sin(2.0 * M_PI * sine_wave_node_data->frequency * t + sine_wave_node_data->phase));
                                }
                            }
                            audio_mixer_node_data->buffer[audio_mixer_node_data->buffer_write++] = mixed_value * mixed_value_multiplier;
                            if (ARRAY_SIZE(audio_mixer_node_data->buffer) <= audio_mixer_node_data->buffer_write) {
                                audio_mixer_node_data->buffer_write = 0;
                            }
                            t += time_step;
                        }
                    }

                    write_p = audio_mixer_node_data->buffer_write;
                    read_p = audio_mixer_node_data->buffer_read;
                    if (read_p <= write_p) {
                        available_to_write = (ARRAY_SIZE(audio_mixer_node_data->buffer) - (write_p - read_p)) - 1;
                    } else {
                        available_to_write = (read_p - write_p) - 1;
                    }
                    // LOG("[MIXER] after, acquire, write: " << write_p << ", read: " << read_p << ", available_to_write: " << available_to_write);
                }
            }, &self);

            return true;
        },
        [](node_t& self) -> bool {
            return true;
        },
        [](const node_t& self) -> string {
            const audio_mixer_node_data_t* audio_mixer_node_data = self.read_state<audio_mixer_node_data_t>();

            // kind of useless as it changes too fast..
            return "audio mixer\n" + to_string(audio_mixer_node_data->buffer[audio_mixer_node_data->buffer_read]);
        },
        [](node_t& self) -> void {
            audio_mixer_node_data_t* audio_mixer_node_data = self.write_state<audio_mixer_node_data_t>();

            if (audio_mixer_node_data->mixer_thread_is_running) {
                audio_mixer_node_data->mixer_thread_is_running = false;
                audio_mixer_node_data->mixer_thread.join();
            }

            if (--n_audio_mixer_nodes == 0) {
                (void) check_pa_error(Pa_Terminate());
            }

            self.remove_state<audio_mixer_node_data_t>();
        },
        0.0f, 0.0f
    )
{
}
