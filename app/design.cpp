#include "design.h"
#include "mathengine/design.h"
#include "mathengine/pretty.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <sstream>

static const unsigned int kStrokeColors[] = {
    IM_COL32(255, 200, 60, 255),  IM_COL32(80, 200, 255, 255),
    IM_COL32(255, 100, 100, 255), IM_COL32(100, 255, 100, 255),
    IM_COL32(200, 100, 255, 255), IM_COL32(255, 150, 50, 255),
    IM_COL32(100, 255, 220, 255), IM_COL32(255, 100, 200, 255),
};

static void fit_stroke(AppState::Stroke& stroke, int degree) {
    if (stroke.points.size() < 3) return;

    std::vector<mathengine::Point2D> pts;
    for (auto& p : stroke.points) pts.push_back({p.x, p.y});

    auto fit = mathengine::parametric_fit(pts, degree);
    stroke.desmos = fit.to_desmos();
    stroke.fitted = true;

    // Generate label
    char buf[32];
    snprintf(buf, sizeof(buf), "Stroke (%d pts)", (int)stroke.points.size());
    stroke.label = buf;
}

static void fit_all_strokes(AppState& state) {
    state.design_desmos_all.clear();
    std::ostringstream ss;
    for (size_t i = 0; i < state.strokes.size(); i++) {
        fit_stroke(state.strokes[i], state.design_fit_order);
        if (i > 0) ss << "\n";
        ss << state.strokes[i].desmos;
    }
    state.design_desmos_all = ss.str();
}

