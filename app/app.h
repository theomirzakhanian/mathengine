#pragma once

#include "mathengine/expr.h"
#include "mathengine/eval.h"
#include <string>
#include <vector>
#include <set>
#include <memory>

// 8 distinct plot colors
static const unsigned int kPlotColors[] = {
    0xFFFFc850, 0xFF6464FF, 0xFF64FF64, 0xFF3CC8FF,
    0xFFFF64C8, 0xFF3296FF, 0xFFDCFF64, 0xFFC864FF,
};
static const int kNumPlotColors = 8;

struct ParamSlider {
    std::string name;
    double value = 1.0;
    double min_val = -10.0;
    double max_val = 10.0;
};

struct PlotEntry {
    int id = 0;
    char expression[512] = "";
    bool visible = true;
    unsigned int color = 0;

    enum class Type { Cartesian, Polar, Parametric };
    Type type = Type::Cartesian;

    char expression_y[512] = "";

    mathengine::Expr::Ptr parsed_expr;
    std::unique_ptr<mathengine::CompiledExpr> compiled;
    mathengine::Expr::Ptr parsed_expr_y;
    std::unique_ptr<mathengine::CompiledExpr> compiled_y;

    std::vector<float> plot_x, plot_y;
    std::vector<bool> discontinuity;

    std::vector<ParamSlider> params;

    std::string error_msg;
    bool needs_resample = true;
};

struct IntegralShading {
    bool enabled = false;
    int plot_index = 0;
    double x_lo = -1.0;
    double x_hi = 1.0;
    double computed_area = 0.0;
};

struct AppState {
    // --- Top-level tab ---
    enum class Tab { Graph, Calculator, ThreeD, Matrix, Console, Design, Tools, Data, Implicit };
    Tab active_tab = Tab::Graph;

    // --- Multi-plot (Graph tab) ---
    std::vector<PlotEntry> plots;
    int next_plot_id = 1;

    // Graph view
    double x_min = -10.0, x_max = 10.0;
    double y_min = -5.0, y_max = 5.0;

    IntegralShading integral;
    bool crosshair_enabled = true;
    double crosshair_x = 0.0, crosshair_y = 0.0;
    bool crosshair_visible = false;

    double t_min = 0.0, t_max = 6.283185307;
    double theta_min = 0.0, theta_max = 6.283185307;
    int polar_samples = 1000;
    int parametric_samples = 1000;

    bool needs_resample = true;

    // --- Animation (M11) ---
    bool anim_playing = false;
    double anim_time = 0.0;
    double anim_speed = 1.0;
    double anim_t_min = 0.0, anim_t_max = 10.0;

    // --- Data Lab (M12) ---
    char csv_input[4096] = "x, y\n1, 2.1\n2, 3.9\n3, 6.2\n4, 7.8\n5, 10.1";
    std::vector<double> data_x, data_y;
    bool data_loaded = false;
    int data_fit_degree = 1;
    std::string data_fit_result;
    std::string data_fit_desmos;
    std::vector<double> data_fit_coeffs;

    // --- Implicit Curves (M13) ---
    char implicit_expr[512] = "x^2 + y^2 - 9";
    bool implicit_dirty = true;
    bool implicit_shade_ineq = false; // shade y < f region
    std::vector<unsigned char> implicit_pixels; // pixel grid for rendering
    int implicit_res = 0;

    // --- Project (M14) ---
    char project_filename[256] = "workspace.mathproj";
    std::string project_status;

    // --- Command Palette (M15) ---
    bool cmd_palette_open = false;
    char cmd_palette_query[256] = "";

    // --- Theme ---
    enum class Theme { Dark, Light };
    Theme theme = Theme::Dark;
    bool export_requested = false;

    // --- CAS shared state ---
    char expr_input[512] = "";
    std::string error_msg;
    mathengine::Expr::Ptr parsed_expr;
    std::unique_ptr<mathengine::CompiledExpr> compiled_expr;
    std::string result_text;
    std::vector<std::string> solve_results;

    // Solver (numeric)
    char solver_a[64] = "-10";
    char solver_b[64] = "10";

    // Taylor
    char taylor_center[64] = "0";
    char taylor_order[64] = "5";

    // Limit
    char limit_point[64] = "0";
    int limit_direction = 0;

    // Systems
    char system_input[2048] = "2*x + 3*y = 5; x - y = 1";

    // Legacy single-plot data (unused, kept for compat)
    std::vector<float> plot_x, plot_y;
    std::vector<bool> discontinuity;

    // --- 3D Surface (M3) ---
    char surface_expr[512] = "sin(x)*cos(y)";
    mathengine::Expr::Ptr surface_parsed;
    std::unique_ptr<mathengine::CompiledExpr> surface_compiled;
    std::string surface_error;
    bool surface_dirty = true;

    double surf_range = 5.0, surf_range_y = 5.0;
    int surf_resolution = 40;
    bool surf_wireframe = true;
    bool surf_filled = true;
    bool surf_contours = false;

    double cam_yaw = 0.6, cam_pitch = -0.5, cam_dist = 8.0;

    std::vector<double> surf_z;
    double surf_z_min = 0, surf_z_max = 0;

    // --- Step-by-step (M4) ---
    bool show_steps = false;
    std::vector<std::string> step_log;

    // --- Graph Designer ---
    struct DrawPoint { float x, y; };

    // Multi-stroke: each drag creates a separate stroke
    struct Stroke {
        std::vector<DrawPoint> points;
        unsigned int color = 0xFFFFc850;
        // Parametric fit: x(t) and y(t)
        std::string desmos;   // Desmos parametric string for this stroke
        std::string label;    // display label
        bool fitted = false;
    };
    std::vector<Stroke> strokes;
    std::vector<DrawPoint> current_stroke; // in-progress stroke (while mouse is down)
    bool drawing_active = false;

    int design_tool = 0;       // 0=draw, 1=erase
    int design_fit_order = 15; // polynomial degree for parametric fit
    std::string design_desmos_all; // all strokes combined for clipboard

    // Background image for tracing
    char bg_image_path[512] = "";
    unsigned int bg_image_texture = 0; // OpenGL texture ID
    int bg_image_width = 0, bg_image_height = 0;
    float bg_image_opacity = 0.5f;
    bool bg_image_load_requested = false;
    bool bg_image_pick_requested = false;
    std::string bg_image_status;
    float bg_image_offset_x = 0.0f;  // canvas pixels
    float bg_image_offset_y = 0.0f;
    float bg_image_scale = 1.0f;     // multiplier
    bool bg_image_lock = false;      // lock to prevent accidental moves

    // --- Matrix tab ---
    int mat_rows = 3, mat_cols = 3;
    double mat_data[8][8] = {{1,2,3,0,0,0,0,0},{0,1,4,0,0,0,0,0},{5,6,0,0,0,0,0,0}};
    // Second matrix for multiplication
    int mat_b_rows = 3, mat_b_cols = 1;
    double mat_b_data[8][8] = {{1,0,0,0,0,0,0,0},{2,0,0,0,0,0,0,0},{3,0,0,0,0,0,0,0}};
    std::string mat_result_text;
};

// Multi-plot functions
void try_parse_plot(PlotEntry& entry);
void resample_plot(PlotEntry& entry, const AppState& state, float graph_width);
void resample_all(AppState& state, float graph_width);
void detect_parameters(PlotEntry& entry);
std::set<std::string> collect_variables(const mathengine::Expr::Ptr& expr);

// Legacy
void try_parse(AppState& state);
void resample(AppState& state, float graph_width);
