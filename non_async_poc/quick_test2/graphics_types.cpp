#include "graphics_types.h"

#include "raylib.h"

#include <thread>
#include <iostream>

static bool is_initialized;
static type_t* graphics_engine;
type_t* drawable_type = 0;

void init_graphics_module() {
    if (!is_initialized) {
        drawable_type = new type_t(
            [](state_t& state) {
                return true;
            },
            [](state_t& state) {
            },
            [](instance_t& instance) {
                return true;
            }
        );
        drawable_type->add_abstract<function<void(framebuffer_t&)>>("draw");
        is_initialized = true;
        graphics_engine = new type_t(
            [](state_t& state) {
                state.add<int>("width", 1800);
                state.add<int>("height", 1000);

                InitWindow(state.read<int>("width"), state.read<int>("height"), "The Window");

                state.add<float>("font-size", 20.0f);
                state.add<float>("font-spacing", 1.0f);
                state.add<Camera2D>("camera", Camera2D{
                    .offset = {
                        .x = state.read<int>("width") / 2.0f,
                        .y = state.read<int>("height") / 2.0f
                    },
                    .target = {
                        .x = 0.0f,
                        .y = 0.0f
                    },
                    .rotation = 0.0f,
                    .zoom = 1.0f
                });
                framebuffer_t& framebuffer = state.add<framebuffer_t>("framebuffer");
                framebuffer.width = state.read<int>("width");
                framebuffer.height = state.read<int>("height");
                framebuffer.data = new uint32_t[framebuffer.width * framebuffer.height];
                state.add<time_type_t>("t-frame-begin");
                state.add<bool>("should-update-thread-run", true);
                state.add<bool>("should-update", false);
                state.add<thread>("update-thread", [](state_t& state) {
                    auto update_fn = graphics_engine->read<function<void(state_t&, double)>>("update");
                    assert(update_fn);
                    while (state.read<bool>("should-update-thread-run")) {
                        if (state.read<bool>("should-update")) {
                            time_type_t t = get_time();
                            auto dt = (t - state.read<time_type_t>("t-frame-begin")).count();
                            update_fn(state, dt);
                            state.write<bool>("should-update") = false;
                        }
                    }
                }, ref(state));

                return true;
            },
            [](state_t& state) {
                state.write<bool>("should-update-thread-run") = false;
                framebuffer_t& framebuffer = state.write<framebuffer_t>("framebuffer");
                delete[] framebuffer.data;
                state.write<thread>("update-thread").join();
                CloseWindow();
            },
            [](instance_t& instance) {
                auto draw_fn = graphics_engine->read<function<void(instance_t&)>>("draw");
                assert(draw_fn);
                while (!WindowShouldClose()) {
                    instance.write<time_type_t>("t-frame-begin") = get_time();
                    instance.write<bool>("should-update") = true;
                    BeginDrawing();
                    ClearBackground(LIGHTGRAY);
                    draw_fn(instance);
                    EndDrawing();
                }

                return true;
            }
        );
        graphics_engine->add<function<void(instance_t&)>>("draw", [](instance_t& instance) {
            framebuffer_t& framebuffer = instance.write<framebuffer_t>("framebuffer");
            for (const auto& p : instance.m_inputs) {
                if (auto draw_fn = p.first->find<function<void(framebuffer_t&, const instance_t&)>>("draw")) {
                    for (const auto& p2 : p.second) {
                        (*draw_fn)(framebuffer, *p2.first);
                    }
                }
            }
        });
        graphics_engine->add<function<void(state_t&, double)>>("update", [](state_t& state, double dt) {
            cout << "updating stuff, dt: " << dt << endl;
        });
    }
}

void deinit_graphics_module() {
    if (is_initialized) {
        is_initialized = false;
        delete graphics_engine;
    }
}

graphics_engine_t::graphics_engine_t():
    instance_t(graphics_engine)
{
}

