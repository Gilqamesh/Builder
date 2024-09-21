#include "raylib.h"
#include "node.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <iomanip>
#include <thread>
#include <unordered_map>
#include <cassert>
#include <unordered_set>

#define LINE() cout << __LINE__ << endl

#define MAX_SAMPLES_PER_UPDATE 4096

struct graphics_node_t;

int w_width = 1800;
int w_height = 1000;

float font_size = 20.0f;
float font_spacing = 1.0f;

Camera2D camera;
Vector2 mp;

vector<graphics_node_t*> nodes;
unordered_set<node_t*> time_nodes;

time_type_t t_frame_begin;

bool have_audio = false;
bool should_update_thread_run = true;
bool should_update = false;
thread update_thread;

// todo: add zoom
Vector2 world_to_screen(Vector2 v) {
    v.x += camera.offset.x;
    v.y += camera.offset.y;

    return v;
}

// todo: add zoom
Vector2 screen_to_world(Vector2 v) {
    v.x -= camera.offset.x;
    v.y -= camera.offset.y;

    return v;
}

void draw_text(const string& str, Vector2 middle, Color color, Vector2 max_dims) {
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

struct event_t {
    time_type_t t;
    void* context;
    std::function<void(void*)> f; // maybe pass weak pointer

    bool operator<(const event_t& other) const {
        return t < other.t;
    }
};
priority_queue<event_t> event_queue;

void add_event(double ms_from_now, void* context, const std::function<void(void*)>& f) {
    event_t event;

    event.t = t_frame_begin + chrono::duration_cast<clock_type_t::duration>(chrono::duration<double, std::milli>(ms_from_now));
    event.f = f;
    event.context = context;
    
    event_queue.push(move(event));
}

unordered_map<string, function<graphics_node_t*()>> generator;

struct graphics_node_t : public node_t {
    Vector2 m_p;
    float m_r;
    Color m_c;
    bool m_selected;
    bool m_dragged;
public:
    graphics_node_t(
        const function<void(node_t&)>& init_fn,
        function<bool(const vector<node_t*>&, memory_slice_t&)>&& run_fn,
        function<void(const memory_slice_t&, string&)>&& describe_fn,
        function<void(node_t&)>&& destructor_fn,
        Vector2 p
    ):
        node_t(init_fn, move(run_fn), move(describe_fn), move(destructor_fn))
    {
        nodes.push_back(this);

        m_selected = false;
        m_dragged = false;
        m_p = p;
        m_r = 100.0f;
        m_c = BLUE;
    }

    ~graphics_node_t() {
        bool deleted = false;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i] == this) {
                while (i < nodes.size() - 1) {
                    nodes[i] = nodes[i + 1];
                    ++i;
                }
                nodes.pop_back();
                deleted = true;
                break ;
            }
        }
        assert(deleted);
    }

    bool is_hovered() {
        Vector2 np = world_to_screen(m_p);
        float dx = mp.x - np.x;
        float dy = mp.y - np.y;
        return dx * dx + dy * dy < m_r * m_r;
    }

    void draw() {
        if (m_dragged) {
            m_p = screen_to_world(mp);
        }

        if (m_selected) {
            m_c = YELLOW;
        } else if (t_frame_begin < m_t_success + chrono::duration_cast<clock_type_t::duration>(chrono::duration<double, std::milli>(250.0))) {
            m_c = GREEN;
        } else if (t_frame_begin < m_t_failure + chrono::duration_cast<clock_type_t::duration>(chrono::duration<double, std::milli>(250.0))) {
            m_c = RED;
        } else {
            m_c = BLUE;
        }

        DrawCircleV(m_p, m_r, m_c);

        if (m_selected) {
            Vector2 vmp = screen_to_world(mp);
            // Vector2 cp = world_to_screen(m_p);
            Vector2 dir = { vmp.x - m_p.x, vmp.y - m_p.y };
            float s = sqrtf(dir.x * dir.x + dir.y * dir.y);
            dir.x /= s;
            dir.y /= s;

            DrawLineV(
                Vector2{ m_p.x + dir.x * m_r, m_p.y + dir.y * m_r },
                Vector2{ vmp.x, vmp.y },
                ORANGE
            );
        }

        string str;
        describe(str);
        draw_text(str, m_p, BLACK, Vector2{ m_r / sqrtf(2.0f), m_r / sqrtf(2.0f) });

        for (int i = 0; i < m_inputs.size(); ++i) {
            size_t n_input_occurance = 0;
            for (node_t* input : m_inputs) {
                if (input == m_inputs[i]) {
                    ++n_input_occurance;
                }
            }
            graphics_node_t* gfx_node = (graphics_node_t*) m_inputs[i];
            Vector2 dir = { gfx_node->m_p.x - m_p.x, gfx_node->m_p.y - m_p.y };
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
            
            Vector2 shared_circle_p = Vector2{ m_p.x + (gfx_node->m_p.x - m_p.x) / 2.0f, m_p.y + (gfx_node->m_p.y - m_p.y) / 2.0f };
            Color shared_circle_c = MAGENTA;
            float shared_circle_r = dirs / 25.0f;

            DrawLineV(
                Vector2{ m_p.x + dir.x * m_r, m_p.y + dir.y * m_r },
                Vector2{ gfx_node->m_p.x - dir.x * gfx_node->m_r, gfx_node->m_p.y - dir.y * gfx_node->m_r },
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

            if (gfx_node->m_t_failure < gfx_node->m_t_success) {
                shared_circle_c = GREEN;
            } else if (gfx_node->m_t_success < gfx_node->m_t_failure) {
                shared_circle_c = RED;
            } else {
                shared_circle_c = WHITE;
            }
            DrawCircleV(
                shared_circle_p,
                shared_circle_r,
                shared_circle_c
            );
            draw_text(to_string(n_input_occurance), shared_circle_p, BLACK, Vector2{ m_r / sqrtf(2.0f), m_r / sqrtf(2.0f) });
        }
    }
};

