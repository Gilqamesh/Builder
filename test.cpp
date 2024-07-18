#include <iostream>
#include <chrono>
#include <string>
#include <unordered_set>
#include <vector>
#include <format>
#include <cassert>
#include <map>
#include <thread>

#include "raylib.h"

using clock_type_t = std::chrono::high_resolution_clock;
using time_type_t  = std::chrono::time_point<clock_type_t>;

static time_type_t t_init = std::chrono::high_resolution_clock::now();

struct obj_t {
    time_type_t m_time;
};

static int g_node_index = 1;

struct node_t {
    node_t() {
        m_index = g_node_index++;
    }

    int m_index;
    time_type_t m_time_run;
    time_type_t m_time_propogation;
    Rectangle m_rec;
    std::vector<node_t*> inputs;
    std::vector<node_t*> outputs;
    double m_time_to_run;

    void add_input(node_t* input) {
        inputs.push_back(input);
        input->outputs.push_back(this);
    }

    void propogate_time(time_type_t time_propogation) {
        if (time_propogation <= m_time_propogation) {
            return ;
        }
        m_time_propogation = time_propogation;
        
        for (node_t* output : outputs) {
            output->propogate_time(time_propogation);
        }
    }

    void run(time_type_t time_propogation) {
        // preamble
        propogate_time(time_propogation);

        for (node_t* input : inputs) {
            assert(!(m_time_propogation < input->m_time_propogation));
            if (input->m_time_propogation == m_time_propogation) {
                if (input->m_time_run < m_time_propogation) {
                    return ;
                }
            }
        }

        // run
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(m_time_to_run));
        m_time_run = std::chrono::high_resolution_clock::now();

        // postamble
        for (node_t* output : outputs) {
            output->run(time_propogation);
        }
    }

    void run() {
        run(std::chrono::high_resolution_clock::now());
    }

    Color time_to_color(time_type_t t) {
        Color color = {
            .a = 255
        };

        double t_since_init = std::chrono::duration_cast<std::chrono::duration<double>>(t - t_init).count();
        const double decimal_part = t_since_init - static_cast<size_t>(t_since_init);
        color.r = 150;
        color.g = static_cast<unsigned char>(decimal_part * 255.0f);
        color.b = static_cast<unsigned char>(decimal_part * 255.0f);

        return color;
    }

    void display() {
        DrawRectangleRec(
            Rectangle{
                .x = m_rec.x,
                .y = m_rec.y,
                .width = m_rec.width,
                .height = m_rec.height
            },
            time_to_color(m_time_run)
        );
        DrawRectangleRec(
            Rectangle{
                .x = m_rec.x,
                .y = m_rec.y + m_rec.height / 2.0f,
                .width = m_rec.width,
                .height = m_rec.height / 2.0f
            },
            time_to_color(m_time_propogation)
        );

        const float text_font_size = 10.0f;
        const float text_font_spacing = 1.0f;
        char text[256];
        snprintf(text, sizeof(text)/sizeof(text[0]), "%s", std::format("ran {}", m_time_run).c_str());
        Vector2 text_dims = MeasureTextEx(GetFontDefault(), text, text_font_size, text_font_spacing);
        DrawTextEx(
            GetFontDefault(),
            text,
            Vector2{
                .x = m_rec.x + m_rec.width / 2.0f - text_dims.x / 2.0f,
                .y = m_rec.y + m_rec.height / 4.0f - text_dims.y / 2.0f
            },
            text_font_size,
            text_font_spacing,
            BLACK
        );

        snprintf(text, sizeof(text)/sizeof(text[0]), "%s", std::format("propogated {}", m_time_propogation).c_str());
        text_dims = MeasureTextEx(GetFontDefault(), text, text_font_size, text_font_spacing);
        DrawTextEx(
            GetFontDefault(),
            text,
            Vector2{
                .x = m_rec.x + m_rec.width / 2.0f - text_dims.x / 2.0f,
                .y = m_rec.y + 3.0f * m_rec.height / 4.0f - text_dims.y / 2.0f
            },
            text_font_size,
            text_font_spacing,
            BLACK
        );

        snprintf(text, sizeof(text)/sizeof(text[0]), "time to run: %.2lfs", m_time_to_run);
        text_dims = MeasureTextEx(GetFontDefault(), text, text_font_size, text_font_spacing);
        DrawTextEx(
            GetFontDefault(),
            text,
            Vector2{
                .x = m_rec.x + m_rec.width / 2.0f - text_dims.x / 2.0f,
                .y = m_rec.y - m_rec.height / 4.0f + text_dims.y / 2.0f
            },
            text_font_size,
            text_font_spacing,
            BLACK
        );

        DrawRectangleLinesEx(m_rec, 2.0f, BLACK);
    }

    void print() {
        std::cout << m_index << " " << std::format("ran {}", m_time_run) << " " << std::format("propogated {}", m_time_propogation) << std::endl;
    }
};

