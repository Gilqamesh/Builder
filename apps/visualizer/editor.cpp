#include "editor.h"

#include "rlImGui.h"
#include "imgui.h"

editor_t::editor_t():
    m_open(false),
    m_just_opened(false)
{
}

void editor_t::init() {
    rlImGuiSetup(true);
}

void editor_t::deinit() {
    rlImGuiShutdown();
}

void editor_t::create_text_editor(std::function<void(std::string)> on_text_complete) {
    if (m_open) {
        return ;
    }

    m_on_complete = std::move(on_text_complete);
    m_buffer[0] = '\0';
    m_open = true;
    m_just_opened = true;
}

void editor_t::draw() {
    if (!m_open) {
        return ;
    }

    if (ImGui::Begin("Text Editor", &m_open)) {
        ImGui::Text("Input:");
        ImGui::SameLine();
        
        if (m_just_opened) {
            m_just_opened = false;
            ImGui::SetKeyboardFocusHere();
        }

        const bool submitted = ImGui::InputText("##input", m_buffer, IM_ARRAYSIZE(m_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::Button("Done") || submitted) {
            assert(m_on_complete);
            m_on_complete(std::string(m_buffer));
            m_on_complete = nullptr;
            m_open = false;
        }
    }
    ImGui::End();
}

bool editor_t::open() {
    return m_open;
}

bool editor_t::is_captured_mouse() {
    return open() && ImGui::GetIO().WantCaptureMouse;
}

bool editor_t::is_captured_keyboard() {
    return open() && ImGui::GetIO().WantCaptureKeyboard;
}
