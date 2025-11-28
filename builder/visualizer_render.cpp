#include <visualizer_render.h>
#include <visualizer_space.h>
#include <rlImGui.h>
#include <raylib.h>

namespace {

struct fitted_text_t {
    std::string wrapped;
    float font_size;
    float spacing;
};

std::string wrap_text_to_width(Font font, const std::string& text, float font_size, float spacing, float max_width) {
    std::istringstream iss(text);
    std::string word;
    std::string current_line;
    std::string result;

    while (iss >> word) {
        std::string trial = current_line.empty() ? word : current_line + " " + word;
        Vector2 size = MeasureTextEx(font, trial.c_str(), font_size, spacing);
        if (size.x > max_width && !current_line.empty()) {
            result += current_line + "\n";
            current_line = word;
        } else {
            current_line = trial;
        }
    }

    if (!current_line.empty()) {
        result += current_line;
    }

    return result;
}

fitted_text_t fit_text_wrapped(Font font, const std::string& text, rec_t rec, float spacing) {
    const float test_font_size = 10.0f;
    Vector2 char_size = MeasureTextEx(font, "M", test_font_size, spacing);

    fitted_text_t best_fit = { text, test_font_size, spacing };
    float best_font_size = 0.0f;

    float low = 2.0f;
    float high = rec.bottom - rec.top;
    for (int i = 0; i < 16; ++i) {
        float mid = (low + high) * 0.5f;
        std::string wrapped = wrap_text_to_width(font, text, mid, spacing, rec.right - rec.left);
        Vector2 dims = MeasureTextEx(font, wrapped.c_str(), mid, spacing);
        int num_lines = std::count(wrapped.begin(), wrapped.end(), '\n') + 1;
        float total_height = num_lines * char_size.y * (mid / test_font_size);

        if (dims.x <= rec.right - rec.left && total_height <= rec.bottom - rec.top) {
            best_font_size = mid;
            best_fit = { wrapped, mid, spacing };
            low = mid;
        } else {
            high = mid;
        }
    }

    return best_fit;
}

void draw_function(function_t* function, function_t* focus_function, rec_t view_rec, rec_t world_rec, Font font) {
    rec_t function_rec = to_view(function, focus_function, view_rec, world_rec);

    rec_t clipped_rec = function_rec;
    clipped_rec.left = std::clamp(function_rec.left, view_rec.left, view_rec.right);
    clipped_rec.right = std::clamp(function_rec.right, view_rec.left, view_rec.right);
    clipped_rec.top = std::clamp(function_rec.top, view_rec.top, view_rec.bottom);
    clipped_rec.bottom = std::clamp(function_rec.bottom, view_rec.top, view_rec.bottom);

    if (
        clipped_rec.right <= clipped_rec.left ||
        clipped_rec.bottom <= clipped_rec.top
    ) {
        return ;
    }

    const double rec_area = ((double)clipped_rec.right - (double)clipped_rec.left) * ((double)clipped_rec.bottom - (double)clipped_rec.top);
    const double view_area = (double)(view_rec.right - view_rec.left) * (double)(view_rec.bottom - view_rec.top);
    const double area_ratio = rec_area / view_area;
    const float fade = std::clamp((float)(1.0 - area_ratio), 0.3f, 1.0f);

    DrawRectangleRec(
        {
            clipped_rec.left,
            clipped_rec.top,
            (clipped_rec.right - clipped_rec.left),
            (clipped_rec.bottom - clipped_rec.top)
        },
        Fade(BLUE, fade)
    );

    if (function != focus_function) {
        const char* function_name = function->function_ir().function_id.name.c_str();
        fitted_text_t text_fit = fit_text_wrapped(font, function_name, function_rec, 1.0f);

        Vector2 size = MeasureTextEx(font, text_fit.wrapped.c_str(), text_fit.font_size, text_fit.spacing);
        Vector2 pos = { function_rec.left + (function_rec.right - function_rec.left - size.x) / 2,
                        function_rec.top + (function_rec.bottom - function_rec.top - size.y) / 2 };

        DrawTextEx(font, text_fit.wrapped.c_str(), pos, text_fit.font_size, text_fit.spacing, Fade(BLACK, fade));
    }
}

void draw_functions(function_t* function, function_t* focus_function, rec_t view_rec, rec_t world_rec, Font font) {
    for (function_t* child : function->children()) {
        draw_functions(child, focus_function, view_rec, world_rec, font);
    }
    draw_function(function, focus_function, view_rec, world_rec, font);
}

void draw_overlay(const visualizer_state_t& state) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s", state.current_function->function_ir().function_id.name.c_str());
    Vector2 text_dims = MeasureTextEx(GetFontDefault(), buffer, 20.0f, 1.0f);
    DrawText(buffer, (int)((state.overlay.right - state.overlay.left) / 2.0f - text_dims.x / 2.0f), (int)(state.overlay.top + 10.0f), 20, WHITE);
}

void draw_world(const visualizer_state_t& state) {
    if (state.created_function != nullptr) {
        draw_function(state.created_function, state.current_function, state.world, state.camera, state.font);
    }
    draw_functions(state.current_function, state.current_function, state.world, state.camera, state.font);
}

} // namespace

void draw_visualizer(visualizer_state_t& state) {
    BeginDrawing();
    rlImGuiBegin();

    BeginScissorMode((int)state.overlay.left, (int)state.overlay.top, (int)(state.overlay.right - state.overlay.left), (int)(state.overlay.bottom - state.overlay.top));
    ClearBackground(BLACK);
    draw_overlay(state);
    EndScissorMode();

    BeginScissorMode((int)state.world.left, (int)state.world.top, (int)(state.world.right - state.world.left), (int)(state.world.bottom - state.world.top));
    ClearBackground(WHITE);
    draw_world(state);
    EndScissorMode();

    state.editor.draw();

    rlImGuiEnd();
    EndDrawing();
}