void render_design_tab(AppState& state, ImVec2 content_size) {
    ImGui::BeginChild("DesignArea", content_size);

    float panel_w = 300;
    bool dark = (state.theme == AppState::Theme::Dark);

    // --- Left panel ---
    ImGui::BeginChild("DesignControls", ImVec2(panel_w, 0), ImGuiChildFlags_Border);

    ImGui::Text("Graph Designer");
    ImGui::TextDisabled("Draw shapes with multiple strokes");
    ImGui::Separator();

    // Tool
    ImGui::RadioButton("Draw", &state.design_tool, 0); ImGui::SameLine();
    ImGui::RadioButton("Erase", &state.design_tool, 1);

    ImGui::Separator();

    // --- Background image ---
    if (ImGui::CollapsingHeader("Background Image", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Trace over a photo");
        float bw = (ImGui::GetContentRegionAvail().x - 6) / 2;
        if (ImGui::Button("Browse...", ImVec2(bw, 0))) {
            state.bg_image_pick_requested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload", ImVec2(bw, 0)) && state.bg_image_path[0]) {
            state.bg_image_load_requested = true;
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##bgpath", "Or paste a path",
            state.bg_image_path, sizeof(state.bg_image_path));

        if (state.bg_image_texture != 0) {
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Opacity", &state.bg_image_opacity, 0.05f, 1.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("Scale", &state.bg_image_scale, 0.1f, 5.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::DragFloat("Offset X", &state.bg_image_offset_x, 2.0f);
            ImGui::SetNextItemWidth(-1);
            ImGui::DragFloat("Offset Y", &state.bg_image_offset_y, 2.0f);

            ImGui::Checkbox("Lock image", &state.bg_image_lock);
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset")) {
                state.bg_image_offset_x = 0;
                state.bg_image_offset_y = 0;
                state.bg_image_scale = 1.0f;
            }
            if (!state.bg_image_lock) {
                ImGui::TextDisabled("Hold Shift+drag to move image");
                ImGui::TextDisabled("Hold Shift+scroll to zoom image");
            }
            ImGui::Spacing();
            if (ImGui::Button("Remove Image", ImVec2(-1, 0))) {
                state.bg_image_texture = 0;
                state.bg_image_width = 0;
                state.bg_image_height = 0;
                state.bg_image_status.clear();
            }
        }
        if (!state.bg_image_status.empty()) {
            ImGui::TextDisabled("%s", state.bg_image_status.c_str());
        }
    }

    ImGui::Separator();

    ImGui::SliderInt("Fit Quality", &state.design_fit_order, 5, 30);
    ImGui::TextDisabled("Higher = tighter fit to strokes");

    ImGui::Spacing();
    if (ImGui::Button("Fit All Strokes", ImVec2(-1, 28))) {
        fit_all_strokes(state);
    }

    if (ImGui::Button("Clear All", ImVec2(-1, 0))) {
        state.strokes.clear();
        state.current_stroke.clear();
        state.design_desmos_all.clear();
    }

    // --- Stroke list ---
    ImGui::Separator();
    ImGui::Text("Strokes: %d", (int)state.strokes.size());

    int remove_idx = -1;
    for (int i = 0; i < (int)state.strokes.size(); i++) {
        auto& s = state.strokes[i];
        ImGui::PushID(i);

        float col4[3] = {
            ((s.color >> 0) & 0xFF) / 255.0f,
            ((s.color >> 8) & 0xFF) / 255.0f,
            ((s.color >> 16) & 0xFF) / 255.0f,
        };
        ImGui::ColorEdit3("##c", col4, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();

        char label[64];
        snprintf(label, sizeof(label), "%d: %d pts%s", i + 1,
                 (int)s.points.size(), s.fitted ? " (fit)" : "");
        ImGui::Text("%s", label);
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) remove_idx = i;

        ImGui::PopID();
    }
    if (remove_idx >= 0) {
        state.strokes.erase(state.strokes.begin() + remove_idx);
        fit_all_strokes(state);
    }

    // --- Desmos output ---
    if (!state.design_desmos_all.empty()) {
        ImGui::Separator();
        ImGui::Text("Desmos (parametric):");
        ImGui::TextDisabled("Each stroke is one parametric eq.");
        ImGui::TextDisabled("In Desmos, set t: 0 to 1");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
        for (auto& s : state.strokes) {
            if (s.fitted) {
                ImGui::TextWrapped("%s", s.desmos.c_str());
                ImGui::Spacing();
            }
        }
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (ImGui::Button("Copy All to Clipboard", ImVec2(-1, 0))) {
            // Copy each stroke's desmos on separate lines
            std::string all;
            for (size_t i = 0; i < state.strokes.size(); i++) {
                if (state.strokes[i].fitted) {
                    if (!all.empty()) all += "\n";
                    all += state.strokes[i].desmos;
                }
            }
            ImGui::SetClipboardText(all.c_str());
        }

        if (ImGui::Button("Copy Single Stroke", ImVec2(-1, 0))) {
            // Copy just the last stroke
            for (int i = (int)state.strokes.size() - 1; i >= 0; i--) {
                if (state.strokes[i].fitted) {
                    ImGui::SetClipboardText(state.strokes[i].desmos.c_str());
                    break;
                }
            }
        }
    }

    ImGui::EndChild(); // DesignControls

    ImGui::SameLine();

    // --- Right panel: canvas ---
    ImGui::BeginChild("DesignCanvas", ImVec2(0, 0), ImGuiChildFlags_Border);
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 bg = dark ? IM_COL32(25, 25, 30, 255) : IM_COL32(245, 245, 250, 255);
    dl->AddRectFilled(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), bg);

    // Background image (if loaded), scaled and translated by user controls
    if (state.bg_image_texture != 0 && state.bg_image_width > 0 && state.bg_image_height > 0) {
        float img_aspect = (float)state.bg_image_width / (float)state.bg_image_height;
        float canvas_aspect = canvas_size.x / canvas_size.y;
        float base_w, base_h;
        if (img_aspect > canvas_aspect) {
            base_w = canvas_size.x;
            base_h = canvas_size.x / img_aspect;
        } else {
            base_h = canvas_size.y;
            base_w = canvas_size.y * img_aspect;
        }
        float draw_w = base_w * state.bg_image_scale;
        float draw_h = base_h * state.bg_image_scale;
        float ox = canvas_pos.x + (canvas_size.x - draw_w) / 2 + state.bg_image_offset_x;
        float oy = canvas_pos.y + (canvas_size.y - draw_h) / 2 + state.bg_image_offset_y;
        unsigned int alpha = (unsigned int)(state.bg_image_opacity * 255) & 0xFF;
        ImU32 tint = IM_COL32(255, 255, 255, alpha);
        dl->AddImage((ImTextureID)(intptr_t)state.bg_image_texture,
                     ImVec2(ox, oy), ImVec2(ox + draw_w, oy + draw_h),
                     ImVec2(0, 0), ImVec2(1, 1), tint);
    }

    float cx = canvas_pos.x, cy = canvas_pos.y;
    float cw = canvas_size.x, ch = canvas_size.y;
    double vx0 = state.x_min, vx1 = state.x_max;
    double vy0 = state.y_min, vy1 = state.y_max;

    auto sx = [&](double x) { return cx + (float)((x - vx0) / (vx1 - vx0)) * cw; };
    auto sy = [&](double y) { return cy + (float)((vy1 - y) / (vy1 - vy0)) * ch; };
    auto mx = [&](float s) { return vx0 + (s - cx) / cw * (vx1 - vx0); };
    auto my = [&](float s) { return vy1 - (s - cy) / ch * (vy1 - vy0); };

    // Grid + axes
    ImU32 grid_col = dark ? IM_COL32(40, 40, 50, 255) : IM_COL32(220, 220, 230, 255);
    ImU32 axis_col = dark ? IM_COL32(80, 80, 100, 255) : IM_COL32(140, 140, 160, 255);
    ImU32 text_col = dark ? IM_COL32(120, 120, 140, 255) : IM_COL32(100, 100, 120, 255);

    double xstep = std::pow(10.0, std::floor(std::log10(std::max(0.001, (vx1 - vx0) / 5))));
    for (double gx = std::ceil(vx0/xstep)*xstep; gx <= vx1; gx += xstep) {
        float px = sx(gx);
        bool is_ax = std::abs(gx) < xstep * 0.01;
        dl->AddLine(ImVec2(px, cy), ImVec2(px, cy+ch), is_ax ? axis_col : grid_col);
        char lbl[16]; snprintf(lbl, 16, "%.3g", gx);
        dl->AddText(ImVec2(px+2, cy+ch-13), text_col, lbl);
    }
    double ystep = std::pow(10.0, std::floor(std::log10(std::max(0.001, (vy1 - vy0) / 4))));
    for (double gy = std::ceil(vy0/ystep)*ystep; gy <= vy1; gy += ystep) {
        float py = sy(gy);
        bool is_ax = std::abs(gy) < ystep * 0.01;
        dl->AddLine(ImVec2(cx, py), ImVec2(cx+cw, py), is_ax ? axis_col : grid_col);
        char lbl[16]; snprintf(lbl, 16, "%.3g", gy);
        dl->AddText(ImVec2(cx+2, py-13), text_col, lbl);
    }

    // --- Draw completed strokes ---
    for (size_t si = 0; si < state.strokes.size(); si++) {
        auto& stroke = state.strokes[si];
        ImU32 col = stroke.color;

        // Draw raw points as connected line
        if (stroke.points.size() >= 2) {
            for (size_t i = 0; i + 1 < stroke.points.size(); i++) {
                auto& a = stroke.points[i];
                auto& b = stroke.points[i+1];
                dl->AddLine(ImVec2(sx(a.x), sy(a.y)), ImVec2(sx(b.x), sy(b.y)),
                           (col & 0x00FFFFFF) | 0x60000000, 2.0f); // semi-transparent
            }
        }

        // Draw fitted parametric curve (if available)
        if (stroke.fitted && stroke.points.size() >= 3) {
            std::vector<mathengine::Point2D> pts;
            for (auto& p : stroke.points) pts.push_back({p.x, p.y});
            auto fit = mathengine::parametric_fit(pts, state.design_fit_order);

            float prev_px = 0, prev_py = 0;
            bool first = true;
            int steps = std::max(200, (int)stroke.points.size() * 3);
            for (int i = 0; i <= steps; i++) {
                double t = (double)i / steps;
                auto pt = fit.eval(t);
                float px = sx(pt.x), py = sy(pt.y);
                if (px < cx - 100 || px > cx + cw + 100 || py < cy - 100 || py > cy + ch + 100) {
                    first = true; continue;
                }
                if (!first) {
                    dl->AddLine(ImVec2(prev_px, prev_py), ImVec2(px, py), col, 2.5f);
                }
                prev_px = px; prev_py = py; first = false;
            }
        }
    }

    // --- Draw in-progress stroke ---
    if (!state.current_stroke.empty()) {
        ImU32 active_col = kStrokeColors[state.strokes.size() % 8];
        for (size_t i = 0; i + 1 < state.current_stroke.size(); i++) {
            auto& a = state.current_stroke[i];
            auto& b = state.current_stroke[i+1];
            dl->AddLine(ImVec2(sx(a.x), sy(a.y)), ImVec2(sx(b.x), sy(b.y)), active_col, 3.0f);
        }
    }

    // --- Mouse input ---
    ImGui::SetCursorScreenPos(canvas_pos);
    ImGui::InvisibleButton("design_canvas", canvas_size);
    bool hovered = ImGui::IsItemHovered();

    // --- Shift+drag/scroll: move/scale background image ---
    bool shift = ImGui::GetIO().KeyShift;
    bool image_loaded = (state.bg_image_texture != 0);
    bool image_interact = shift && image_loaded && !state.bg_image_lock;

    if (image_interact && hovered) {
        // Shift+scroll: zoom image
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            float factor = (wheel > 0) ? 1.1f : 0.9f;
            state.bg_image_scale = std::clamp(state.bg_image_scale * factor, 0.05f, 20.0f);
        }
    }
    if (image_interact && ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        state.bg_image_offset_x += d.x;
        state.bg_image_offset_y += d.y;
    }

    // Disable drawing while shift is held (image-edit mode)
    if (image_interact) {
        // Show "moving image" indicator at cursor
        if (hovered) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            ImU32 ind = IM_COL32(255, 220, 80, 220);
            dl->AddCircle(mouse, 10, ind, 16, 2.0f);
            dl->AddText(ImVec2(mouse.x + 14, mouse.y - 8), ind, "Move/scale image");
        }
    } else if (state.design_tool == 0) {
        // DRAW mode
        if (hovered && ImGui::IsMouseDown(0)) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            double px = mx(mouse.x), py = my(mouse.y);

            if (!state.drawing_active) {
                state.drawing_active = true;
                state.current_stroke.clear();
            }

            // Add point if far enough from last
            bool add = true;
            if (!state.current_stroke.empty()) {
                auto& last = state.current_stroke.back();
                float dx = sx(px) - sx(last.x);
                float dy = sy(py) - sy(last.y);
                if (std::sqrt(dx*dx + dy*dy) < 2.0f) add = false;
            }
            if (add) {
                state.current_stroke.push_back({(float)px, (float)py});
            }
        }

        // Mouse released: finish stroke
        if (state.drawing_active && !ImGui::IsMouseDown(0)) {
            state.drawing_active = false;
            if (state.current_stroke.size() >= 3) {
                AppState::Stroke stroke;
                stroke.points = state.current_stroke;
                stroke.color = kStrokeColors[state.strokes.size() % 8];
                fit_stroke(stroke, state.design_fit_order);
                state.strokes.push_back(std::move(stroke));
                // Rebuild combined desmos
                fit_all_strokes(state);
            }
            state.current_stroke.clear();
        }
    } else {
        // ERASE mode: click near a stroke to delete it
        if (hovered && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            double epx = mx(mouse.x), epy = my(mouse.y);

            int closest = -1;
            float closest_dist = 20.0f; // pixel threshold
            for (int i = 0; i < (int)state.strokes.size(); i++) {
                for (auto& p : state.strokes[i].points) {
                    float dx = sx(p.x) - mouse.x;
                    float dy = sy(p.y) - mouse.y;
                    float d = std::sqrt(dx*dx + dy*dy);
                    if (d < closest_dist) {
                        closest_dist = d;
                        closest = i;
                    }
                }
            }
            if (closest >= 0) {
                state.strokes.erase(state.strokes.begin() + closest);
                fit_all_strokes(state);
            }
        }
    }

    // Zoom + pan (right-click drag to pan, scroll to zoom)
    if (hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            double mcx = mx(mouse.x), mcy = my(mouse.y);
            double zoom = (wheel > 0) ? 0.9 : 1.1;
            state.x_min = mcx + (state.x_min - mcx) * zoom;
            state.x_max = mcx + (state.x_max - mcx) * zoom;
            state.y_min = mcy + (state.y_min - mcy) * zoom;
            state.y_max = mcy + (state.y_max - mcy) * zoom;
        }
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(1)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        double ddx = -(delta.x / cw) * (vx1 - vx0);
        double ddy = (delta.y / ch) * (vy1 - vy0);
        state.x_min += ddx; state.x_max += ddx;
        state.y_min += ddy; state.y_max += ddy;
    }

    // Cursor label
    if (hovered) {
        ImVec2 mouse = ImGui::GetIO().MousePos;
        char buf[64];
        snprintf(buf, sizeof(buf), "(%.2f, %.2f)", mx(mouse.x), my(mouse.y));
        ImU32 tc = dark ? IM_COL32(200, 200, 210, 200) : IM_COL32(40, 40, 50, 200);
        dl->AddText(ImVec2(mouse.x + 12, mouse.y - 16), tc, buf);

        // Drawing cursor
        if (state.design_tool == 0)
            dl->AddCircle(mouse, 4, IM_COL32(255, 255, 255, 150), 12, 1.5f);
        else
            dl->AddCircle(mouse, 12, IM_COL32(255, 80, 80, 150), 12, 1.5f);
    }

    ImGui::EndChild(); // DesignCanvas
    ImGui::EndChild(); // DesignArea
}
