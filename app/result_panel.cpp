#include "result_panel.h"
#include "imgui.h"
#include <cmath>

void render_result_panel(AppState& state) {
    ImGui::Begin("Results");

    // Crosshair info (most important when hovering graph)
    if (state.crosshair_visible) {
        ImGui::Text("x = %.6g", state.crosshair_x);
        ImGui::Text("y = %.6g", state.crosshair_y);

        ImGui::Spacing();
        for (auto& plot : state.plots) {
            if (!plot.visible || !plot.compiled) continue;
            if (plot.type == PlotEntry::Type::Cartesian) {
                double y = plot.compiled->eval(state.crosshair_x);
                if (!std::isnan(y) && !std::isinf(y)) {
                    float r = ((plot.color >> 0) & 0xFF) / 255.0f;
                    float g = ((plot.color >> 8) & 0xFF) / 255.0f;
                    float b = ((plot.color >> 16) & 0xFF) / 255.0f;
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(r, g, b, 1.0f));
                    ImGui::Text("%s = %.6g", plot.expression, y);
                    ImGui::PopStyleColor();
                }
            }
        }
    } else {
        ImGui::TextDisabled("Hover graph to see values");
    }

    // View bounds
    ImGui::Separator();
    ImGui::TextDisabled("[%.1f, %.1f] x [%.1f, %.1f]",
                        state.x_min, state.x_max, state.y_min, state.y_max);

    int vis = 0;
    for (auto& p : state.plots) if (p.visible && p.compiled) vis++;
    if (vis > 0) ImGui::TextDisabled("%d function%s", vis, vis == 1 ? "" : "s");

    ImGui::End();
}
