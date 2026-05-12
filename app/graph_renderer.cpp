#include "graph_renderer.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace {

float math_to_screen_x(double x, double x_min, double x_max, float screen_left, float screen_width) {
    return screen_left + (float)((x - x_min) / (x_max - x_min)) * screen_width;
}

float math_to_screen_y(double y, double y_min, double y_max, float screen_top, float screen_height) {
    return screen_top + (float)((y_max - y) / (y_max - y_min)) * screen_height;
}

double nice_step(double range, int target_ticks) {
    double raw = range / target_ticks;
    double mag = std::pow(10.0, std::floor(std::log10(raw)));
    double norm = raw / mag;
    double nice;
    if (norm < 1.5) nice = 1;
    else if (norm < 3.5) nice = 2;
    else if (norm < 7.5) nice = 5;
    else nice = 10;
    return nice * mag;
}

} // anonymous namespace

void render_graph(AppState& state, ImVec2 pos, ImVec2 size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float sx = pos.x, sy = pos.y;
    float sw = size.x, sh = size.y;

    // Theme-aware colors
    bool dark = (state.theme == AppState::Theme::Dark);
    ImU32 bg_color     = dark ? IM_COL32(25, 25, 30, 255)    : IM_COL32(245, 245, 250, 255);
    ImU32 border_color = dark ? IM_COL32(60, 60, 70, 255)    : IM_COL32(180, 180, 190, 255);
    ImU32 grid_color   = dark ? IM_COL32(45, 45, 55, 255)    : IM_COL32(210, 210, 220, 255);
    ImU32 axis_color   = dark ? IM_COL32(100, 100, 120, 255) : IM_COL32(80, 80, 100, 255);
    ImU32 text_color   = dark ? IM_COL32(150, 150, 170, 255) : IM_COL32(60, 60, 80, 255);

    // Background
    draw_list->AddRectFilled(pos, ImVec2(sx + sw, sy + sh), bg_color);
    draw_list->AddRect(pos, ImVec2(sx + sw, sy + sh), border_color);

    // Grid lines
    double x_step = nice_step(state.x_max - state.x_min, 10);
    double y_step = nice_step(state.y_max - state.y_min, 8);

    double x_start = std::ceil(state.x_min / x_step) * x_step;
    for (double x = x_start; x <= state.x_max; x += x_step) {
        float screen_x = math_to_screen_x(x, state.x_min, state.x_max, sx, sw);
        bool is_axis = std::abs(x) < x_step * 0.01;
        draw_list->AddLine(ImVec2(screen_x, sy), ImVec2(screen_x, sy + sh),
                          is_axis ? axis_color : grid_color);
        char label[32];
        snprintf(label, sizeof(label), "%.4g", x);
        draw_list->AddText(ImVec2(screen_x + 2, sy + sh - 14), text_color, label);
    }

    double y_start = std::ceil(state.y_min / y_step) * y_step;
    for (double y = y_start; y <= state.y_max; y += y_step) {
        float screen_y = math_to_screen_y(y, state.y_min, state.y_max, sy, sh);
        bool is_axis = std::abs(y) < y_step * 0.01;
        draw_list->AddLine(ImVec2(sx, screen_y), ImVec2(sx + sw, screen_y),
                          is_axis ? axis_color : grid_color);
        char label[32];
        snprintf(label, sizeof(label), "%.4g", y);
        draw_list->AddText(ImVec2(sx + 2, screen_y - 14), text_color, label);
    }

    // Resample if needed
    resample_all(state, sw);

    // --- Integral shading ---
    if (state.integral.enabled && state.integral.plot_index >= 0 &&
        state.integral.plot_index < (int)state.plots.size()) {
        const auto& plot = state.plots[state.integral.plot_index];
        if (plot.visible && !plot.plot_x.empty() && plot.compiled) {
            // Shade color: same hue as plot, low alpha
            ImU32 shade_color = (plot.color & 0xFF00FFFF) | 0x40000000;
            // Fix alpha: set to ~25%
            unsigned int r = (plot.color >> 0) & 0xFF;
            unsigned int g = (plot.color >> 8) & 0xFF;
            unsigned int b = (plot.color >> 16) & 0xFF;
            shade_color = IM_COL32(r, g, b, 60);

            float axis_sy = math_to_screen_y(0.0, state.y_min, state.y_max, sy, sh);
            axis_sy = std::clamp(axis_sy, sy, sy + sh);

            double area = 0.0;
            for (size_t i = 0; i + 1 < plot.plot_x.size(); i++) {
                float px0 = plot.plot_x[i], px1 = plot.plot_x[i + 1];
                if (px1 < state.integral.x_lo || px0 > state.integral.x_hi) continue;
                if (plot.discontinuity[i]) continue;
                float py0 = plot.plot_y[i], py1 = plot.plot_y[i + 1];
                if (std::isnan(py0) || std::isnan(py1) || std::isinf(py0) || std::isinf(py1)) continue;

                // Trapezoidal area accumulation
                area += 0.5 * (py0 + py1) * (px1 - px0);

                float sx0 = math_to_screen_x(px0, state.x_min, state.x_max, sx, sw);
                float sy0 = math_to_screen_y(std::clamp((double)py0, state.y_min, state.y_max),
                                             state.y_min, state.y_max, sy, sh);
                float sx1 = math_to_screen_x(px1, state.x_min, state.x_max, sx, sw);
                float sy1 = math_to_screen_y(std::clamp((double)py1, state.y_min, state.y_max),
                                             state.y_min, state.y_max, sy, sh);

                ImVec2 quad[4] = {
                    {sx0, sy0}, {sx1, sy1},
                    {sx1, axis_sy}, {sx0, axis_sy}
                };
                draw_list->AddConvexPolyFilled(quad, 4, shade_color);
            }
            state.integral.computed_area = area;

            // Bound lines
            ImU32 bound_color = IM_COL32(255, 255, 100, 180);
            float lo_sx = math_to_screen_x(state.integral.x_lo, state.x_min, state.x_max, sx, sw);
            float hi_sx = math_to_screen_x(state.integral.x_hi, state.x_min, state.x_max, sx, sw);
            if (lo_sx >= sx && lo_sx <= sx + sw)
                draw_list->AddLine(ImVec2(lo_sx, sy), ImVec2(lo_sx, sy + sh), bound_color, 1.5f);
            if (hi_sx >= sx && hi_sx <= sx + sw)
                draw_list->AddLine(ImVec2(hi_sx, sy), ImVec2(hi_sx, sy + sh), bound_color, 1.5f);
        }
    }

    // --- Plot all functions ---
    for (const auto& plot : state.plots) {
        if (!plot.visible || plot.plot_x.empty() || !plot.compiled) continue;
        ImU32 col = plot.color;
        for (size_t i = 0; i + 1 < plot.plot_x.size(); i++) {
            if (plot.discontinuity[i]) continue;

            float y1 = plot.plot_y[i], y2 = plot.plot_y[i + 1];
            if ((y1 < state.y_min && y2 < state.y_min) || (y1 > state.y_max && y2 > state.y_max))
                continue;

            float x1s = math_to_screen_x(plot.plot_x[i], state.x_min, state.x_max, sx, sw);
            float y1s = math_to_screen_y(y1, state.y_min, state.y_max, sy, sh);
            float x2s = math_to_screen_x(plot.plot_x[i + 1], state.x_min, state.x_max, sx, sw);
            float y2s = math_to_screen_y(y2, state.y_min, state.y_max, sy, sh);

            y1s = std::clamp(y1s, sy, sy + sh);
            y2s = std::clamp(y2s, sy, sy + sh);

            draw_list->AddLine(ImVec2(x1s, y1s), ImVec2(x2s, y2s), col, 2.0f);
        }
    }

    // --- Legend ---
    {
        int visible_count = 0;
        for (auto& p : state.plots) if (p.visible && p.compiled) visible_count++;
        if (visible_count > 0) {
            float line_height = 18.0f;
            float legend_w = 180.0f;
            float legend_x = sx + sw - legend_w - 10.0f;
            float legend_y = sy + 10.0f;
            float legend_h = visible_count * line_height + 8.0f;

            ImU32 legend_bg = dark ? IM_COL32(30, 30, 35, 220) : IM_COL32(240, 240, 245, 220);
            draw_list->AddRectFilled(
                ImVec2(legend_x, legend_y),
                ImVec2(legend_x + legend_w, legend_y + legend_h),
                legend_bg, 4.0f);

            float cy = legend_y + 4.0f;
            for (auto& p : state.plots) {
                if (!p.visible || !p.compiled) continue;
                draw_list->AddRectFilled(
                    ImVec2(legend_x + 6, cy + 3),
                    ImVec2(legend_x + 20, cy + 13),
                    p.color, 2.0f);

                const char* label = p.expression;
                if (p.type == PlotEntry::Type::Polar) {
                    // Show "r = ..."
                    static char buf[520];
                    snprintf(buf, sizeof(buf), "r = %s", p.expression);
                    label = buf;
                }
                draw_list->AddText(ImVec2(legend_x + 26, cy), text_color, label);
                cy += line_height;
            }
        }
    }

    // --- Mouse interaction area ---
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("graph_area", size);
    bool hovered = ImGui::IsItemHovered();

    // --- Crosshair ---
    if (state.crosshair_enabled && hovered) {
        ImVec2 mouse = ImGui::GetIO().MousePos;
        double mx = state.x_min + (mouse.x - sx) / sw * (state.x_max - state.x_min);
        double my = state.y_max - (mouse.y - sy) / sh * (state.y_max - state.y_min);
        state.crosshair_x = mx;
        state.crosshair_y = my;
        state.crosshair_visible = true;

        ImU32 cross_color = dark ? IM_COL32(200, 200, 200, 80) : IM_COL32(100, 100, 100, 80);
        draw_list->AddLine(ImVec2(mouse.x, sy), ImVec2(mouse.x, sy + sh), cross_color, 1.0f);
        draw_list->AddLine(ImVec2(sx, mouse.y), ImVec2(sx + sw, mouse.y), cross_color, 1.0f);

        char coord_buf[64];
        snprintf(coord_buf, sizeof(coord_buf), "(%.4g, %.4g)", mx, my);
        ImU32 coord_color = dark ? IM_COL32(255, 255, 255, 220) : IM_COL32(0, 0, 0, 220);
        ImU32 coord_bg = dark ? IM_COL32(30, 30, 35, 200) : IM_COL32(240, 240, 245, 200);
        ImVec2 text_size = ImGui::CalcTextSize(coord_buf);
        float tx = mouse.x + 12, ty = mouse.y - 20;
        draw_list->AddRectFilled(ImVec2(tx - 2, ty - 1), ImVec2(tx + text_size.x + 4, ty + text_size.y + 2), coord_bg, 3.0f);
        draw_list->AddText(ImVec2(tx, ty), coord_color, coord_buf);
    } else {
        state.crosshair_visible = false;
    }

    // --- Zoom ---
    if (hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            double mx = state.x_min + (mouse.x - sx) / sw * (state.x_max - state.x_min);
            double my = state.y_max - (mouse.y - sy) / sh * (state.y_max - state.y_min);

            double zoom = (wheel > 0) ? 0.9 : 1.1;
            state.x_min = mx + (state.x_min - mx) * zoom;
            state.x_max = mx + (state.x_max - mx) * zoom;
            state.y_min = my + (state.y_min - my) * zoom;
            state.y_max = my + (state.y_max - my) * zoom;
            state.needs_resample = true;
        }
    }

    // --- Pan ---
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        double dx = -(delta.x / sw) * (state.x_max - state.x_min);
        double dy = (delta.y / sh) * (state.y_max - state.y_min);
        state.x_min += dx;
        state.x_max += dx;
        state.y_min += dy;
        state.y_max += dy;
        state.needs_resample = true;
    }
}
