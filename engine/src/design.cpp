#include "mathengine/design.h"
#include "mathengine/simplify.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace mathengine {

// ============================================================
// Number formatting for Desmos
// Rule: NEVER emit scientific notation. Desmos reads 'e' as Euler's number.
// ============================================================

static std::string fmt_num(double v) {
    // Integer check
    if (v == (long long)v && std::abs(v) < 1e12) {
        return std::to_string((long long)v);
    }
    // Fixed notation, strip trailing zeros
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(12) << v;
    std::string s = ss.str();
    size_t dot = s.find('.');
    if (dot != std::string::npos) {
        size_t last = s.find_last_not_of('0');
        if (last == dot) s = s.substr(0, dot);
        else s = s.substr(0, last + 1);
    }
    return s;
}

// ============================================================
// Polynomial fitting (least squares, direct coordinates)
// ============================================================

Polynomial poly_fit(const std::vector<Point2D>& points, int degree) {
    int n = (int)points.size();
    int m = degree + 1;
    if (n < m) m = n;

    std::vector<double> ata(m * m, 0), aty(m, 0);
    for (int i = 0; i < n; i++) {
        std::vector<double> row(m);
        row[0] = 1;
        for (int j = 1; j < m; j++) row[j] = row[j-1] * points[i].x;
        for (int j = 0; j < m; j++) {
            aty[j] += row[j] * points[i].y;
            for (int k = 0; k < m; k++)
                ata[j * m + k] += row[j] * row[k];
        }
    }

    std::vector<double> aug(m * (m + 1));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) aug[i*(m+1)+j] = ata[i*m+j];
        aug[i*(m+1)+m] = aty[i];
    }
    for (int col = 0; col < m; col++) {
        int pivot = col;
        for (int row = col+1; row < m; row++)
            if (std::abs(aug[row*(m+1)+col]) > std::abs(aug[pivot*(m+1)+col]))
                pivot = row;
        if (pivot != col)
            for (int j = 0; j <= m; j++) std::swap(aug[col*(m+1)+j], aug[pivot*(m+1)+j]);
        double d = aug[col*(m+1)+col];
        if (std::abs(d) < 1e-30) continue;
        for (int j = 0; j <= m; j++) aug[col*(m+1)+j] /= d;
        for (int row = 0; row < m; row++) {
            if (row == col) continue;
            double f = aug[row*(m+1)+col];
            for (int j = 0; j <= m; j++) aug[row*(m+1)+j] -= f * aug[col*(m+1)+j];
        }
    }

    std::vector<double> coeffs(m);
    for (int i = 0; i < m; i++) coeffs[i] = aug[i*(m+1)+m];
    return Polynomial(coeffs);
}

// ============================================================
// Fourier fitting (proper least squares)
// ============================================================

double FourierFit::eval(double x) const {
    double result = a0;
    for (int k = 0; k < order; k++) {
        result += a_cos[k] * std::cos((k+1) * freq * x);
        result += b_sin[k] * std::sin((k+1) * freq * x);
    }
    return result;
}

Expr::Ptr FourierFit::to_expr() const {
    Expr::Ptr result = Expr::num(a0);
    for (int k = 0; k < order; k++) {
        Expr::Ptr kfx;
        double kf = (k+1) * freq;
        if (std::abs(kf - 1.0) < 1e-10)
            kfx = Expr::var("x");
        else
            kfx = Expr::binop('*', Expr::num(kf), Expr::var("x"));

        if (std::abs(a_cos[k]) > 1e-10) {
            auto t = Expr::binop('*', Expr::num(a_cos[k]), Expr::func("cos", kfx));
            result = Expr::binop('+', result, t);
        }
        if (std::abs(b_sin[k]) > 1e-10) {
            auto t = Expr::binop('*', Expr::num(b_sin[k]), Expr::func("sin", kfx));
            result = Expr::binop('+', result, t);
        }
    }
    return simplify(result);
}

// ============================================================
// Fourier -> Desmos
//
// Desmos format: a0 + a1*cos(f*x) + b1*sin(f*x) + ...
// Use \cos\left(arg\right), \sin\left(arg\right)
// Coefficient * trig: use juxtaposition for positive, explicit - for negative
// ============================================================

std::string FourierFit::to_desmos() const {
    std::ostringstream ss;
    ss << fmt_num(a0);
    for (int k = 0; k < order; k++) {
        double kf = (k+1) * freq;

        // Build the argument string: "kf*x" or just "x" if kf==1
        std::string arg_str;
        if (std::abs(kf - 1.0) < 1e-10)
            arg_str = "x";
        else
            arg_str = fmt_num(kf) + "x";

        if (std::abs(a_cos[k]) > 1e-10) {
            if (a_cos[k] > 0) {
                ss << "+" << fmt_num(a_cos[k]);
            } else {
                ss << "-" << fmt_num(std::abs(a_cos[k]));
            }
            ss << "\\cos\\left(" << arg_str << "\\right)";
        }
        if (std::abs(b_sin[k]) > 1e-10) {
            if (b_sin[k] > 0) {
                ss << "+" << fmt_num(b_sin[k]);
            } else {
                ss << "-" << fmt_num(std::abs(b_sin[k]));
            }
            ss << "\\sin\\left(" << arg_str << "\\right)";
        }
    }
    return ss.str();
}

