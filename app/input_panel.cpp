#include "input_panel.h"
#include "imgui.h"
#include "mathengine/parser.h"
#include "mathengine/simplify.h"
#include "mathengine/calculus.h"
#include "mathengine/solver.h"
#include "mathengine/algebra.h"
#include "mathengine/symbolic_solve.h"
#include "mathengine/taylor.h"
#include "mathengine/limits.h"
#include "mathengine/pretty.h"
#include "mathengine/steps.h"
#include "mathengine/linalg.h"
#include "mathengine/matrix.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ================================================================
// Helpers
// ================================================================

static void section_heading(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.75f, 1.0f, 1.0f));
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
    ImGui::Separator();
}

static void show_error(const std::string& msg) {
    if (!msg.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        ImGui::TextWrapped("%s", msg.c_str());
        ImGui::PopStyleColor();
    }
}

static void show_result(const std::string& text, const std::vector<std::string>& items = {}) {
    if (!text.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.65f, 1.0f));
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::PopStyleColor();
    }
    for (auto& r : items) {
        ImGui::BulletText("%s", r.c_str());
    }
}

static void show_steps(AppState& state) {
    if (state.step_log.empty()) return;
    ImGui::Spacing();
    ImGui::Checkbox("Show work", &state.show_steps);
    if (state.show_steps) {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::BeginChild("StepsView", ImVec2(0, std::min(200.0f, state.step_log.size() * 22.0f + 10.0f)),
                          ImGuiChildFlags_Border);
        for (size_t i = 0; i < state.step_log.size(); i++) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 0.9f, 1.0f));
            ImGui::Text("%d.", (int)(i + 1));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", state.step_log[i].c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
}

// ================================================================
// GRAPH TAB — Plot controls (left sidebar)
// ================================================================