struct prog1_t : node_t {
    prog1_t() {
        m_time_to_run = 0;
    }
};

struct prog2_t : node_t {
    prog2_t() {
        m_time_to_run = 0.1;
    }
};

struct prog3_t : node_t {
    prog3_t() {
        m_time_to_run = 2;
    }
};

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    
    float width = 1600;
    float height = 1200;

    std::vector<node_t*> nodes;
    for (int i = 0; i < 7; ++i) {
        if (i < 3) {
            nodes.push_back(new prog1_t());
        } else if (i < 6) {
            nodes.push_back(new prog2_t());
        } else {
            nodes.push_back(new prog3_t());
        }
    }

    nodes[1]->add_input(nodes[0]);
    nodes[2]->add_input(nodes[1]);
    nodes[3]->add_input(nodes[2]);
    nodes[4]->add_input(nodes[3]);
    nodes[6]->add_input(nodes[0]);
    nodes[5]->add_input(nodes[6]);
    nodes[4]->add_input(nodes[5]);

    Rectangle rec = {
        .width = width / 4.0f,
        .height = height / 8.0f
    };
    for (node_t* node : nodes) {
        node->m_rec = rec;
    }

    nodes[0]->m_rec.x = width / 2.0f - rec.width / 2.0f;
    nodes[0]->m_rec.y = height - rec.height - rec.height * 0.05f;

    const float row_increment = 1.5f * rec.height;

    nodes[1]->m_rec.x = width * 0.05f;
    nodes[1]->m_rec.y = nodes[0]->m_rec.y - row_increment;

    nodes[2]->m_rec.x = nodes[1]->m_rec.x;
    nodes[2]->m_rec.y = nodes[1]->m_rec.y - row_increment;

    nodes[3]->m_rec.x = nodes[2]->m_rec.x;
    nodes[3]->m_rec.y = nodes[2]->m_rec.y - row_increment;

    nodes[4]->m_rec.x = nodes[0]->m_rec.x;
    nodes[4]->m_rec.y = nodes[3]->m_rec.y - row_increment;

    nodes[5]->m_rec.x = width - rec.width - width * 0.05f;
    nodes[5]->m_rec.y = nodes[3]->m_rec.y;

    nodes[6]->m_rec.x = nodes[5]->m_rec.x;
    nodes[6]->m_rec.y = nodes[5]->m_rec.y + row_increment;

    bool is_window_closed = false;
    bool should_run_node = true;
    std::thread t([&nodes, &should_run_node, &is_window_closed]() {
        while (!is_window_closed) {
            if (should_run_node) {
                nodes[std::rand() % nodes.size()]->run();
                std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));
            }
        }
    });

    InitWindow(static_cast<int>(width), static_cast<int>(height), "Test time algo");
    while (!WindowShouldClose()) {
        // std::cout << "-------------------------------------------------------------------------------------------------" << std::endl;
        // std::map<time_type_t, node_t*> m;
        // for (node_t* node : nodes) {
        //     m[node->m_time_run] = node;
        // }
        // for (const auto& p : m) {
        //     p.second->print();
        // }
        // std::cout << "-------------------------------------------------------------------------------------------------" << std::endl;
        // std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(1));

        BeginDrawing();
        ClearBackground(WHITE);

        if (IsKeyPressed(KEY_P)) {
            should_run_node = !should_run_node;
        }

        for (node_t* node : nodes) {
            node->display();
        }

        EndDrawing();
    }

    CloseWindow();

    is_window_closed = true;

    t.join();

    return 0;
}