graphics_node_t* add_const_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 0;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            *(float*) result.memory = 42;

            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "constant\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { 200, 300 }
    );
}

graphics_node_t* add_sum_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 0;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            *(float*) result.memory = 0;
            for (node_t* input : inputs) {
                memory_slice_t& sub_result = input->write();
                if (sub_result.memory && sizeof(float) <= sub_result.size) {
                    *(float*) result.memory += *(float*) sub_result.memory;
                }
            }
            
            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "sum\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { 200, -300 }
    );
}

graphics_node_t* add_acc_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 0;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            for (node_t* input : inputs) {
                memory_slice_t& sub_result = input->write();
                if (sub_result.memory && sizeof(float) <= sub_result.size) {
                    *(float*) result.memory += *(float*) sub_result.memory;
                }
            }
            
            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "accumulate\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { -200, -300 }
    );
}

graphics_node_t* add_dec_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 100;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            if (*(float*) result.memory <= 0.0f) {
                return false;
            }

            *(float*) result.memory -= 1.0f;

            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "decrement\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { -200, 300 }
    );
}

graphics_node_t* add_audio_node() {
    struct audio_data_t {
        float frequency;
        function<void(void*)>* update_fn;
        AudioStream stream;
        float audio_frequency; // for smoothing apparently
        int sample_rate;
        float x;
        short data[MAX_SAMPLES_PER_UPDATE];
        short write_buffer[MAX_SAMPLES_PER_UPDATE];
        int read_cursor;
        bool is_playing;
    };

    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(audio_data_t);
            result.memory = malloc(sizeof(audio_data_t));
            
            audio_data_t* audio_data = (audio_data_t*) result.memory;
            audio_data->frequency = 440.0f;
            audio_data->read_cursor = 0;
            audio_data->update_fn = new function<void(void*)>([](void* context) {
                audio_data_t* audio_data = (audio_data_t*) context;

                if (!audio_data->is_playing) {
                    audio_data->is_playing = true;
                    PlayAudioStream(audio_data->stream);
                }

                if (IsAudioStreamProcessed(audio_data->stream)) {
                    audio_data->audio_frequency = audio_data->frequency + (audio_data->audio_frequency - audio_data->frequency) * 0.95f;

                    int write_cursor = 0;
                    int wave_length = (int) (22050 / audio_data->frequency);
                    while (write_cursor < MAX_SAMPLES_PER_UPDATE) {
                        int write_length = MAX_SAMPLES_PER_UPDATE - write_cursor;
                        int read_length = wave_length - audio_data->read_cursor;
                        write_length = min(read_length, write_length);

                        memcpy(audio_data->write_buffer + write_cursor, audio_data->data + audio_data->read_cursor, write_length * sizeof(short));

                        audio_data->read_cursor = (audio_data->read_cursor + write_length) % wave_length;

                        write_cursor += write_length;
                    }

                    UpdateAudioStream(audio_data->stream, audio_data->write_buffer, MAX_SAMPLES_PER_UPDATE);
                }

                add_event(100.0f, (void*) audio_data, *audio_data->update_fn);
            });
            audio_data->audio_frequency = 440.0f;
            audio_data->sample_rate = 44100;
            audio_data->x = 0.0f;
            audio_data->stream = LoadAudioStream(audio_data->sample_rate, 16, 1);
            audio_data->is_playing = false;

            const float increment = audio_data->audio_frequency / audio_data->sample_rate;
            for (size_t i = 0; i < sizeof(audio_data->data) / sizeof(audio_data->data[0]); i++) {
                ((short*) audio_data->data)[i] = (short) (32000.0f * sinf(2 * PI * audio_data->x));
                audio_data->x += increment;
                if (1.0f < audio_data->x) {
                    audio_data->x -= 1.0f;
                }
            }
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            audio_data_t* audio_data = (audio_data_t*) result.memory;
            if (inputs.empty()) {
                audio_data->frequency = 440;
            } else {
                audio_data->frequency = 0;
                for (node_t* input : inputs) {
                    memory_slice_t& sub_result = input->write();
                    if (sub_result.memory && sizeof(float) <= sub_result.size) {
                        audio_data->frequency += *(float*) sub_result.memory;
                    }
                }
            }


            if (!audio_data->is_playing) {
                assert(!IsAudioStreamPlaying(audio_data->stream));
                add_event(0, (void*) audio_data, *audio_data->update_fn);
            }

            return true;
        },
        [](const memory_slice_t& result, string& str) {
            audio_data_t* audio_data = (audio_data_t*) result.memory;
            str = "audio\n" + to_string(audio_data->frequency) + "\nplaying: " + (IsAudioStreamPlaying(audio_data->stream) ? "yes" : "no");
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            audio_data_t* audio_data = (audio_data_t*) result.memory;
            if (IsAudioStreamPlaying(audio_data->stream)) {
                StopAudioStream(audio_data->stream);
            }
            UnloadAudioStream(audio_data->stream);
            free(result.memory);
        },
        { -250, 0 }
    );
}

