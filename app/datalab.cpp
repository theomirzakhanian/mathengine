#include "datalab.h"
#include "mathengine/design.h"
#include "mathengine/statistics.h"
#include "mathengine/pretty.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <cmath>
#include <algorithm>

static void parse_csv(AppState& state) {
    state.data_x.clear();
    state.data_y.clear();
    std::istringstream ss(state.csv_input);
    std::string line;
    bool first_line = true;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        // Skip header if it contains non-numeric chars
        if (first_line) {
            first_line = false;
            bool has_alpha = false;
            for (char c : line) if (std::isalpha(c)) { has_alpha = true; break; }
            if (has_alpha) continue;
        }
        size_t comma = line.find(',');
        if (comma == std::string::npos) continue;
        double x = std::atof(line.substr(0, comma).c_str());
        double y = std::atof(line.substr(comma + 1).c_str());
        state.data_x.push_back(x);
        state.data_y.push_back(y);
    }
    state.data_loaded = !state.data_x.empty();
}

void render_data_tab(AppState& state, ImVec2 content_size) {
    ImGui::BeginChild("DataArea", content_size);
    bool dark = (state.theme == AppState::Theme::Dark);

    float panel_w = 300;

    // --- Left panel ---
    ImGui::BeginChild("DataControls", ImVec2(panel_w, 0), ImGuiChildFlags_Border);
    ImGui::Text("Data Lab");
    ImGui::TextDisabled("Paste CSV data, fit curves");
    ImGui::Separator();

    ImGui::Text("CSV Data (x, y):");
    ImGui::InputTextMultiline("##csv", state.csv_input, sizeof(state.csv_input), ImVec2(-1, 120));

    if (ImGui::Button("Load Data", ImVec2(-1, 0))) {
        parse_csv(state);
    }

    if (state.data_loaded) {
        ImGui::Text("%d points loaded", (int)state.data_x.size());

        // Stats
        if (state.data_y.size() >= 2) {
            ImGui::TextDisabled("y: mean=%.4g, std=%.4g",
                mathengine::stat_mean(state.data_y), mathengine::stat_stddev(state.data_y));
        }

        ImGui::Separator();
        ImGui::Text("Regression:");
        ImGui::SliderInt("Degree##fit", &state.data_fit_degree, 1, 10);

        if (ImGui::Button("Fit Curve", ImVec2(-1, 28))) {
            std::vector<mathengine::Point2D> pts;
            for (size_t i = 0; i < state.data_x.size(); i++)
                pts.push_back({state.data_x[i], state.data_y[i]});

            auto poly = mathengine::poly_fit(pts, state.data_fit_degree);
            auto expr = poly.to_expr("x");
            state.data_fit_result = "y = " + mathengine::pretty_string(expr);
            state.data_fit_desmos = mathengine::poly_to_desmos(poly);

            state.data_fit_coeffs.resize(poly.degree() + 1);
            for (int i = 0; i <= poly.degree(); i++)
                state.data_fit_coeffs[i] = poly.coeff(i);

            // R-squared
            double y_mean = mathengine::stat_mean(state.data_y);
            double ss_tot = 0, ss_res = 0;
            for (size_t i = 0; i < state.data_x.size(); i++) {
                double pred = poly.eval(state.data_x[i]);
                ss_res += (state.data_y[i] - pred) * (state.data_y[i] - pred);
                ss_tot += (state.data_y[i] - y_mean) * (state.data_y[i] - y_mean);
            }
            double r2 = (ss_tot > 1e-15) ? 1.0 - ss_res / ss_tot : 1.0;
            char buf[128];
            snprintf(buf, sizeof(buf), "\nR^2 = %.6f", r2);
            state.data_fit_result += buf;
        }

        if (!state.data_fit_result.empty()) {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.65f, 1.0f));
            ImGui::TextWrapped("%s", state.data_fit_result.c_str());
            ImGui::PopStyleColor();

            if (ImGui::Button("Copy Desmos", ImVec2(-1, 0))) {
                ImGui::SetClipboardText(("y=" + state.data_fit_desmos).c_str());
            }
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // --- Right panel: scatter plot ---
    ImGui::BeginChild("DataPlot", ImVec2(0, 0), ImGuiChildFlags_Border);
    ImVec2 plot_pos = ImGui::GetCursorScreenPos();
    ImVec2 plot_size = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 bg = dark ? IM_COL32(25, 25, 30, 255) : IM_COL32(245, 245, 250, 255);
    dl->AddRectFilled(plot_pos, ImVec2(plot_pos.x + plot_size.x, plot_pos.y + plot_size.y), bg);

    if (state.data_loaded && state.data_x.size() >= 2) {
        float px = plot_pos.x, py = plot_pos.y, pw = plot_size.x, ph = plot_size.y;
        float margin = 40;

        double xlo = *std::min_element(state.data_x.begin(), state.data_x.end());
        double xhi = *std::max_element(state.data_x.begin(), state.data_x.end());
        double ylo = *std::min_element(state.data_y.begin(), state.data_y.end());
        double yhi = *std::max_element(state.data_y.begin(), state.data_y.end());
        double xpad = (xhi - xlo) * 0.1 + 0.01;
        double ypad = (yhi - ylo) * 0.1 + 0.01;
        xlo -= xpad; xhi += xpad; ylo -= ypad; yhi += ypad;

        auto sx = [&](double x) { return px + margin + (float)((x - xlo) / (xhi - xlo)) * (pw - 2*margin); };
        auto sy = [&](double y) { return py + ph - margin - (float)((y - ylo) / (yhi - ylo)) * (ph - 2*margin); };

        // Axes
        ImU32 axis_col = dark ? IM_COL32(80, 80, 100, 255) : IM_COL32(140, 140, 160, 255);
        dl->AddLine(ImVec2(px+margin, py+ph-margin), ImVec2(px+pw-margin, py+ph-margin), axis_col);
        dl->AddLine(ImVec2(px+margin, py+margin), ImVec2(px+margin, py+ph-margin), axis_col);

        // Axis labels
        ImU32 tc = dark ? IM_COL32(140, 140, 160, 255) : IM_COL32(80, 80, 100, 255);
        char lbl[32];
        snprintf(lbl, 32, "%.3g", xlo); dl->AddText(ImVec2(sx(xlo), py+ph-margin+4), tc, lbl);
        snprintf(lbl, 32, "%.3g", xhi); dl->AddText(ImVec2(sx(xhi)-30, py+ph-margin+4), tc, lbl);
        snprintf(lbl, 32, "%.3g", ylo); dl->AddText(ImVec2(px+2, sy(ylo)-6), tc, lbl);
        snprintf(lbl, 32, "%.3g", yhi); dl->AddText(ImVec2(px+2, sy(yhi)-6), tc, lbl);

        // Scatter points
        ImU32 dot_col = IM_COL32(255, 200, 60, 255);
        for (size_t i = 0; i < state.data_x.size(); i++) {
            dl->AddCircleFilled(ImVec2(sx(state.data_x[i]), sy(state.data_y[i])), 4.0f, dot_col);
        }

        // Regression curve
        if (!state.data_fit_coeffs.empty()) {
            mathengine::Polynomial poly(state.data_fit_coeffs);
            ImU32 fit_col = IM_COL32(80, 200, 255, 255);
            float prev_fx = 0, prev_fy = 0;
            bool first = true;
            for (int i = 0; i < (int)(pw - 2*margin); i += 2) {
                double x = xlo + (xhi - xlo) * i / (pw - 2*margin);
                double y = poly.eval(x);
                float fx = sx(x), fy = sy(y);
                if (fy < py || fy > py + ph) { first = true; continue; }
                if (!first) dl->AddLine(ImVec2(prev_fx, prev_fy), ImVec2(fx, fy), fit_col, 2.0f);
                prev_fx = fx; prev_fy = fy; first = false;
            }
        }
    } else {
        ImU32 hint = dark ? IM_COL32(100, 100, 120, 200) : IM_COL32(120, 120, 140, 200);
        dl->AddText(ImVec2(plot_pos.x + plot_size.x/2 - 60, plot_pos.y + plot_size.y/2),
                   hint, "Load CSV data to plot");
    }

    ImGui::EndChild();
    ImGui::EndChild();
}
