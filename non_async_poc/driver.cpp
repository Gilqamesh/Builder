#include "raylib.h"
#include "node.h"
#include "graphics_node.h"

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

int w_width = 1800;
int w_height = 1000;

float font_size = 20.0f;
float font_spacing = 1.0f;

Camera2D camera;
Vector2 mp;

vector<graphics_node_t*> nodes;
unordered_set<node_t*> time_nodes;

time_type_t t_frame_begin;

// bool have_audio = false;
bool should_update_thread_run = true;
bool should_update = false;
thread update_thread;

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

    // InitAudioDevice();
    // have_audio = IsAudioDeviceReady();
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

    // if (have_audio) {
    //     CloseAudioDevice();
    // }
    CloseWindow();

    return 0;
}