graphics_node_t* add_time_node() {
    struct time_node_data_t {
        float f;
        time_type_t t;
    };
    graphics_node_t* result = new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(time_node_data_t);
            result.memory = malloc(sizeof(time_node_data_t));
            time_node_data_t* time_node_data = (time_node_data_t*) result.memory;
            time_node_data->t = get_time();
            time_node_data->f = chrono::duration<float>(time_node_data->t - get_time_init()).count();
            time_nodes.insert(&self);
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            time_node_data_t* time_node_data = (time_node_data_t*) result.memory;
            time_node_data->t = get_time();
            time_node_data->f = chrono::duration<float>(time_node_data->t - get_time_init()).count();

            return true;
        },
        [](const memory_slice_t& result, string& str) {
            time_node_data_t* time_node_data = (time_node_data_t*) result.memory;
            str = "time [s]\n" + to_string(time_node_data->f);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            time_nodes.erase(&self);
            free(result.memory);
        },
        { -800, 0 }
    );
    return result;
}

graphics_node_t* add_sin_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 0.0f;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            *(float*) result.memory = 0.0f;
            for (node_t* input : inputs) {
                memory_slice_t& sub_result = input->write();
                if (sub_result.memory && sizeof(float) <= sub_result.size) {
                    *(float*) result.memory += *(float*) sub_result.memory;
                }
            }
            *(float*) result.memory = sinf(*(float*) result.memory);

            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "sin\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { -550, 0 }
    );
}

graphics_node_t* add_mul_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 1.0f;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            *(float*) result.memory = 1.0f;
            for (node_t* input : inputs) {
                memory_slice_t& sub_result = input->write();
                if (sub_result.memory && sizeof(float) <= sub_result.size) {
                    *(float*) result.memory *= *(float*) sub_result.memory;
                }
            }

            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "mul\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { -550, 300 }
    );
}

graphics_node_t* add_floor_node() {
    return new graphics_node_t(
        [](node_t& self) {
            memory_slice_t& result = self.write();
            result.size = sizeof(float);
            result.memory = malloc(sizeof(float));
            *(float*) result.memory = 0;
        },
        [](const vector<node_t*>& inputs, memory_slice_t& result) {
            *(float*) result.memory = 0;
            for (node_t* input : inputs) {
                memory_slice_t& sub_result = input->write();
                if (sub_result.memory && sizeof(float) <= sub_result.size) {
                    *(float*) result.memory += *(float*) sub_result.memory;
                }
            }
            *(float*) result.memory = floorf(*(float*) result.memory);
            
            return true;
        },
        [](const memory_slice_t& result, string& str) {
            str = "floor\n" + to_string(*(float*) result.memory);
        },
        [](node_t& self) {
            memory_slice_t& result = self.write();
            free(result.memory);
        },
        { 500, 0 }
    );
}

void update();