void render_input_panel(AppState& state) {
    ImGui::Begin("Controls");

    // --- Plot management ---
    if (ImGui::Button("+ Add Function", ImVec2(-1, 0))) {
        PlotEntry entry;
        entry.id = state.next_plot_id++;
        entry.color = kPlotColors[state.plots.size() % kNumPlotColors];
        state.plots.push_back(std::move(entry));
    }

    ImGui::Spacing();

    // --- Plot list ---
    int remove_idx = -1;
    for (int i = 0; i < (int)state.plots.size(); i++) {
        auto& plot = state.plots[i];
        ImGui::PushID(plot.id);

        // Color bar on left
        float col4[3] = {
            ((plot.color >> 0) & 0xFF) / 255.0f,
            ((plot.color >> 8) & 0xFF) / 255.0f,
            ((plot.color >> 16) & 0xFF) / 255.0f
        };

        // Row: [vis] [color] [expr input] [X]
        ImGui::Checkbox("##v", &plot.visible);
        ImGui::SameLine();
        if (ImGui::ColorEdit3("##c", col4,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            plot.color = IM_COL32((int)(col4[0]*255), (int)(col4[1]*255), (int)(col4[2]*255), 255);
        }
        ImGui::SameLine();

        // Type as a compact dropdown
        const char* short_types[] = {"y=", "r=", "xy="};
        ImGui::SetNextItemWidth(42);
        int ti = (int)plot.type;
        if (ImGui::Combo("##t", &ti, short_types, 3)) {
            plot.type = (PlotEntry::Type)ti;
            plot.needs_resample = true;
            state.needs_resample = true;
            if (plot.expression[0]) try_parse_plot(plot);
        }
        ImGui::SameLine();

        const char* hint = (plot.type == PlotEntry::Type::Polar) ? "e.g. cos(3*theta)" :
                           (plot.type == PlotEntry::Type::Parametric) ? "x(t)" : "e.g. sin(x)";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 24);
        if (ImGui::InputTextWithHint("##e", hint, plot.expression, sizeof(plot.expression),
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
            try_parse_plot(plot);
            state.needs_resample = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) remove_idx = i;

        // Parametric second input
        if (plot.type == PlotEntry::Type::Parametric) {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputTextWithHint("##ey", "y(t)", plot.expression_y, sizeof(plot.expression_y),
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                try_parse_plot(plot);
                state.needs_resample = true;
            }
        }

        // Parameter sliders
        for (auto& param : plot.params) {
            char label[64];
            snprintf(label, sizeof(label), "%s", param.name.c_str());
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderScalar(("##p" + param.name).c_str(),
                    ImGuiDataType_Double, &param.value, &param.min_val, &param.max_val, label)) {
                plot.needs_resample = true;
                state.needs_resample = true;
            }
        }

        show_error(plot.error_msg);
        ImGui::Spacing();
        ImGui::PopID();
    }

    if (remove_idx >= 0) state.plots.erase(state.plots.begin() + remove_idx);

    // --- Tools ---
    if (ImGui::CollapsingHeader("Integral Shading")) {
        ImGui::Checkbox("Enabled##integ", &state.integral.enabled);
        if (state.integral.enabled && !state.plots.empty()) {
            int max_idx = (int)state.plots.size() - 1;
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderInt("Plot##i", &state.integral.plot_index, 0, max_idx);
            state.integral.plot_index = std::clamp(state.integral.plot_index, 0, max_idx);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::DragScalar("from", ImGuiDataType_Double, &state.integral.x_lo, 0.1f);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            ImGui::DragScalar("to", ImGuiDataType_Double, &state.integral.x_hi, 0.1f);
            ImGui::Text("Area: %.6f", state.integral.computed_area);
        }
    }

    if (ImGui::CollapsingHeader("Animation")) {
        ImGui::TextDisabled("Use 't' in expressions to animate");
        if (ImGui::Button(state.anim_playing ? "Pause" : "Play", ImVec2(60, 0))) {
            state.anim_playing = !state.anim_playing;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(50, 0))) {
            state.anim_time = state.anim_t_min;
            state.needs_resample = true;
        }
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderScalar("t##anim", ImGuiDataType_Double, &state.anim_time,
                                &state.anim_t_min, &state.anim_t_max)) {
            state.needs_resample = true;
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderScalar("Speed", ImGuiDataType_Double, &state.anim_speed,
                           &(const double&)(0.1), &(const double&)(5.0));
        ImGui::Text("t = %.3f", state.anim_time);
    }

    if (ImGui::CollapsingHeader("View Settings")) {
        ImGui::Checkbox("Crosshair", &state.crosshair_enabled);
        bool vc = false;
        ImGui::SetNextItemWidth(-1);
        vc |= ImGui::DragScalar("X min", ImGuiDataType_Double, &state.x_min, 0.1f);
        ImGui::SetNextItemWidth(-1);
        vc |= ImGui::DragScalar("X max", ImGuiDataType_Double, &state.x_max, 0.1f);
        ImGui::SetNextItemWidth(-1);
        vc |= ImGui::DragScalar("Y min", ImGuiDataType_Double, &state.y_min, 0.1f);
        ImGui::SetNextItemWidth(-1);
        vc |= ImGui::DragScalar("Y max", ImGuiDataType_Double, &state.y_max, 0.1f);
        if (vc) state.needs_resample = true;
        if (ImGui::Button("Reset View", ImVec2(-1, 0))) {
            state.x_min = -10; state.x_max = 10;
            state.y_min = -5; state.y_max = 5;
            state.needs_resample = true;
        }

        // Polar/parametric ranges only if needed
        bool has_polar = false, has_para = false;
        for (auto& p : state.plots) {
            if (p.type == PlotEntry::Type::Polar) has_polar = true;
            if (p.type == PlotEntry::Type::Parametric) has_para = true;
        }
        if (has_polar) {
            ImGui::Spacing();
            ImGui::Text("Polar:");
            bool c = false;
            c |= ImGui::DragScalar("theta min", ImGuiDataType_Double, &state.theta_min, 0.1f);
            c |= ImGui::DragScalar("theta max", ImGuiDataType_Double, &state.theta_max, 0.1f);
            if (c) state.needs_resample = true;
        }
        if (has_para) {
            ImGui::Spacing();
            ImGui::Text("Parametric:");
            bool c = false;
            c |= ImGui::DragScalar("t min", ImGuiDataType_Double, &state.t_min, 0.1f);
            c |= ImGui::DragScalar("t max", ImGuiDataType_Double, &state.t_max, 0.1f);
            if (c) state.needs_resample = true;
        }
    }

    if (ImGui::CollapsingHeader("Quick Add")) {
        const char* examples[] = {
            "sin(x)", "cos(x)", "x^2", "exp(-x^2)",
            "tan(x)", "x^3 - 3x", "sqrt(abs(x))", "sin(x)/x",
        };
        float w = (ImGui::GetContentRegionAvail().x - 6) / 2;
        for (int i = 0; i < 8; i++) {
            if (i % 2 != 0) ImGui::SameLine();
            if (ImGui::Button(examples[i], ImVec2(w, 0))) {
                PlotEntry entry;
                entry.id = state.next_plot_id++;
                entry.color = kPlotColors[state.plots.size() % kNumPlotColors];
                snprintf(entry.expression, sizeof(entry.expression), "%s", examples[i]);
                try_parse_plot(entry);
                state.plots.push_back(std::move(entry));
                state.needs_resample = true;
            }
        }
    }

    // Export at bottom
    ImGui::Spacing();
    if (ImGui::Button("Export PNG", ImVec2(-1, 0))) {
        state.export_requested = true;
    }

    ImGui::End();
}

