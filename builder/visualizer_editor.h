#ifndef VISUALIZER_EDITOR_H
#define VISUALIZER_EDITOR_H

#include <functional>
#include <string>

class visualizer_editor_t {
public:
    visualizer_editor_t();

    void init();
    void deinit();

    void create_text_editor(std::function<void(std::string)> on_text_complete);
    void draw();

    bool open();
    bool is_captured_mouse();
    bool is_captured_keyboard();

private:
    char m_buffer[128];
    bool m_open;
    bool m_just_opened;
    std::function<void(std::string)> m_on_complete;
};

#endif // VISUALIZER_EDITOR_H