FourierFit fourier_fit(const std::vector<Point2D>& points, int order) {
    FourierFit fit;
    fit.order = order;
    fit.a_cos.resize(order, 0);
    fit.b_sin.resize(order, 0);

    int n = (int)points.size();
    if (n == 0) { fit.a0 = 0; fit.freq = 1.0; return fit; }

    double x_min = points[0].x, x_max = points[0].x;
    for (auto& p : points) { x_min = std::min(x_min, p.x); x_max = std::max(x_max, p.x); }
    double range = x_max - x_min;
    if (range < 1e-10) range = 1.0;
    fit.freq = 2.0 * M_PI / range;

    int m = 1 + 2 * order;
    std::vector<double> ata(m * m, 0), aty(m, 0);
    for (int i = 0; i < n; i++) {
        std::vector<double> row(m);
        row[0] = 1.0;
        for (int k = 0; k < order; k++) {
            row[1 + 2*k]     = std::cos((k+1) * fit.freq * points[i].x);
            row[1 + 2*k + 1] = std::sin((k+1) * fit.freq * points[i].x);
        }
        for (int j = 0; j < m; j++) {
            aty[j] += row[j] * points[i].y;
            for (int k2 = 0; k2 < m; k2++)
                ata[j * m + k2] += row[j] * row[k2];
        }
    }

    std::vector<double> aug(m * (m + 1));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) aug[i*(m+1)+j] = ata[i*m+j];
        aug[i*(m+1)+m] = aty[i];
    }
    for (int col = 0; col < m; col++) {
        int pivot = col;
        for (int row = col+1; row < m; row++)
            if (std::abs(aug[row*(m+1)+col]) > std::abs(aug[pivot*(m+1)+col]))
                pivot = row;
        if (pivot != col)
            for (int j = 0; j <= m; j++) std::swap(aug[col*(m+1)+j], aug[pivot*(m+1)+j]);
        double d = aug[col*(m+1)+col];
        if (std::abs(d) < 1e-30) continue;
        for (int j = 0; j <= m; j++) aug[col*(m+1)+j] /= d;
        for (int row = 0; row < m; row++) {
            if (row == col) continue;
            double f = aug[row*(m+1)+col];
            for (int j = 0; j <= m; j++) aug[row*(m+1)+j] -= f * aug[col*(m+1)+j];
        }
    }

    fit.a0 = aug[0*(m+1)+m];
    for (int k = 0; k < order; k++) {
        fit.a_cos[k] = aug[(1+2*k)*(m+1)+m];
        fit.b_sin[k] = aug[(1+2*k+1)*(m+1)+m];
    }

    return fit;
}

// ============================================================
// Expr -> Desmos LaTeX
//
// Desmos syntax rules:
//   2x        → 2 times x (juxtaposition, number before var)
//   2x^{3}    → 2 times x^3 (Desmos binds juxtaposition before ^)
//              WAIT — Desmos actually parses 2x^{3} as 2*(x^3), NOT (2x)^3
//              This is verified behavior.
//   -x^{2}    → -(x^2) (unary minus binds loosely)
//   \frac{a}{b} → a/b
//   \sin\left(x\right) → sin(x)
//   x^{n}     → x to the n
//   \left(...\right) → grouping
// ============================================================

std::string to_desmos(const Expr::Ptr& expr) {
    if (!expr) return "";

    return std::visit([](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            double v = node.value;
            if (v < 0) {
                // Wrap negative numbers to avoid ambiguity: output as (-3) not -3
                // unless it will be the only thing (caller handles context)
                return fmt_num(v);
            }
            return fmt_num(v);
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            return node.name;
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            std::string l = to_desmos(node.lhs);
            std::string r = to_desmos(node.rhs);
            switch (node.op) {
                case '+': return l + "+" + r;
                case '-': return l + "-\\left(" + r + "\\right)";
                case '*': {
                    // Safe multiplication: always use \cdot
                    // This avoids ALL ambiguity with juxtaposition
                    return l + "\\cdot " + r;
                }
                case '/': return "\\frac{" + l + "}{" + r + "}";
                case '^': {
                    // Base needs parens if it's compound
                    bool base_simple = (std::holds_alternative<Expr::Num>(node.lhs->node) ||
                                       std::holds_alternative<Expr::Var>(node.lhs->node));
                    std::string base = base_simple ? l : "\\left(" + l + "\\right)";
                    return base + "^{" + r + "}";
                }
                default: return l + "?" + r;
            }
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            std::string inner = to_desmos(node.operand);
            return "-\\left(" + inner + "\\right)";
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            std::string arg = to_desmos(node.arg);
            if (node.name == "ln") return "\\ln\\left(" + arg + "\\right)";
            if (node.name == "log") return "\\log\\left(" + arg + "\\right)";
            if (node.name == "sqrt") return "\\sqrt{" + arg + "}";
            if (node.name == "abs") return "\\left|" + arg + "\\right|";
            if (node.name == "asin") return "\\arcsin\\left(" + arg + "\\right)";
            if (node.name == "acos") return "\\arccos\\left(" + arg + "\\right)";
            if (node.name == "atan") return "\\arctan\\left(" + arg + "\\right)";
            return "\\" + node.name + "\\left(" + arg + "\\right)";
        }
        else {
            return "?";
        }
    }, expr->node);
}

