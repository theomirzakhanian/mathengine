#include "app.h"
#include "mathengine/parser.h"
#include <cmath>
#include <unordered_map>

// --- Variable collection (walk AST, find all Var nodes) ---

std::set<std::string> collect_variables(const mathengine::Expr::Ptr& expr) {
    std::set<std::string> vars;
    if (!expr) return vars;
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, mathengine::Expr::Var>) {
            vars.insert(node.name);
        } else if constexpr (std::is_same_v<T, mathengine::Expr::BinOp>) {
            auto l = collect_variables(node.lhs);
            auto r = collect_variables(node.rhs);
            vars.insert(l.begin(), l.end());
            vars.insert(r.begin(), r.end());
        } else if constexpr (std::is_same_v<T, mathengine::Expr::Unary>) {
            auto inner = collect_variables(node.operand);
            vars.insert(inner.begin(), inner.end());
        } else if constexpr (std::is_same_v<T, mathengine::Expr::Func>) {
            auto inner = collect_variables(node.arg);
            vars.insert(inner.begin(), inner.end());
        }
    }, expr->node);
    return vars;
}

// --- Parameter detection ---

void detect_parameters(PlotEntry& entry) {
    auto vars = collect_variables(entry.parsed_expr);
    if (entry.type == PlotEntry::Type::Parametric && entry.parsed_expr_y) {
        auto vars_y = collect_variables(entry.parsed_expr_y);
        vars.insert(vars_y.begin(), vars_y.end());
    }

    // Remove independent variable based on plot type
    if (entry.type == PlotEntry::Type::Polar) {
        vars.erase("theta");
    } else if (entry.type == PlotEntry::Type::Parametric) {
        vars.erase("t");
    } else {
        vars.erase("x");
    }

    // Preserve existing slider values where names match
    std::unordered_map<std::string, double> old_values;
    for (auto& p : entry.params) old_values[p.name] = p.value;

    entry.params.clear();
    for (const auto& v : vars) {
        ParamSlider s;
        s.name = v;
        auto it = old_values.find(v);
        if (it != old_values.end()) s.value = it->second;
        entry.params.push_back(s);
    }
}

// --- Parse a plot entry ---

void try_parse_plot(PlotEntry& entry) {
    try {
        entry.parsed_expr = mathengine::parse(entry.expression);
        entry.compiled = std::make_unique<mathengine::CompiledExpr>(entry.parsed_expr);
        entry.error_msg.clear();

        if (entry.type == PlotEntry::Type::Parametric && entry.expression_y[0] != '\0') {
            entry.parsed_expr_y = mathengine::parse(entry.expression_y);
            entry.compiled_y = std::make_unique<mathengine::CompiledExpr>(entry.parsed_expr_y);
        } else {
            entry.parsed_expr_y = nullptr;
            entry.compiled_y = nullptr;
        }

        detect_parameters(entry);
        entry.needs_resample = true;
    } catch (const std::exception& e) {
        entry.error_msg = e.what();
        entry.compiled = nullptr;
        entry.compiled_y = nullptr;
    }
}

// --- Resample a single plot ---