// ================================================================
// CALCULATOR TAB — Unified CAS workspace
// ================================================================

static void render_calculator_tab(AppState& state) {
    ImGui::BeginChild("CalcArea", ImVec2(0, 0));
    float padding = 16;
    ImGui::SetCursorPosX(padding);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

    // Max width for content
    float content_w = std::min(ImGui::GetContentRegionAvail().x - padding * 2, 700.0f);
    ImGui::BeginChild("CalcInner", ImVec2(content_w, 0));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

    // --- Expression input ---
    ImGui::Text("Expression");
    ImGui::SetNextItemWidth(-1);
    bool enter = ImGui::InputTextWithHint("##cas", "e.g. x^2 + 2*x - 8",
        state.expr_input, sizeof(state.expr_input), ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::Spacing();

    // --- Operation buttons in a clean grid ---
    section_heading("Algebra");
    float bw = (ImGui::GetContentRegionAvail().x - 12) / 3;
    bool do_simplify = ImGui::Button("Simplify", ImVec2(bw, 28)); ImGui::SameLine();
    bool do_expand   = ImGui::Button("Expand", ImVec2(bw, 28)); ImGui::SameLine();
    bool do_factor   = ImGui::Button("Factor", ImVec2(bw, 28));

    section_heading("Calculus");
    bool do_diff      = ImGui::Button("Derivative", ImVec2(bw, 28)); ImGui::SameLine();
    bool do_integrate = ImGui::Button("Integral", ImVec2(bw, 28)); ImGui::SameLine();
    bool do_taylor    = false;
    bool do_limit     = false;

    // Taylor and Limit need extra params, so they get a sub-row
    {
        // Taylor button + inline params
        ImGui::BeginGroup();
        do_taylor = ImGui::Button("Taylor", ImVec2(bw, 28));
        ImGui::EndGroup();
    }

    ImGui::SetNextItemWidth(bw * 0.45f);
    ImGui::InputText("center##t", state.taylor_center, sizeof(state.taylor_center));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(bw * 0.35f);
    ImGui::InputText("order##t", state.taylor_order, sizeof(state.taylor_order));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(bw * 0.45f);
    ImGui::InputText("point##l", state.limit_point, sizeof(state.limit_point));
    ImGui::SameLine();
    do_limit = ImGui::Button("Limit", ImVec2(bw * 0.5f, 0));

    section_heading("Solve");
    bw = (ImGui::GetContentRegionAvail().x - 8) / 2;
    bool do_solve = ImGui::Button("Solve f(x) = 0", ImVec2(bw, 28)); ImGui::SameLine();
    bool do_numeric = ImGui::Button("Numeric Root", ImVec2(bw, 28));

    if (do_numeric) {
        ImGui::SetNextItemWidth(100);
        ImGui::InputText("a##s", state.solver_a, sizeof(state.solver_a));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputText("b##s", state.solver_b, sizeof(state.solver_b));
    }

    // Systems
    ImGui::Spacing();
    bool do_system = false;
    if (ImGui::CollapsingHeader("System of Equations")) {
        ImGui::TextDisabled("Separate equations with ;");
        ImGui::InputTextMultiline("##sys", state.system_input, sizeof(state.system_input),
                                  ImVec2(-1, 60));
        do_system = ImGui::Button("Solve System", ImVec2(-1, 28));
    }

    // --- Process the clicked operation ---
    bool any_op = enter || do_simplify || do_expand || do_factor || do_diff ||
                  do_integrate || do_taylor || do_limit || do_solve || do_numeric;

    if (any_op && !do_system) {
        try_parse(state);
        state.solve_results.clear();
        state.step_log.clear();

        if (state.parsed_expr) {
            try {
                if (do_simplify || enter) {
                    auto r = mathengine::simplify(state.parsed_expr);
                    state.result_text = mathengine::pretty_string(r);
                    if (do_simplify) {
                        mathengine::StepLog steps;
                        mathengine::simplify_steps(state.parsed_expr, steps);
                        for (auto& s : steps) state.step_log.push_back(s.rule + ": " + s.expression);
                    }
                }
                if (do_expand) {
                    auto r = mathengine::expand(state.parsed_expr);
                    state.result_text = mathengine::pretty_string(r);
                }
                if (do_factor) {
                    auto r = mathengine::factor(state.parsed_expr, "x");
                    state.result_text = mathengine::pretty_string(r);
                }
                if (do_diff) {
                    mathengine::StepLog steps;
                    auto r = mathengine::differentiate_steps(state.parsed_expr, "x", steps);
                    state.result_text = "d/dx = " + mathengine::pretty_string(r);
                    for (auto& s : steps) state.step_log.push_back(s.rule + ": " + s.expression);
                }
                if (do_integrate) {
                    auto r = mathengine::integrate(state.parsed_expr, "x");
                    state.result_text = r ? (mathengine::pretty_string(r) + " + C") :
                                            "Cannot integrate symbolically";
                }
                if (do_taylor) {
                    double c = std::atof(state.taylor_center);
                    int o = std::atoi(state.taylor_order);
                    auto r = mathengine::taylor(state.parsed_expr, "x", c, o);
                    state.result_text = mathengine::pretty_string(r);
                }
                if (do_limit) {
                    double p = std::atof(state.limit_point);
                    auto r = mathengine::limit(state.parsed_expr, "x", p, state.limit_direction);
                    state.result_text = r ? ("lim = " + mathengine::pretty_string(r)) :
                                            "Limit does not exist or cannot be computed";
                }
                if (do_solve) {
                    mathengine::StepLog steps;
                    auto roots = mathengine::solve_steps(state.parsed_expr, "x", steps);
                    for (auto& s : steps) state.step_log.push_back(s.rule + ": " + s.expression);
                    state.result_text = roots.empty() ? "No solutions found" : "Solutions:";
                    for (auto& r : roots)
                        state.solve_results.push_back("x = " + mathengine::pretty_string(r));
                }
                if (do_numeric && state.compiled_expr) {
                    double a = std::atof(state.solver_a), b = std::atof(state.solver_b);
                    auto r = mathengine::bisect(*state.compiled_expr, a, b);
                    state.result_text = r.converged ?
                        ("Root: x = " + std::to_string(r.root)) : "Did not converge";
                }
            } catch (const std::exception& e) {
                state.result_text = std::string("Error: ") + e.what();
            }
        }
    }

    if (do_system) {
        state.solve_results.clear();
        state.step_log.clear();
        try {
            auto sol = mathengine::solve_linear_system(state.system_input);
            if (sol.solved) {
                state.result_text = "Solution:";
                for (size_t i = 0; i < sol.var_names.size(); i++) {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%s = %.10g", sol.var_names[i].c_str(), sol.values[i]);
                    state.solve_results.push_back(buf);
                }
            } else {
                state.result_text = "Error: " + sol.error;
            }
        } catch (const std::exception& e) {
            state.result_text = std::string("Error: ") + e.what();
        }
    }

    // --- Results ---
    show_error(state.error_msg);
    show_result(state.result_text, state.solve_results);
    show_steps(state);

    // --- Examples ---
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Examples")) {
        const char* exs[] = {
            "(x+1)*(x-1)", "x^2 + 2*x - 8", "sin(x)", "x*exp(x)",
            "x^3 - 6*x^2 + 11*x - 6", "sin(x)/x", "(x+2)^3", "1/(x^2+1)",
        };
        float ew = (ImGui::GetContentRegionAvail().x - 6) / 2;
        for (int i = 0; i < 8; i++) {
            if (i % 2 != 0) ImGui::SameLine();
            if (ImGui::Button(exs[i], ImVec2(ew, 0)))
                snprintf(state.expr_input, sizeof(state.expr_input), "%s", exs[i]);
        }
    }

    ImGui::PopStyleVar();
    ImGui::EndChild(); // CalcInner
    ImGui::EndChild(); // CalcArea
}