void init() {
    set_time_init();

    generator["const"] = &add_const_node;
    generator["sum"] = &add_sum_node;
    generator["acc"] = &add_acc_node;
    generator["dec"] = &add_dec_node;
    generator["audio"] = &add_audio_node;
    generator["time"] = &add_time_node;
    generator["sin"] = &add_sin_node;
    generator["mul"] = &add_mul_node;
    generator["floor"] = &add_floor_node;

    InitWindow(w_width, w_height, "The Window");

    InitAudioDevice();
    have_audio = IsAudioDeviceReady();
    if (have_audio) {
        SetAudioStreamBufferSizeDefault(MAX_SAMPLES_PER_UPDATE);
    }
    SetTargetFPS(60);

    camera.offset.x = w_width / 2.0f;
    camera.offset.y = w_height / 2.0f;
    camera.rotation = 0.0f;
    camera.target.x = 0.0f;
    camera.target.y = 0.0f;
    camera.zoom = 1.0f;

    should_update_thread_run = true;
    should_update = false;
    update_thread = thread([]() {
        while (should_update_thread_run) {
            if (should_update) {
                update();
                should_update = false;
            }
        }
    });
}

void update() {
    mp = GetMousePosition();

    time_type_t t_propagate = get_time();
    for (node_t* node : time_nodes) {
        node->run_force(t_propagate);
    }

    while (!event_queue.empty()) {
        const event_t& event = event_queue.top();
        if (t_frame_begin < event.t) {
            break ;
        }
        event_queue.pop();

        if (event.f) {
            cout << "Running callback for event.." << endl;
            event.f(event.context);
        } else {
            cout << "No callback for event!" << endl;
        }
    }

    bool resolved_selected = false;
    size_t node_index = 0;
    while (node_index < nodes.size()) {
        if (IsKeyPressed(KEY_Q)) {
            nodes[node_index]->m_selected = false;
        }

        if (nodes[node_index]->is_hovered()) {
            if (IsKeyPressed(KEY_DELETE)) {
                delete nodes[node_index];
                continue ;
            }

            if (IsKeyPressed(KEY_S) && !resolved_selected) {
                resolved_selected = true;
                for (size_t other_node_index = 0; other_node_index < nodes.size(); ++other_node_index) {
                    if (nodes[other_node_index]->m_selected) {
                        nodes[other_node_index]->m_selected = false;
                        nodes[node_index]->add_input(nodes[other_node_index]);
                    }
                }
            }

            if (IsKeyPressed(KEY_A)) {
                nodes[node_index]->m_selected = !nodes[node_index]->m_selected;
            }

            if (IsKeyPressed(KEY_D)) {
                nodes[node_index]->isolate();
            }
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                (void) nodes[node_index]->run_force(get_time());
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                nodes[node_index]->m_dragged = !nodes[node_index]->m_dragged;
            }
        }

        ++node_index;
    }
}

void draw_palette() {
    assert(w_width % 10 == 0);
    assert(w_height % 10 == 0);
    float rec_width = w_width / 10.0f;
    float rec_height = w_height / 10.0f;
    float rec_x = 0.0f;
    float rec_y = 0.0f;

    Rectangle rec_cur = { rec_x, rec_y, rec_width, rec_height };

    for (const auto& p : generator) {
        DrawRectangleLinesEx(rec_cur, 2.0f, RED);

        if (
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            rec_cur.x <= mp.x && mp.x <= rec_cur.x + rec_cur.width &&
            rec_cur.y <= mp.y && mp.y <= rec_cur.y + rec_cur.height
        ) {
            graphics_node_t* node = p.second();
            node->m_p = screen_to_world(mp);
            node->m_dragged = true;
        }

        draw_text(
            p.first,
            Vector2{ rec_cur.x + rec_cur.width / 2.0f, rec_cur.y + rec_cur.height / 2.0f },
            BLACK,
            Vector2{ rec_cur.width, rec_cur.height }
        );

        rec_cur.x += rec_width;
        if (w_width <= rec_cur.x) {
            rec_cur.x = 0;
            rec_cur.y += rec_height;
        }
    }
}

void draw() {
    draw_palette();
    BeginMode2D(camera);
    for (int i = 0; i < nodes.size(); ++i) {
        nodes[i]->draw();
    }
    EndMode2D();
}

int main(int argc, char** argv) {
    init();

    while (!WindowShouldClose()) {
        t_frame_begin = get_time();

        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        should_update = true;

        draw();

        EndDrawing();
    }
    should_update_thread_run = false;
    update_thread.join();

    if (have_audio) {
        CloseAudioDevice();
    }
    CloseWindow();

    return 0;
}