void resample_plot(PlotEntry& entry, const AppState& state, float graph_width) {
    if (!entry.compiled) return;

    // Build VarMap for parameters + animation time
    mathengine::VarMap vars;
    for (auto& p : entry.params) vars[p.name] = p.value;
    vars["t"] = state.anim_time; // animation time variable
    bool has_params = !entry.params.empty() || state.anim_playing;

    if (entry.type == PlotEntry::Type::Polar) {
        int N = state.polar_samples;
        entry.plot_x.resize(N);
        entry.plot_y.resize(N);
        entry.discontinuity.resize(N, false);
        double dtheta = (state.theta_max - state.theta_min) / (N - 1);

        for (int i = 0; i < N; i++) {
            double theta = state.theta_min + i * dtheta;
            vars["theta"] = theta;
            double r = entry.compiled->eval(vars);
            entry.plot_x[i] = (float)(r * std::cos(theta));
            entry.plot_y[i] = (float)(r * std::sin(theta));
        }

        // Discontinuity: large spatial jumps or NaN
        double y_range = state.y_max - state.y_min;
        if (y_range < 1e-10) y_range = 1.0;
        for (int i = 0; i + 1 < N; i++) {
            float dx = entry.plot_x[i + 1] - entry.plot_x[i];
            float dy = entry.plot_y[i + 1] - entry.plot_y[i];
            float dist = std::sqrt(dx * dx + dy * dy);
            bool bad = std::isnan(entry.plot_x[i]) || std::isnan(entry.plot_y[i]) ||
                       std::isinf(entry.plot_x[i]) || std::isinf(entry.plot_y[i]);
            entry.discontinuity[i] = bad || (dist > y_range * 0.5);
        }
    } else if (entry.type == PlotEntry::Type::Parametric) {
        if (!entry.compiled_y) return;
        int N = state.parametric_samples;
        entry.plot_x.resize(N);
        entry.plot_y.resize(N);
        entry.discontinuity.resize(N, false);
        double dt = (state.t_max - state.t_min) / (N - 1);

        for (int i = 0; i < N; i++) {
            double t = state.t_min + i * dt;
            vars["t"] = t;
            entry.plot_x[i] = (float)entry.compiled->eval(vars);
            entry.plot_y[i] = (float)entry.compiled_y->eval(vars);
        }

        double y_range = state.y_max - state.y_min;
        if (y_range < 1e-10) y_range = 1.0;
        for (int i = 0; i + 1 < N; i++) {
            float dx = entry.plot_x[i + 1] - entry.plot_x[i];
            float dy = entry.plot_y[i + 1] - entry.plot_y[i];
            float dist = std::sqrt(dx * dx + dy * dy);
            bool bad = std::isnan(entry.plot_x[i]) || std::isnan(entry.plot_y[i]) ||
                       std::isinf(entry.plot_x[i]) || std::isinf(entry.plot_y[i]);
            entry.discontinuity[i] = bad || (dist > y_range * 0.5);
        }
    } else {
        // Cartesian
        int num_samples = std::max(100, (int)(graph_width * 2));
        entry.plot_x.resize(num_samples);
        entry.plot_y.resize(num_samples);
        entry.discontinuity.resize(num_samples, false);
        double dx = (state.x_max - state.x_min) / (num_samples - 1);

        for (int i = 0; i < num_samples; i++) {
            double x = state.x_min + i * dx;
            entry.plot_x[i] = (float)x;
            if (has_params) {
                vars["x"] = x;
                entry.plot_y[i] = (float)entry.compiled->eval(vars);
            } else {
                entry.plot_y[i] = (float)entry.compiled->eval(x);
            }
        }

        double y_range = state.y_max - state.y_min;
        if (y_range < 1e-10) y_range = 1.0;
        for (int i = 0; i + 1 < num_samples; i++) {
            float dy = std::abs(entry.plot_y[i + 1] - entry.plot_y[i]);
            bool bad = std::isnan(entry.plot_y[i]) || std::isinf(entry.plot_y[i]) ||
                       std::isnan(entry.plot_y[i + 1]) || std::isinf(entry.plot_y[i + 1]);
            entry.discontinuity[i] = bad || (dy > y_range * 0.5);
        }
    }

    entry.needs_resample = false;
}

// --- Resample all plots ---

void resample_all(AppState& state, float graph_width) {
    for (auto& plot : state.plots) {
        if (plot.visible && plot.compiled && (plot.needs_resample || state.needs_resample)) {
            resample_plot(plot, state, graph_width);
        }
    }
    state.needs_resample = false;
}

// --- Legacy functions for non-graph modes ---

void try_parse(AppState& state) {
    try {
        state.parsed_expr = mathengine::parse(state.expr_input);
        state.compiled_expr = std::make_unique<mathengine::CompiledExpr>(state.parsed_expr);
        state.error_msg.clear();
        state.needs_resample = true;
    } catch (const std::exception& e) {
        state.error_msg = e.what();
        state.parsed_expr = nullptr;
        state.compiled_expr = nullptr;
    }
}

void resample(AppState& state, float graph_width) {
    if (!state.compiled_expr) return;

    int num_samples = std::max(100, (int)(graph_width * 2));
    state.plot_x.resize(num_samples);
    state.plot_y.resize(num_samples);
    state.discontinuity.resize(num_samples, false);

    double dx = (state.x_max - state.x_min) / (num_samples - 1);
    for (int i = 0; i < num_samples; i++) {
        double x = state.x_min + i * dx;
        state.plot_x[i] = (float)x;
        state.plot_y[i] = (float)state.compiled_expr->eval(x);
    }

    double y_range = state.y_max - state.y_min;
    if (y_range < 1e-10) y_range = 1.0;
    for (int i = 0; i + 1 < num_samples; i++) {
        float dy = std::abs(state.plot_y[i + 1] - state.plot_y[i]);
        bool bad = std::isnan(state.plot_y[i]) || std::isinf(state.plot_y[i]) ||
                   std::isnan(state.plot_y[i + 1]) || std::isinf(state.plot_y[i + 1]);
        state.discontinuity[i] = bad || (dy > y_range * 0.5);
    }

    state.needs_resample = false;
}
