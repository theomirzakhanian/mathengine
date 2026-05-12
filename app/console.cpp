#include "console.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"
#include "mathengine/simplify.h"
#include "mathengine/calculus.h"
#include "mathengine/algebra.h"
#include "mathengine/symbolic_solve.h"
#include "mathengine/taylor.h"
#include "mathengine/limits.h"
#include "mathengine/pretty.h"
#include "mathengine/polynomial.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <cmath>
#include <map>

namespace {

struct ConsoleState {
    std::vector<std::string> history;       // command history
    std::vector<std::string> output;        // output lines
    std::vector<bool> output_is_error;      // per-line error flag
    mathengine::VarMap variables;           // user-defined variables
    std::map<std::string, std::string> functions; // name -> expression body
    int history_pos = -1;

    void add_output(const std::string& s, bool is_err = false) {
        output.push_back(s);
        output_is_error.push_back(is_err);
    }

    void clear() {
        output.clear();
        output_is_error.clear();
    }
};

static ConsoleState console;
static bool initialized = false;

void init_console() {
    if (initialized) return;
    initialized = true;
    console.variables["pi"] = M_PI;
    console.variables["e"] = M_E;
    console.add_output("MathEngine Console - Type 'help' for commands");
    console.add_output("---");
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

void process_command(const std::string& input, AppState& state) {
    auto cmd = trim(input);
    if (cmd.empty()) return;

    console.history.push_back(cmd);
    console.add_output("> " + cmd);

    // --- Built-in commands ---
    if (cmd == "help") {
        console.add_output("Commands:");
        console.add_output("  <expr>          Evaluate expression");
        console.add_output("  x = <expr>      Assign variable");
        console.add_output("  f(x) = <expr>   Define function");
        console.add_output("  diff <expr>     Differentiate w.r.t. x");
        console.add_output("  integrate <expr> Integrate w.r.t. x");
        console.add_output("  expand <expr>   Expand expression");
        console.add_output("  factor <expr>   Factor expression");
        console.add_output("  solve <expr>    Solve expr = 0");
        console.add_output("  taylor <expr> <order>  Taylor series at 0");
        console.add_output("  table <expr> <start> <end> <step>");
        console.add_output("  fit <degree> <x1,y1> <x2,y2> ...");
        console.add_output("  vars            List variables");
        console.add_output("  funcs           List functions");
        console.add_output("  clear           Clear console");
        return;
    }

    if (cmd == "clear") {
        console.clear();
        return;
    }

    if (cmd == "vars") {
        for (auto& [name, val] : console.variables)
            console.add_output("  " + name + " = " + std::to_string(val));
        return;
    }

    if (cmd == "funcs") {
        for (auto& [name, body] : console.functions)
            console.add_output("  " + name + "(x) = " + body);
        return;
    }

    // --- diff <expr> ---
    if (cmd.substr(0, 5) == "diff ") {
        try {
            auto expr = mathengine::parse(cmd.substr(5));
            auto deriv = mathengine::simplify(mathengine::differentiate(expr, "x"));
            console.add_output("= " + mathengine::pretty_string(deriv));
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- integrate <expr> ---
    if (cmd.substr(0, 10) == "integrate ") {
        try {
            auto expr = mathengine::parse(cmd.substr(10));
            auto result = mathengine::integrate(expr, "x");
            if (result)
                console.add_output("= " + mathengine::pretty_string(result) + " + C");
            else
                console.add_output("Cannot integrate symbolically", true);
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- expand <expr> ---
    if (cmd.substr(0, 7) == "expand ") {
        try {
            auto expr = mathengine::parse(cmd.substr(7));
            auto result = mathengine::expand(expr);
            console.add_output("= " + mathengine::pretty_string(result));
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- factor <expr> ---
    if (cmd.substr(0, 7) == "factor ") {
        try {
            auto expr = mathengine::parse(cmd.substr(7));
            auto result = mathengine::factor(expr, "x");
            console.add_output("= " + mathengine::pretty_string(result));
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- solve <expr> ---
    if (cmd.substr(0, 6) == "solve ") {
        try {
            auto expr = mathengine::parse(cmd.substr(6));
            auto roots = mathengine::symbolic_solve(expr, "x");
            if (roots.empty()) {
                console.add_output("No solutions found", true);
            } else {
                for (auto& r : roots)
                    console.add_output("x = " + mathengine::pretty_string(r));
            }
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- taylor <expr> <order> ---
    if (cmd.substr(0, 7) == "taylor ") {
        try {
            auto rest = cmd.substr(7);
            // Find last space-separated number as order
            size_t last_space = rest.rfind(' ');
            int order = 5;
            std::string expr_str = rest;
            if (last_space != std::string::npos) {
                std::string maybe_num = rest.substr(last_space + 1);
                char* end;
                long val = strtol(maybe_num.c_str(), &end, 10);
                if (*end == '\0') {
                    order = (int)val;
                    expr_str = rest.substr(0, last_space);
                }
            }
            auto expr = mathengine::parse(expr_str);
            auto series = mathengine::taylor(expr, "x", 0.0, order);
            console.add_output("= " + mathengine::pretty_string(series));
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- table <expr> <start> <end> <step> ---
    if (cmd.substr(0, 6) == "table ") {
        try {
            std::istringstream ss(cmd.substr(6));
            std::string expr_str;
            double start = -5, end = 5, step = 1;
            // Parse: everything up to the last 3 numbers
            std::vector<std::string> tokens;
            std::string tok;
            while (ss >> tok) tokens.push_back(tok);

            if (tokens.size() >= 4) {
                step = std::atof(tokens.back().c_str()); tokens.pop_back();
                end = std::atof(tokens.back().c_str()); tokens.pop_back();
                start = std::atof(tokens.back().c_str()); tokens.pop_back();
                expr_str = "";
                for (auto& t : tokens) expr_str += t + " ";
            } else {
                expr_str = cmd.substr(6);
            }

            auto expr = mathengine::parse(trim(expr_str));
            mathengine::CompiledExpr compiled(expr);

            console.add_output("  x          | f(x)");
            console.add_output("  -----------+-----------");
            if (step <= 0) step = 1;
            for (double x = start; x <= end + step * 0.01; x += step) {
                double y = compiled.eval(x);
                char buf[80];
                snprintf(buf, sizeof(buf), "  %-10.4g | %.6g", x, y);
                console.add_output(buf);
            }
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- fit <degree> <x1,y1> <x2,y2> ... ---
    if (cmd.substr(0, 4) == "fit ") {
        try {
            std::istringstream ss(cmd.substr(4));
            int degree;
            ss >> degree;
            std::vector<double> xs, ys;
            std::string pair;
            while (ss >> pair) {
                size_t comma = pair.find(',');
                if (comma == std::string::npos) continue;
                xs.push_back(std::atof(pair.substr(0, comma).c_str()));
                ys.push_back(std::atof(pair.substr(comma + 1).c_str()));
            }
            if (xs.size() < (size_t)(degree + 1)) {
                console.add_output("Need at least " + std::to_string(degree + 1) + " data points", true);
                return;
            }

            // Least squares polynomial fit: solve (A^T A) c = A^T y
            int n = (int)xs.size(), m = degree + 1;
            // Build A^T A and A^T y
            std::vector<double> ata(m * m, 0), aty(m, 0);
            for (int i = 0; i < n; i++) {
                std::vector<double> row(m);
                row[0] = 1;
                for (int j = 1; j < m; j++) row[j] = row[j-1] * xs[i];
                for (int j = 0; j < m; j++) {
                    aty[j] += row[j] * ys[i];
                    for (int k = 0; k < m; k++)
                        ata[j * m + k] += row[j] * row[k];
                }
            }

            // Solve via Gaussian elimination
            std::vector<double> aug(m * (m + 1));
            for (int i = 0; i < m; i++) {
                for (int j = 0; j < m; j++) aug[i * (m+1) + j] = ata[i * m + j];
                aug[i * (m+1) + m] = aty[i];
            }
            for (int col = 0; col < m; col++) {
                int pivot = col;
                for (int row = col + 1; row < m; row++)
                    if (std::abs(aug[row*(m+1)+col]) > std::abs(aug[pivot*(m+1)+col]))
                        pivot = row;
                if (pivot != col)
                    for (int j = 0; j <= m; j++) std::swap(aug[col*(m+1)+j], aug[pivot*(m+1)+j]);
                double d = aug[col*(m+1)+col];
                if (std::abs(d) < 1e-15) { console.add_output("Singular matrix in fit", true); return; }
                for (int j = 0; j <= m; j++) aug[col*(m+1)+j] /= d;
                for (int row = 0; row < m; row++) {
                    if (row == col) continue;
                    double f = aug[row*(m+1)+col];
                    for (int j = 0; j <= m; j++) aug[row*(m+1)+j] -= f * aug[col*(m+1)+j];
                }
            }

            std::vector<double> coeffs(m);
            for (int i = 0; i < m; i++) coeffs[i] = aug[i*(m+1)+m];

            mathengine::Polynomial poly(coeffs);
            console.add_output("Fit: " + mathengine::pretty_string(poly.to_expr("x")));

            // R-squared
            double y_mean = 0;
            for (double y : ys) y_mean += y;
            y_mean /= n;
            double ss_tot = 0, ss_res = 0;
            for (int i = 0; i < n; i++) {
                double pred = poly.eval(xs[i]);
                ss_res += (ys[i] - pred) * (ys[i] - pred);
                ss_tot += (ys[i] - y_mean) * (ys[i] - y_mean);
            }
            double r2 = (ss_tot > 1e-15) ? 1.0 - ss_res / ss_tot : 1.0;
            char buf[64];
            snprintf(buf, sizeof(buf), "R^2 = %.6f", r2);
            console.add_output(buf);
        } catch (const std::exception& e) {
            console.add_output(std::string("Error: ") + e.what(), true);
        }
        return;
    }

    // --- Function definition: f(x) = <expr> ---
    {
        size_t paren = cmd.find('(');
        size_t eq = cmd.find('=');
        if (paren != std::string::npos && eq != std::string::npos && paren < eq && paren > 0) {
            size_t rparen = cmd.find(')');
            if (rparen != std::string::npos && rparen < eq) {
                std::string fname = trim(cmd.substr(0, paren));
                std::string body = trim(cmd.substr(eq + 1));
                if (!fname.empty() && !body.empty()) {
                    try {
                        mathengine::parse(body); // validate
                        console.functions[fname] = body;
                        console.add_output("Defined " + fname + "(x) = " + body);
                    } catch (const std::exception& e) {
                        console.add_output(std::string("Error in definition: ") + e.what(), true);
                    }
                    return;
                }
            }
        }
    }

    // --- Variable assignment: var = <expr> ---
    {
        size_t eq = cmd.find('=');
        if (eq != std::string::npos && eq > 0) {
            std::string varname = trim(cmd.substr(0, eq));
            std::string expr_str = trim(cmd.substr(eq + 1));
            // Check it's a valid identifier
            bool valid_name = true;
            for (char c : varname) {
                if (!std::isalnum(c) && c != '_') { valid_name = false; break; }
            }
            if (valid_name && !expr_str.empty()) {
                try {
                    auto expr = mathengine::parse(expr_str);
                    double val = mathengine::evaluate(expr, console.variables);
                    console.variables[varname] = val;
                    console.add_output(varname + " = " + std::to_string(val));
                } catch (const std::exception& e) {
                    console.add_output(std::string("Error: ") + e.what(), true);
                }
                return;
            }
        }
    }

    // --- Expression evaluation ---
    try {
        // Substitute user-defined functions
        std::string eval_str = cmd;
        for (auto& [fname, fbody] : console.functions) {
            // Simple substitution: f(arg) -> (body with x replaced)
            // This is a basic approach - just evaluate
            size_t pos = 0;
            while ((pos = eval_str.find(fname + "(", pos)) != std::string::npos) {
                size_t start = pos + fname.size() + 1;
                int depth = 1;
                size_t end = start;
                while (end < eval_str.size() && depth > 0) {
                    if (eval_str[end] == '(') depth++;
                    if (eval_str[end] == ')') depth--;
                    end++;
                }
                std::string arg = eval_str.substr(start, end - start - 1);
                // Replace f(arg) with (body) where x is replaced with (arg)
                std::string expanded = fbody;
                size_t xpos = 0;
                while ((xpos = expanded.find('x', xpos)) != std::string::npos) {
                    if ((xpos == 0 || !std::isalnum(expanded[xpos-1])) &&
                        (xpos + 1 >= expanded.size() || !std::isalnum(expanded[xpos+1]))) {
                        expanded.replace(xpos, 1, "(" + arg + ")");
                        xpos += arg.size() + 2;
                    } else {
                        xpos++;
                    }
                }
                eval_str.replace(pos, end - pos, "(" + expanded + ")");
                pos += expanded.size() + 2;
            }
        }

        auto expr = mathengine::parse(eval_str);
        double val = mathengine::evaluate(expr, console.variables);
        console.add_output("= " + std::to_string(val));
        console.variables["ans"] = val;
    } catch (const std::exception& e) {
        console.add_output(std::string("Error: ") + e.what(), true);
    }
}

} // anonymous namespace

static int input_callback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        if (console.history.empty()) return 0;
        if (data->EventKey == ImGuiKey_UpArrow) {
            if (console.history_pos < 0)
                console.history_pos = (int)console.history.size() - 1;
            else if (console.history_pos > 0)
                console.history_pos--;
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (console.history_pos >= 0)
                console.history_pos++;
            if (console.history_pos >= (int)console.history.size())
                console.history_pos = -1;
        }
        if (console.history_pos >= 0 && console.history_pos < (int)console.history.size()) {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, console.history[console.history_pos].c_str());
        } else {
            data->DeleteChars(0, data->BufTextLen);
        }
    }
    return 0;
}

void render_console_tab(AppState& state, ImVec2 content_size) {
    init_console();

    ImGui::BeginChild("ConsoleArea", content_size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    bool dark = (state.theme == AppState::Theme::Dark);

    // Output area
    float input_height = 30;
    ImVec2 output_size(0, ImGui::GetContentRegionAvail().y - input_height - 8);
    ImGui::BeginChild("ConsoleOutput", output_size, ImGuiChildFlags_Border);

    for (size_t i = 0; i < console.output.size(); i++) {
        if (console.output_is_error[i]) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("%s", console.output[i].c_str());
            ImGui::PopStyleColor();
        } else if (console.output[i].size() > 0 && console.output[i][0] == '>') {
            ImU32 col = dark ? IM_COL32(120, 200, 255, 255) : IM_COL32(0, 80, 180, 255);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col));
            ImGui::TextWrapped("%s", console.output[i].c_str());
            ImGui::PopStyleColor();
        } else if (console.output[i].size() > 1 && console.output[i][0] == '=') {
            ImU32 col = dark ? IM_COL32(100, 255, 140, 255) : IM_COL32(0, 140, 40, 255);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col));
            ImGui::TextWrapped("%s", console.output[i].c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextWrapped("%s", console.output[i].c_str());
        }
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    // Input line
    static char input_buf[512] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##console_input", input_buf, sizeof(input_buf),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
            input_callback)) {
        process_command(input_buf, state);
        input_buf[0] = '\0';
        console.history_pos = -1;
        ImGui::SetKeyboardFocusHere(-1);
    }

    // Auto-focus input
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere(-1);

    ImGui::PopStyleVar();
    ImGui::EndChild();
}
