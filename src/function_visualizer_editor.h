#ifndef FUNCTION_VISUALIZER_EDITOR_H
#define FUNCTION_VISUALIZER_EDITOR_H

#include <functional>
#include <string>

class function_visualizer_editor_t {
public:
    function_visualizer_editor_t();

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

#endif // FUNCTION_VISUALIZER_EDITOR_H
