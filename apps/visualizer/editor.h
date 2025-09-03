#ifndef EDITOR_H
# define EDITOR_H

# include "libc.h"

class editor_t {
public:
    editor_t();

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

#endif // EDITOR_H
