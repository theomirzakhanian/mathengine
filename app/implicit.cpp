#include "implicit.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"
#include <cmath>
#include <cstdio>

void render_implicit_tab(AppState& state, ImVec2 content_size) {
    ImGui::BeginChild("ImplicitArea", content_size);
    bool dark = (state.theme == AppState::Theme::Dark);
    float panel_w = 300;

    // --- Left panel ---
    ImGui::BeginChild("ImplicitControls", ImVec2(panel_w, 0), ImGuiChildFlags_Border);
    ImGui::Text("Implicit Curves");
    ImGui::TextDisabled("Plot f(x,y) = 0");
    ImGui::Separator();

    ImGui::Text("f(x, y) = 0:");
    if (ImGui::InputText("##impl_expr", state.implicit_expr, sizeof(state.implicit_expr)))
        state.implicit_dirty = true;

    ImGui::Checkbox("Shade f(x,y) < 0", &state.implicit_shade_ineq);

    ImGui::Separator();
    ImGui::Text("Examples:");
    const char* impl_exs[] = {
        "x^2 + y^2 - 9",
        "x^2/4 - y^2/9 - 1",
        "x^2 + y^2 - 1",
        "sin(x) - y",
        "x^3 + y^3 - 3*x*y",
        "sin(x*y) - 0.5",
        "(x^2+y^2)^2 - 4*(x^2-y^2)",
    };
    for (auto* ex : impl_exs) {
        if (ImGui::SmallButton(ex)) {
            snprintf(state.implicit_expr, sizeof(state.implicit_expr), "%s", ex);
            state.implicit_dirty = true;
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // --- Right panel: rendered implicit curve ---
    ImGui::BeginChild("ImplicitPlot", ImVec2(0, 0), ImGuiChildFlags_Border);
    ImVec2 plot_pos = ImGui::GetCursorScreenPos();
    ImVec2 plot_size = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 bg = dark ? IM_COL32(25, 25, 30, 255) : IM_COL32(245, 245, 250, 255);
    dl->AddRectFilled(plot_pos, ImVec2(plot_pos.x + plot_size.x, plot_pos.y + plot_size.y), bg);

    float px = plot_pos.x, py = plot_pos.y;
    float pw = plot_size.x, ph = plot_size.y;

    double vx0 = state.x_min, vx1 = state.x_max;
    double vy0 = state.y_min, vy1 = state.y_max;

    // Grid
    ImU32 axis_col = dark ? IM_COL32(60, 60, 80, 255) : IM_COL32(160, 160, 180, 255);
    float ax = px + (float)((0 - vx0) / (vx1 - vx0)) * pw;
    float ay = py + (float)((vy1 - 0) / (vy1 - vy0)) * ph;
    if (ax >= px && ax <= px + pw)
        dl->AddLine(ImVec2(ax, py), ImVec2(ax, py + ph), axis_col);
    if (ay >= py && ay <= py + ph)
        dl->AddLine(ImVec2(px, ay), ImVec2(px + pw, ay), axis_col);

    // Render implicit curve using marching squares
    try {
        auto expr = mathengine::parse(state.implicit_expr);
        int res = std::min(400, (int)std::max(pw, ph));
        // Evaluate on grid
        std::vector<double> grid((res+1) * (res+1));
        for (int iy = 0; iy <= res; iy++) {
            for (int ix = 0; ix <= res; ix++) {
                double x = vx0 + (vx1 - vx0) * ix / res;
                double y = vy1 - (vy1 - vy0) * iy / res; // y inverted for screen
                double val = mathengine::evaluate(expr, {{"x", x}, {"y", y}});
                if (std::isnan(val) || std::isinf(val)) val = 1e10;
                grid[iy * (res+1) + ix] = val;
            }
        }

        ImU32 curve_col = dark ? IM_COL32(80, 200, 255, 255) : IM_COL32(20, 100, 220, 255);
        ImU32 shade_col = dark ? IM_COL32(40, 80, 120, 60) : IM_COL32(100, 150, 220, 40);

        float cell_w = pw / res, cell_h = ph / res;

        // Shade inequality region
        if (state.implicit_shade_ineq) {
            for (int iy = 0; iy < res; iy++) {
                for (int ix = 0; ix < res; ix++) {
                    double v = grid[iy * (res+1) + ix];
                    if (v < 0) {
                        float fx = px + ix * cell_w;
                        float fy = py + iy * cell_h;
                        dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + cell_w, fy + cell_h), shade_col);
                    }
                }
            }
        }

        // Marching squares for the zero contour
        for (int iy = 0; iy < res; iy++) {
            for (int ix = 0; ix < res; ix++) {
                double v00 = grid[iy*(res+1)+ix];
                double v10 = grid[iy*(res+1)+ix+1];
                double v01 = grid[(iy+1)*(res+1)+ix];
                double v11 = grid[(iy+1)*(res+1)+ix+1];

                float fx = px + ix * cell_w, fy = py + iy * cell_h;

                // Find edge crossings
                struct Pt { float x, y; };
                Pt crossings[4];
                int nc = 0;

                auto interp = [](float a, float b, double va, double vb) {
                    double t = va / (va - vb);
                    return a + (float)(t) * (b - a);
                };

                // Top edge
                if ((v00 > 0) != (v10 > 0))
                    crossings[nc++] = {interp(fx, fx+cell_w, v00, v10), fy};
                // Right edge
                if ((v10 > 0) != (v11 > 0))
                    crossings[nc++] = {fx+cell_w, interp(fy, fy+cell_h, v10, v11)};
                // Bottom edge
                if ((v01 > 0) != (v11 > 0))
                    crossings[nc++] = {interp(fx, fx+cell_w, v01, v11), fy+cell_h};
                // Left edge
                if ((v00 > 0) != (v01 > 0))
                    crossings[nc++] = {fx, interp(fy, fy+cell_h, v00, v01)};

                if (nc >= 2) {
                    dl->AddLine(ImVec2(crossings[0].x, crossings[0].y),
                               ImVec2(crossings[1].x, crossings[1].y), curve_col, 2.0f);
                }
                if (nc >= 4) {
                    dl->AddLine(ImVec2(crossings[2].x, crossings[2].y),
                               ImVec2(crossings[3].x, crossings[3].y), curve_col, 2.0f);
                }
            }
        }
    } catch (...) {
        ImU32 err_col = dark ? IM_COL32(255, 80, 80, 200) : IM_COL32(200, 0, 0, 200);
        dl->AddText(ImVec2(px + pw/2 - 40, py + ph/2), err_col, "Invalid expression");
    }

    // Mouse interaction: zoom/pan
    ImGui::SetCursorScreenPos(plot_pos);
    ImGui::InvisibleButton("impl_plot", plot_size);
    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            double mx = vx0 + (mouse.x - px) / pw * (vx1 - vx0);
            double my = vy1 - (mouse.y - py) / ph * (vy1 - vy0);
            double z = (wheel > 0) ? 0.9 : 1.1;
            state.x_min = mx + (state.x_min - mx) * z;
            state.x_max = mx + (state.x_max - mx) * z;
            state.y_min = my + (state.y_min - my) * z;
            state.y_max = my + (state.y_max - my) * z;
            state.implicit_dirty = true;
        }
        // Cursor position
        ImVec2 mouse = ImGui::GetIO().MousePos;
        double mx = vx0 + (mouse.x - px) / pw * (vx1 - vx0);
        double my = vy1 - (mouse.y - py) / ph * (vy1 - vy0);
        char buf[64]; snprintf(buf, 64, "(%.3f, %.3f)", mx, my);
        ImU32 tc = dark ? IM_COL32(200,200,210,200) : IM_COL32(40,40,50,200);
        dl->AddText(ImVec2(mouse.x+12, mouse.y-16), tc, buf);
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        double dx = -(delta.x / pw) * (vx1 - vx0);
        double dy = (delta.y / ph) * (vy1 - vy0);
        state.x_min += dx; state.x_max += dx;
        state.y_min += dy; state.y_max += dy;
        state.implicit_dirty = true;
    }

    ImGui::EndChild();
    ImGui::EndChild();
}
