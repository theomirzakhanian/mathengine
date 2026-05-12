#include "renderer3d.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

namespace {

struct Vec3 { double x, y, z; };
struct Vec2 { float x, y; };

Vec3 rotate_y(Vec3 p, double angle) {
    double c = std::cos(angle), s = std::sin(angle);
    return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}

Vec3 rotate_x(Vec3 p, double angle) {
    double c = std::cos(angle), s = std::sin(angle);
    return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
}

Vec2 project(Vec3 p, double dist, float cx, float cy, float scale) {
    double d = dist + p.z;
    if (d < 0.1) d = 0.1;
    float factor = (float)(dist / d) * scale;
    return {cx + (float)p.x * factor, cy - (float)p.y * factor};
}

// Color based on height (z value)
ImU32 height_color(double z, double z_min, double z_max, bool dark) {
    if (z_max - z_min < 1e-10) return IM_COL32(100, 150, 255, 255);
    double t = (z - z_min) / (z_max - z_min);
    t = std::clamp(t, 0.0, 1.0);
    // Blue -> Cyan -> Green -> Yellow -> Red
    int r, g, b;
    if (t < 0.25) {
        double s = t / 0.25;
        r = 0; g = (int)(s * 255); b = 255;
    } else if (t < 0.5) {
        double s = (t - 0.25) / 0.25;
        r = 0; g = 255; b = (int)((1 - s) * 255);
    } else if (t < 0.75) {
        double s = (t - 0.5) / 0.25;
        r = (int)(s * 255); g = 255; b = 0;
    } else {
        double s = (t - 0.75) / 0.25;
        r = 255; g = (int)((1 - s) * 255); b = 0;
    }
    return IM_COL32(r, g, b, dark ? 200 : 220);
}

} // anonymous namespace