// ============================================================
// Polynomial -> Desmos LaTeX (direct from coefficients)
//
// This is the PRIMARY export path for the design tool.
// Format: each term is [sign][abscoeff]x^{n}
// Desmos parses 2.5x^{3} as 2.5 * x^3 (verified).
//
// Rules:
//   - First term: no leading + sign
//   - Subsequent positive terms: + prefix
//   - Negative terms: - prefix, use abs(coeff)
//   - Coeff of 1: omit (just x^{n})
//   - Coeff of -1: just -x^{n}
//   - Degree 0: just the number
//   - Degree 1: x not x^{1}
// ============================================================

std::string poly_to_desmos(const Polynomial& p) {
    std::ostringstream ss;
    bool first = true;

    for (int i = p.degree(); i >= 0; i--) {
        double c = p.coeff(i);
        if (std::abs(c) < 1e-12) continue;

        double ac = std::abs(c);
        bool neg = (c < 0);

        if (i == 0) {
            // Constant term
            if (!first && !neg) ss << "+";
            if (neg) ss << "-";
            ss << fmt_num(ac);
        } else {
            // Build x part
            std::string xp = (i == 1) ? "x" : ("x^{" + std::to_string(i) + "}");

            if (!first) ss << (neg ? "-" : "+");
            else if (neg) ss << "-";

            if (std::abs(ac - 1.0) < 1e-12) {
                ss << xp;
            } else {
                ss << fmt_num(ac) << xp;
            }
        }
        first = false;
    }

    if (first) ss << "0";
    return ss.str();
}

// ============================================================
// Parametric stroke fitting
//
// Given a sequence of (x,y) points from a drawn stroke:
// 1. Compute cumulative arc-length parameter t in [0,1]
// 2. Fit x(t) as polynomial of given degree
// 3. Fit y(t) as polynomial of given degree
// 4. Export as Desmos parametric: (px(t), py(t))
// ============================================================

Point2D ParametricFit::eval(double t) const {
    return {px.eval(t), py.eval(t)};
}

// Convert polynomial in variable "t" to Desmos string
static std::string poly_to_desmos_var(const Polynomial& p, const char* var) {
    std::ostringstream ss;
    bool first = true;
    for (int i = p.degree(); i >= 0; i--) {
        double c = p.coeff(i);
        if (std::abs(c) < 1e-12) continue;
        double ac = std::abs(c);
        bool neg = (c < 0);

        if (i == 0) {
            if (!first && !neg) ss << "+";
            if (neg) ss << "-";
            ss << fmt_num(ac);
        } else {
            std::string vp = (i == 1) ? std::string(var) :
                             (std::string(var) + "^{" + std::to_string(i) + "}");
            if (!first) ss << (neg ? "-" : "+");
            else if (neg) ss << "-";
            if (std::abs(ac - 1.0) < 1e-12) ss << vp;
            else ss << fmt_num(ac) << vp;
        }
        first = false;
    }
    if (first) ss << "0";
    return ss.str();
}

std::string ParametricFit::to_desmos() const {
    std::string sx = poly_to_desmos_var(px, "t");
    std::string sy = poly_to_desmos_var(py, "t");
    return "\\left(" + sx + ",\\ " + sy + "\\right)";
}

ParametricFit parametric_fit(const std::vector<Point2D>& points, int degree) {
    int n = (int)points.size();
    if (n < 2) {
        ParametricFit fit;
        fit.px = Polynomial({0});
        fit.py = Polynomial({0});
        return fit;
    }

    // Compute cumulative arc-length parameter
    std::vector<double> t_vals(n);
    t_vals[0] = 0;
    for (int i = 1; i < n; i++) {
        double dx = points[i].x - points[i-1].x;
        double dy = points[i].y - points[i-1].y;
        t_vals[i] = t_vals[i-1] + std::sqrt(dx*dx + dy*dy);
    }
    // Normalize to [0, 1]
    double total = t_vals[n-1];
    if (total < 1e-15) total = 1.0;
    for (int i = 0; i < n; i++) t_vals[i] /= total;

    // Build two sets of Point2D: (t, x) and (t, y)
    std::vector<Point2D> tx_pts(n), ty_pts(n);
    for (int i = 0; i < n; i++) {
        tx_pts[i] = {t_vals[i], points[i].x};
        ty_pts[i] = {t_vals[i], points[i].y};
    }

    ParametricFit fit;
    fit.px = poly_fit(tx_pts, degree);
    fit.py = poly_fit(ty_pts, degree);
    return fit;
}

} // namespace mathengine