// ================================================================
// MATRIX TAB
// ================================================================

static mathengine::Matrix build_matrix(double data[8][8], int rows, int cols) {
    mathengine::Matrix m(rows, cols);
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            m(r, c) = data[r][c];
    return m;
}

static void render_matrix_grid(const char* label, double data[8][8], int& rows, int& cols) {
    ImGui::Text("%s", label);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 180);
    ImGui::SetNextItemWidth(60);
    ImGui::PushID(label);
    ImGui::InputInt("rows", &rows, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("cols", &cols, 0);
    rows = std::clamp(rows, 1, 8);
    cols = std::clamp(cols, 1, 8);

    float cell_w = std::min(70.0f, (ImGui::GetContentRegionAvail().x - cols * 4.0f) / cols);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            ImGui::PushID(r * 8 + c);
            ImGui::SetNextItemWidth(cell_w);
            ImGui::InputDouble("##c", &data[r][c], 0, 0, "%.4g");
            if (c < cols - 1) ImGui::SameLine();
            ImGui::PopID();
        }
    }
    ImGui::PopID();
}

static void render_matrix_tab(AppState& state) {
    ImGui::BeginChild("MatArea", ImVec2(0, 0));
    float padding = 16;
    ImGui::SetCursorPosX(padding);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
    float content_w = std::min(ImGui::GetContentRegionAvail().x - padding * 2, 750.0f);
    ImGui::BeginChild("MatInner", ImVec2(content_w, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));

    render_matrix_grid("Matrix A", state.mat_data, state.mat_rows, state.mat_cols);

    ImGui::Spacing();
    render_matrix_grid("Matrix B", state.mat_b_data, state.mat_b_rows, state.mat_b_cols);

    section_heading("Operations");
    float bw = (ImGui::GetContentRegionAvail().x - 18) / 4;
    bool ops[8] = {};
    ops[0] = ImGui::Button("Determinant", ImVec2(bw, 28)); ImGui::SameLine();
    ops[1] = ImGui::Button("Inverse", ImVec2(bw, 28)); ImGui::SameLine();
    ops[2] = ImGui::Button("Transpose", ImVec2(bw, 28)); ImGui::SameLine();
    ops[3] = ImGui::Button("Eigenvalues", ImVec2(bw, 28));
    ops[4] = ImGui::Button("A * B", ImVec2(bw, 28)); ImGui::SameLine();
    ops[5] = ImGui::Button("Solve Ax=b", ImVec2(bw, 28)); ImGui::SameLine();
    ops[6] = ImGui::Button("LU Decomp", ImVec2(bw, 28)); ImGui::SameLine();
    ops[7] = ImGui::Button("QR Decomp", ImVec2(bw, 28));

    for (int i = 0; i < 8; i++) {
        if (!ops[i]) continue;
        try {
            auto A = build_matrix(state.mat_data, state.mat_rows, state.mat_cols);
            switch (i) {
                case 0: state.mat_result_text = "det(A) = " + std::to_string(mathengine::determinant(A)); break;
                case 1: state.mat_result_text = "A^(-1) =\n" + mathengine::inverse(A).to_string(); break;
                case 2: state.mat_result_text = "A^T =\n" + A.transpose().to_string(); break;
                case 3: {
                    auto ev = mathengine::eigenvalues(A);
                    state.mat_result_text = "Eigenvalues:\n";
                    for (size_t j = 0; j < ev.eigenvalues.size(); j++)
                        state.mat_result_text += "  lambda_" + std::to_string(j+1) + " = " +
                                                 std::to_string(ev.eigenvalues[j]) + "\n";
                    break;
                }
                case 4: {
                    auto B = build_matrix(state.mat_b_data, state.mat_b_rows, state.mat_b_cols);
                    state.mat_result_text = "A*B =\n" + (A * B).to_string();
                    break;
                }
                case 5: {
                    auto b = build_matrix(state.mat_b_data, state.mat_b_rows, state.mat_b_cols);
                    state.mat_result_text = "x =\n" + mathengine::solve(A, b).to_string();
                    break;
                }
                case 6: {
                    auto [L, U, p] = mathengine::lu_decompose(A);
                    state.mat_result_text = "L =\n" + L.to_string() + "\nU =\n" + U.to_string();
                    break;
                }
                case 7: {
                    auto [Q, R] = mathengine::qr_decompose(A);
                    state.mat_result_text = "Q =\n" + Q.to_string() + "\nR =\n" + R.to_string();
                    break;
                }
            }
        } catch (const std::exception& e) {
            state.mat_result_text = std::string("Error: ") + e.what();
        }
    }

    // Result
    if (!state.mat_result_text.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextWrapped("%s", state.mat_result_text.c_str());
    }

    // Presets
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Presets")) {
        if (ImGui::SmallButton("Identity 3x3")) {
            state.mat_rows = state.mat_cols = 3;
            memset(state.mat_data, 0, sizeof(state.mat_data));
            state.mat_data[0][0] = state.mat_data[1][1] = state.mat_data[2][2] = 1;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("2x2 Example")) {
            state.mat_rows = state.mat_cols = 2;
            state.mat_data[0][0]=1; state.mat_data[0][1]=2;
            state.mat_data[1][0]=3; state.mat_data[1][1]=4;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Symmetric 3x3")) {
            state.mat_rows = state.mat_cols = 3;
            state.mat_data[0][0]=2; state.mat_data[0][1]=1; state.mat_data[0][2]=0;
            state.mat_data[1][0]=1; state.mat_data[1][1]=3; state.mat_data[1][2]=1;
            state.mat_data[2][0]=0; state.mat_data[2][1]=1; state.mat_data[2][2]=2;
        }
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::EndChild();
}

// ================================================================
// CAS tab router
// ================================================================

void render_cas_tab(AppState& state, ImVec2 content_size) {
    ImGui::BeginChild("CASRoot", content_size);
    if (state.active_tab == AppState::Tab::Calculator)
        render_calculator_tab(state);
    else if (state.active_tab == AppState::Tab::Matrix)
        render_matrix_tab(state);
    ImGui::EndChild();
}