void render_3d_tab(AppState& state, ImVec2 content_size) {
    ImGui::BeginChild("3DArea", content_size);

    // Left panel: controls
    float panel_w = 280;
    ImGui::BeginChild("3DControls", ImVec2(panel_w, 0), ImGuiChildFlags_Border);

    ImGui::Text("3D Surface Plot");
    ImGui::Separator();

    ImGui::Text("z = f(x, y):");
    bool enter = ImGui::InputTextWithHint("##surf_expr", "e.g. sin(x)*cos(y)",
        state.surface_expr, sizeof(state.surface_expr), ImGuiInputTextFlags_EnterReturnsTrue);
    if (enter || ImGui::Button("Plot")) {
        try {
            state.surface_parsed = mathengine::parse(state.surface_expr);
            state.surface_compiled = std::make_unique<mathengine::CompiledExpr>(state.surface_parsed);
            state.surface_error.clear();
            state.surface_dirty = true;
        } catch (const std::exception& e) {
            state.surface_error = e.what();
            state.surface_compiled = nullptr;
        }
    }

    if (!state.surface_error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.3f, 0.3f, 1));
        ImGui::TextWrapped("%s", state.surface_error.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::Text("View Range:");
    bool range_changed = false;
    range_changed |= ImGui::DragScalar("X range", ImGuiDataType_Double, &state.surf_range, 0.1f);
    range_changed |= ImGui::DragScalar("Y range", ImGuiDataType_Double, &state.surf_range_y, 0.1f);
    if (range_changed) state.surface_dirty = true;

    ImGui::Separator();
    ImGui::Text("Resolution:");
    if (ImGui::SliderInt("Grid", &state.surf_resolution, 10, 80))
        state.surface_dirty = true;

    ImGui::Separator();
    ImGui::Checkbox("Wireframe", &state.surf_wireframe);
    ImGui::Checkbox("Filled", &state.surf_filled);
    ImGui::Checkbox("Contours", &state.surf_contours);

    ImGui::Separator();
    ImGui::Text("Camera:");
    ImGui::SliderScalar("Distance", ImGuiDataType_Double, &state.cam_dist,
                        &(const double&)(1.0), &(const double&)(20.0));

    ImGui::Separator();
    ImGui::Text("Examples:");
    const char* examples_3d[] = {
        "sin(x)*cos(y)", "x^2 + y^2", "sin(sqrt(x^2+y^2))",
        "exp(-(x^2+y^2))", "x*y", "cos(x)*sin(y)*exp(-(x^2+y^2)/10)",
    };
    for (auto* ex : examples_3d) {
        if (ImGui::SmallButton(ex)) {
            snprintf(state.surface_expr, sizeof(state.surface_expr), "%s", ex);
            try {
                state.surface_parsed = mathengine::parse(state.surface_expr);
                state.surface_compiled = std::make_unique<mathengine::CompiledExpr>(state.surface_parsed);
                state.surface_error.clear();
                state.surface_dirty = true;
            } catch (const std::exception& e) {
                state.surface_error = e.what();
            }
        }
    }

    ImGui::EndChild(); // 3DControls

    ImGui::SameLine();

    // Right panel: 3D viewport
    ImGui::BeginChild("3DViewport", ImVec2(0, 0), ImGuiChildFlags_Border);
    ImVec2 vp_pos = ImGui::GetCursorScreenPos();
    ImVec2 vp_size = ImGui::GetContentRegionAvail();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    bool dark = (state.theme == AppState::Theme::Dark);

    // Background
    ImU32 bg = dark ? IM_COL32(20, 20, 25, 255) : IM_COL32(240, 240, 245, 255);
    draw_list->AddRectFilled(vp_pos, ImVec2(vp_pos.x + vp_size.x, vp_pos.y + vp_size.y), bg);

    float cx = vp_pos.x + vp_size.x / 2;
    float cy = vp_pos.y + vp_size.y / 2;
    float view_scale = std::min(vp_size.x, vp_size.y) * 0.08f;

    // Mouse interaction for rotation
    ImGui::SetCursorScreenPos(vp_pos);
    ImGui::InvisibleButton("3d_viewport", vp_size);
    bool vp_hovered = ImGui::IsItemHovered();

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        state.cam_yaw += delta.x * 0.01;
        state.cam_pitch += delta.y * 0.01;
        state.cam_pitch = std::clamp(state.cam_pitch, -1.5, 1.5);
    }
    if (vp_hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            state.cam_dist *= (wheel > 0) ? 0.9 : 1.1;
            state.cam_dist = std::clamp(state.cam_dist, 1.0, 30.0);
        }
    }

    if (state.surface_compiled) {
        int N = state.surf_resolution;
        double rx = state.surf_range, ry = state.surf_range_y;

        // Resample if dirty
        if (state.surface_dirty) {
            state.surface_dirty = false;
            state.surf_z.resize(N * N);
            state.surf_z_min = 1e30;
            state.surf_z_max = -1e30;

            for (int iy = 0; iy < N; iy++) {
                for (int ix = 0; ix < N; ix++) {
                    double x = -rx + 2.0 * rx * ix / (N - 1);
                    double y = -ry + 2.0 * ry * iy / (N - 1);
                    double z = state.surface_compiled->eval(
                        mathengine::VarMap{{"x", x}, {"y", y}});
                    if (std::isnan(z) || std::isinf(z)) z = 0;
                    state.surf_z[iy * N + ix] = z;
                    state.surf_z_min = std::min(state.surf_z_min, z);
                    state.surf_z_max = std::max(state.surf_z_max, z);
                }
            }
        }

        // Z normalization for display
        double z_range = state.surf_z_max - state.surf_z_min;
        if (z_range < 1e-10) z_range = 1.0;
        double z_mid = (state.surf_z_max + state.surf_z_min) / 2.0;
        double z_scale = rx / z_range * 2.0; // normalize z to similar range as x/y

        // Build projected points
        struct ProjPoint { Vec2 screen; double z_world; double depth; };
        std::vector<ProjPoint> points(N * N);

        for (int iy = 0; iy < N; iy++) {
            for (int ix = 0; ix < N; ix++) {
                double x = -rx + 2.0 * rx * ix / (N - 1);
                double y = -ry + 2.0 * ry * iy / (N - 1);
                double z = state.surf_z[iy * N + ix];
                double zn = (z - z_mid) * z_scale;

                Vec3 p = {x, zn, y}; // y-up coordinate system
                p = rotate_y(p, state.cam_yaw);
                p = rotate_x(p, state.cam_pitch);

                auto screen = project(p, state.cam_dist, cx, cy, view_scale);
                points[iy * N + ix] = {screen, z, p.z};
            }
        }

        // Draw filled quads (back-to-front using painter's algorithm)
        if (state.surf_filled) {
            struct Quad { int i00, i10, i11, i01; double avg_depth; };
            std::vector<Quad> quads;
            quads.reserve((N-1)*(N-1));
            for (int iy = 0; iy < N - 1; iy++) {
                for (int ix = 0; ix < N - 1; ix++) {
                    int i00 = iy*N+ix, i10 = iy*N+ix+1;
                    int i01 = (iy+1)*N+ix, i11 = (iy+1)*N+ix+1;
                    double avg = (points[i00].depth + points[i10].depth +
                                  points[i11].depth + points[i01].depth) / 4.0;
                    quads.push_back({i00, i10, i11, i01, avg});
                }
            }
            std::sort(quads.begin(), quads.end(), [](const Quad& a, const Quad& b) {
                return a.avg_depth > b.avg_depth;
            });

            for (auto& q : quads) {
                double avg_z = (points[q.i00].z_world + points[q.i10].z_world +
                               points[q.i11].z_world + points[q.i01].z_world) / 4.0;
                ImU32 col = height_color(avg_z, state.surf_z_min, state.surf_z_max, dark);
                ImVec2 verts[4] = {
                    {points[q.i00].screen.x, points[q.i00].screen.y},
                    {points[q.i10].screen.x, points[q.i10].screen.y},
                    {points[q.i11].screen.x, points[q.i11].screen.y},
                    {points[q.i01].screen.x, points[q.i01].screen.y},
                };
                draw_list->AddConvexPolyFilled(verts, 4, col);
            }
        }

        // Draw wireframe
        if (state.surf_wireframe) {
            ImU32 wire_col = dark ? IM_COL32(180, 180, 200, 100) : IM_COL32(40, 40, 60, 100);
            for (int iy = 0; iy < N; iy++) {
                for (int ix = 0; ix < N - 1; ix++) {
                    auto& a = points[iy*N+ix].screen;
                    auto& b = points[iy*N+ix+1].screen;
                    draw_list->AddLine({a.x, a.y}, {b.x, b.y}, wire_col, 1.0f);
                }
            }
            for (int ix = 0; ix < N; ix++) {
                for (int iy = 0; iy < N - 1; iy++) {
                    auto& a = points[iy*N+ix].screen;
                    auto& b = points[(iy+1)*N+ix].screen;
                    draw_list->AddLine({a.x, a.y}, {b.x, b.y}, wire_col, 1.0f);
                }
            }
        }

        // Draw contour lines
        if (state.surf_contours) {
            int num_contours = 10;
            ImU32 contour_col = dark ? IM_COL32(255, 255, 255, 140) : IM_COL32(0, 0, 0, 140);
            for (int c = 1; c < num_contours; c++) {
                double level = state.surf_z_min + (state.surf_z_max - state.surf_z_min) * c / num_contours;
                // March through quads looking for contour crossings
                for (int iy = 0; iy < N - 1; iy++) {
                    for (int ix = 0; ix < N - 1; ix++) {
                        int idx[4] = {iy*N+ix, iy*N+ix+1, (iy+1)*N+ix+1, (iy+1)*N+ix};
                        double z[4];
                        for (int k = 0; k < 4; k++) z[k] = state.surf_z[idx[k]];

                        // Find edge crossings
                        std::vector<Vec2> crossings;
                        for (int k = 0; k < 4; k++) {
                            int k2 = (k + 1) % 4;
                            if ((z[k] - level) * (z[k2] - level) < 0) {
                                double t = (level - z[k]) / (z[k2] - z[k]);
                                float sx = points[idx[k]].screen.x + t * (points[idx[k2]].screen.x - points[idx[k]].screen.x);
                                float sy = points[idx[k]].screen.y + t * (points[idx[k2]].screen.y - points[idx[k]].screen.y);
                                crossings.push_back({sx, sy});
                            }
                        }
                        if (crossings.size() >= 2) {
                            draw_list->AddLine(
                                {crossings[0].x, crossings[0].y},
                                {crossings[1].x, crossings[1].y},
                                contour_col, 1.5f);
                        }
                    }
                }
            }
        }

        // Axes
        ImU32 axis_col = dark ? IM_COL32(255, 80, 80, 200) : IM_COL32(200, 0, 0, 200);
        ImU32 axis_col_y = dark ? IM_COL32(80, 255, 80, 200) : IM_COL32(0, 180, 0, 200);
        ImU32 axis_col_z = dark ? IM_COL32(80, 80, 255, 200) : IM_COL32(0, 0, 200, 200);
        double alen = rx * 1.2;
        auto draw_axis = [&](Vec3 from, Vec3 to, ImU32 col, const char* label) {
            Vec3 f = rotate_y(from, state.cam_yaw); f = rotate_x(f, state.cam_pitch);
            Vec3 t = rotate_y(to, state.cam_yaw); t = rotate_x(t, state.cam_pitch);
            auto sf = project(f, state.cam_dist, cx, cy, view_scale);
            auto st = project(t, state.cam_dist, cx, cy, view_scale);
            draw_list->AddLine({sf.x, sf.y}, {st.x, st.y}, col, 2.0f);
            draw_list->AddText({st.x + 4, st.y - 10}, col, label);
        };
        draw_axis({0,0,0}, {alen,0,0}, axis_col, "X");
        draw_axis({0,0,0}, {0,alen,0}, axis_col_y, "Z");
        draw_axis({0,0,0}, {0,0,alen}, axis_col_z, "Y");

        // Info
        char info[128];
        snprintf(info, sizeof(info), "z range: [%.3g, %.3g]", state.surf_z_min, state.surf_z_max);
        ImU32 info_col = dark ? IM_COL32(200,200,210,200) : IM_COL32(40,40,50,200);
        draw_list->AddText(ImVec2(vp_pos.x + 8, vp_pos.y + 8), info_col, info);
    } else {
        ImU32 hint_col = dark ? IM_COL32(100,100,120,200) : IM_COL32(120,120,140,200);
        draw_list->AddText(ImVec2(cx - 80, cy), hint_col, "Enter z = f(x,y) and press Plot");
    }

    ImGui::EndChild(); // 3DViewport
    ImGui::EndChild(); // 3DArea
}
