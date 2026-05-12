#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/design.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"
#include <cmath>
#include <iostream>

using namespace mathengine;

// ============================================================
// Desmos LaTeX -> our parser syntax converter
//
// Converts Desmos-style LaTeX to something our parser can handle.
// This simulates what Desmos does internally.
// ============================================================

static std::string desmos_to_parseable(const std::string& input) {
    std::string s = input;

    // Strip y= prefix
    if (s.size() > 2 && s[0] == 'y' && s[1] == '=') s = s.substr(2);

    // \left( \right) -> ( )
    for (;;) { auto p = s.find("\\left("); if (p == std::string::npos) break; s.replace(p, 6, "("); }
    for (;;) { auto p = s.find("\\right)"); if (p == std::string::npos) break; s.replace(p, 7, ")"); }

    // \left| \right| -> abs(...)
    for (;;) {
        auto l = s.find("\\left|");
        if (l == std::string::npos) break;
        auto r = s.find("\\right|", l);
        if (r == std::string::npos) break;
        std::string inner = s.substr(l + 6, r - l - 6);
        s = s.substr(0, l) + "abs(" + inner + ")" + s.substr(r + 7);
    }

    // \frac{num}{den} -> ((num)/(den))
    for (;;) {
        auto p = s.find("\\frac{");
        if (p == std::string::npos) break;
        int d = 0; size_t ne = p + 6;
        for (size_t i = ne; i < s.size(); i++) {
            if (s[i] == '{') d++; if (s[i] == '}') { if (d == 0) { ne = i; break; } d--; }
        }
        size_t ds = ne + 2; d = 0; size_t de = ds;
        for (size_t i = ds; i < s.size(); i++) {
            if (s[i] == '{') d++; if (s[i] == '}') { if (d == 0) { de = i; break; } d--; }
        }
        std::string num = s.substr(p + 6, ne - p - 6);
        std::string den = s.substr(ds, de - ds);
        s = s.substr(0, p) + "((" + num + ")/(" + den + "))" + s.substr(de + 1);
    }

    // \sqrt{arg} -> sqrt(arg)
    for (;;) {
        auto p = s.find("\\sqrt{");
        if (p == std::string::npos) break;
        int d = 0; size_t e = p + 6;
        for (size_t i = e; i < s.size(); i++) {
            if (s[i] == '{') d++; if (s[i] == '}') { if (d == 0) { e = i; break; } d--; }
        }
        std::string inner = s.substr(p + 6, e - p - 6);
        s = s.substr(0, p) + "sqrt(" + inner + ")" + s.substr(e + 1);
    }

    // x^{exp} -> x^(exp)
    for (;;) {
        auto p = s.find("^{");
        if (p == std::string::npos) break;
        int d = 0; size_t e = p + 2;
        for (size_t i = e; i < s.size(); i++) {
            if (s[i] == '{') d++; if (s[i] == '}') { if (d == 0) { e = i; break; } d--; }
        }
        std::string exp_s = s.substr(p + 2, e - p - 2);
        s = s.substr(0, p) + "^(" + exp_s + ")" + s.substr(e + 1);
    }

    // LaTeX function names -> our names (longer patterns first)
    const char* fmap[][2] = {
        {"\\arcsin", "asin"}, {"\\arccos", "acos"}, {"\\arctan", "atan"},
        {"\\sinh", "sinh"}, {"\\cosh", "cosh"}, {"\\tanh", "tanh"},
        {"\\sin", "sin"}, {"\\cos", "cos"}, {"\\tan", "tan"},
        {"\\ln", "ln"}, {"\\log", "log"}, {"\\exp", "exp"},
    };
    for (auto& [from, to] : fmap) {
        for (;;) { auto p = s.find(from); if (p == std::string::npos) break; s.replace(p, strlen(from), to); }
    }

    // \cdot -> *
    for (;;) { auto p = s.find("\\cdot"); if (p == std::string::npos) break; s.replace(p, 5, "*"); }

    // Remove remaining backslashes
    std::string clean;
    for (char c : s) if (c != '\\') clean += c;
    s = clean;

    // Desmos precedence: -x^{2} means -(x^2), not (-x)^2
    // Convert: if we see -<var>^( or -<num><var>^(, wrap the power part
    // Simpler approach: insert explicit * for juxtaposition AND
    // handle leading - before x^n by wrapping: -x^(n) -> -(x^(n))

    // First: insert * between number and letter
    clean.clear();
    for (size_t i = 0; i < s.size(); i++) {
        clean += s[i];
        if (i + 1 < s.size()) {
            bool d = (std::isdigit(s[i]) || s[i] == '.');
            bool a = std::isalpha(s[i+1]);
            bool p = (s[i+1] == '(');
            if (d && (a || p)) clean += '*';
            if (s[i] == ')' && (std::isdigit(s[i+1]) || std::isalpha(s[i+1]) || s[i+1] == '('))
                clean += '*';
        }
    }
    s = clean;

    // Handle Desmos unary minus precedence: in Desmos, -x^2 = -(x^2)
    // But our parser reads -x^2 as (-x)^2
    // Fix: convert patterns like "-x^(" to "-(x^(" and add closing paren
    // More general: after a - that's at start or after +/-, if followed by
    // something with ^, wrap the entire power expression in parens
    //
    // Simple approach: convert "-VAR^(" to "-(VAR^(" and track parens
    // Also: "-NUM*VAR^(" to "-(NUM*VAR^("
    clean.clear();
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '-' && (i == 0 || s[i-1] == '+' || s[i-1] == '-' || s[i-1] == '(')) {
            // Look ahead: is this -EXPR where EXPR contains ^?
            // Find the extent of the next "term" (until + or - or end)
            size_t j = i + 1;
            int depth = 0;
            while (j < s.size()) {
                if (s[j] == '(') depth++;
                if (s[j] == ')') { if (depth == 0) break; depth--; }
                if (depth == 0 && (s[j] == '+' || (s[j] == '-' && j > i + 1))) break;
                j++;
            }
            std::string term = s.substr(i + 1, j - i - 1);
            if (term.find('^') != std::string::npos) {
                clean += "-(";
                clean += term;
                clean += ")";
                i = j - 1;
                continue;
            }
        }
        clean += s[i];
    }

    return clean;
}

// ============================================================
// Verification helpers
// ============================================================

// Verify Desmos string matches a Polynomial at N test points
static bool verify_poly_desmos(const Polynomial& poly, const std::string& desmos,
                               double xlo, double xhi, int npts, double tol) {
    std::string parseable = desmos_to_parseable(desmos);
    Expr::Ptr dexpr;
    try { dexpr = parse(parseable); }
    catch (const std::exception& e) {
        std::cerr << "PARSE FAIL: " << e.what() << "\n  Desmos:  " << desmos << "\n  Parsed:  " << parseable << std::endl;
        return false;
    }
    CompiledExpr dc(dexpr);
    for (int i = 0; i < npts; i++) {
        double x = xlo + (xhi - xlo) * i / (npts - 1);
        double yp = poly.eval(x);
        double yd = dc.eval(x);
        if (std::isnan(yp) || std::isinf(yp)) continue;
        if (std::isnan(yd) || std::isinf(yd)) {
            std::cerr << "NaN at x=" << x << " | parsed: " << parseable << std::endl;
            return false;
        }
        double err = std::abs(yp - yd);
        double scale = std::max(1.0, std::abs(yp));
        if (err / scale > tol) {
            std::cerr << "MISMATCH x=" << x << " poly=" << yp << " desmos=" << yd
                      << "\n  desmos: " << desmos << "\n  parsed: " << parseable << std::endl;
            return false;
        }
    }
    return true;
}

// Verify Desmos string matches an Expr at N test points
static bool verify_expr_desmos(const Expr::Ptr& expr, const std::string& desmos,
                               double xlo, double xhi, int npts, double tol) {
    std::string parseable = desmos_to_parseable(desmos);
    Expr::Ptr dexpr;
    try { dexpr = parse(parseable); }
    catch (const std::exception& e) {
        std::cerr << "PARSE FAIL: " << e.what() << "\n  Desmos: " << desmos << "\n  Parsed: " << parseable << std::endl;
        return false;
    }
    CompiledExpr oc(expr), dc(dexpr);
    for (int i = 0; i < npts; i++) {
        double x = xlo + (xhi - xlo) * i / (npts - 1);
        double yo = oc.eval(x), yd = dc.eval(x);
        if (std::isnan(yo) || std::isinf(yo)) continue;
        if (std::isnan(yd) || std::isinf(yd)) return false;
        double err = std::abs(yo - yd);
        double scale = std::max(1.0, std::abs(yo));
        if (err / scale > tol) {
            std::cerr << "MISMATCH x=" << x << " orig=" << yo << " desmos=" << yd
                      << "\n  desmos: " << desmos << "\n  parsed: " << parseable << std::endl;
            return false;
        }
    }
    return true;
}

// Assert no scientific notation in string
static void assert_no_sci(const std::string& s) {
    for (size_t i = 1; i < s.size(); i++) {
        if ((s[i] == 'e' || s[i] == 'E') && std::isdigit(s[i-1])) {
            if (i + 1 < s.size() && (s[i+1] == '+' || s[i+1] == '-' || std::isdigit(s[i+1]))) {
                FAIL("Scientific notation found: " << s);
            }
        }
    }
}

// ============================================================
// CONVERTER TESTS
// ============================================================

TEST_CASE("Converter: basics", "[desmos]") {
    CHECK(desmos_to_parseable("x") == "x");
    CHECK(desmos_to_parseable("x^{2}") == "x^(2)");
    CHECK(desmos_to_parseable("2x") == "2*x");
    CHECK(desmos_to_parseable("2x^{3}") == "2*x^(3)");
    CHECK(desmos_to_parseable("-3x^{2}") == "-(3*x^(2))");
    CHECK(desmos_to_parseable("\\sin\\left(x\\right)") == "sin(x)");
    CHECK(desmos_to_parseable("\\frac{1}{2}") == "((1)/(2))");
    CHECK(desmos_to_parseable("\\sqrt{x}") == "sqrt(x)");
    CHECK(desmos_to_parseable("\\left|x\\right|") == "abs(x)");
}

// ============================================================
// EXPR -> DESMOS TESTS
// ============================================================

TEST_CASE("Expr desmos: x^2", "[desmos]") {
    auto e = parse("x^2");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_expr_desmos(e, d, -5, 5, 30, 1e-6));
}

TEST_CASE("Expr desmos: x^3 - 2*x + 1", "[desmos]") {
    auto e = parse("x^3 - 2*x + 1");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_expr_desmos(e, d, -5, 5, 30, 1e-6));
}

TEST_CASE("Expr desmos: sin(x)", "[desmos]") {
    auto e = parse("sin(x)");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    REQUIRE(verify_expr_desmos(e, d, -6, 6, 30, 1e-6));
}

TEST_CASE("Expr desmos: exp(-x^2)", "[desmos]") {
    auto e = parse("exp(-x^2)");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    REQUIRE(verify_expr_desmos(e, d, -3, 3, 30, 1e-6));
}

TEST_CASE("Expr desmos: 1/(x^2+1)", "[desmos]") {
    auto e = parse("1/(x^2+1)");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    REQUIRE(verify_expr_desmos(e, d, -5, 5, 30, 1e-6));
}

TEST_CASE("Expr desmos: sqrt(abs(x))", "[desmos]") {
    auto e = parse("sqrt(abs(x))");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    REQUIRE(verify_expr_desmos(e, d, 0.1, 5, 20, 1e-6));
}

TEST_CASE("Expr desmos: sin(x)/x", "[desmos]") {
    auto e = parse("sin(x)/x");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    REQUIRE(verify_expr_desmos(e, d, 0.1, 6, 20, 1e-6));
}

TEST_CASE("Expr desmos: 3*x^2 + 2*x - 7", "[desmos]") {
    auto e = parse("3*x^2 + 2*x - 7");
    auto d = to_desmos(e);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_expr_desmos(e, d, -5, 5, 30, 1e-6));
}

// ============================================================
// POLY -> DESMOS TESTS
// ============================================================

TEST_CASE("Poly desmos: x^2 - 3x + 2", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 40; i++) {
        double x = -5.0 + 10.0 * i / 40;
        pts.push_back({x, x*x - 3*x + 2});
    }
    auto poly = poly_fit(pts, 2);
    auto d = poly_to_desmos(poly);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_poly_desmos(poly, d, -5, 5, 30, 1e-4));
}

TEST_CASE("Poly desmos: degree 5 fit of sin(x)*x", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 60; i++) {
        double x = -3.0 + 6.0 * i / 60;
        pts.push_back({x, std::sin(x) * x});
    }
    auto poly = poly_fit(pts, 5);
    auto d = poly_to_desmos(poly);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_poly_desmos(poly, d, -3, 3, 30, 1e-4));
}

TEST_CASE("Poly desmos: degree 10 fit", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 80; i++) {
        double x = -4.0 + 8.0 * i / 80;
        pts.push_back({x, std::sin(x * 1.5) * std::exp(-x*x/10) + 0.3*x});
    }
    auto poly = poly_fit(pts, 10);
    auto d = poly_to_desmos(poly);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_poly_desmos(poly, d, -4, 4, 30, 1e-2));
}

TEST_CASE("Poly desmos: tiny coefficients (no sci notation)", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 30; i++) {
        double x = -1.0 + 2.0 * i / 30;
        pts.push_back({x, 0.00001 * x * x * x + 0.001 * x});
    }
    auto poly = poly_fit(pts, 3);
    auto d = poly_to_desmos(poly);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_poly_desmos(poly, d, -1, 1, 20, 1e-4));
}

TEST_CASE("Poly desmos: all negative coefficients", "[desmos]") {
    Polynomial p({-5, -3, -1}); // -x^2 - 3x - 5
    auto d = poly_to_desmos(p);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_poly_desmos(p, d, -5, 5, 20, 1e-6));
}

TEST_CASE("Poly desmos: coefficient = 1 and -1", "[desmos]") {
    Polynomial p({0, -1, 1}); // x^2 - x
    auto d = poly_to_desmos(p);
    INFO("Desmos: " << d);
    assert_no_sci(d);
    REQUIRE(verify_poly_desmos(p, d, -5, 5, 20, 1e-6));
    // Should NOT contain "1x" — just "x"
    REQUIRE(d.find("1x") == std::string::npos);
}

TEST_CASE("Poly desmos: constant only", "[desmos]") {
    Polynomial p({42});
    auto d = poly_to_desmos(p);
    INFO("Desmos: " << d);
    REQUIRE(d == "42");
}

TEST_CASE("Poly desmos: single term x^5", "[desmos]") {
    Polynomial p({0, 0, 0, 0, 0, 1}); // x^5
    auto d = poly_to_desmos(p);
    INFO("Desmos: " << d);
    REQUIRE(verify_poly_desmos(p, d, -2, 2, 20, 1e-6));
}

// ============================================================
// FOURIER -> DESMOS TESTS
// ============================================================

TEST_CASE("Fourier desmos: sin+cos mix", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 50; i++) {
        double x = -5.0 + 10.0 * i / 50;
        pts.push_back({x, std::sin(x) + 0.5 * std::cos(2*x)});
    }
    auto fit = fourier_fit(pts, 4);
    auto d = fit.to_desmos();
    INFO("Desmos: " << d);
    assert_no_sci(d);

    // Verify against FourierFit::eval (ground truth)
    std::string parseable = desmos_to_parseable(d);
    auto dexpr = parse(parseable);
    CompiledExpr dc(dexpr);
    for (int i = 0; i < 20; i++) {
        double x = -5.0 + 10.0 * i / 19;
        double yf = fit.eval(x);
        double yd = dc.eval(x);
        REQUIRE_THAT(yd, Catch::Matchers::WithinAbs(yf, 1e-4));
    }
}

// ============================================================
// PARAMETRIC -> DESMOS TESTS
// ============================================================

TEST_CASE("Parametric fit: circle", "[desmos]") {
    // Generate points on a circle
    std::vector<Point2D> pts;
    for (int i = 0; i <= 100; i++) {
        double theta = 2.0 * M_PI * i / 100;
        pts.push_back({std::cos(theta), std::sin(theta)});
    }
    auto fit = parametric_fit(pts, 15);

    // Check that the fit traces the circle
    for (int i = 0; i < 20; i++) {
        double t = (double)i / 19;
        auto pt = fit.eval(t);
        double r = std::sqrt(pt.x * pt.x + pt.y * pt.y);
        REQUIRE_THAT(r, Catch::Matchers::WithinAbs(1.0, 0.1));
    }

    // Check desmos output has no sci notation
    auto d = fit.to_desmos();
    INFO("Parametric Desmos: " << d);
    assert_no_sci(d);
    // Should start with \left( and end with \right)
    REQUIRE(d.find("\\left(") == 0);
    REQUIRE(d.find("\\right)") != std::string::npos);
}

TEST_CASE("Parametric fit: line segment", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 20; i++) {
        double t = (double)i / 20;
        pts.push_back({t * 3 - 1, t * 2 + 1});
    }
    auto fit = parametric_fit(pts, 3);
    // Should closely trace from (-1,1) to (2,3)
    auto p0 = fit.eval(0);
    auto p1 = fit.eval(1);
    REQUIRE_THAT(p0.x, Catch::Matchers::WithinAbs(-1.0, 0.1));
    REQUIRE_THAT(p0.y, Catch::Matchers::WithinAbs(1.0, 0.1));
    REQUIRE_THAT(p1.x, Catch::Matchers::WithinAbs(2.0, 0.1));
    REQUIRE_THAT(p1.y, Catch::Matchers::WithinAbs(3.0, 0.1));
}

TEST_CASE("Parametric fit: smiley mouth (arc)", "[desmos]") {
    // Semi-circle arc (bottom half of a circle)
    std::vector<Point2D> pts;
    for (int i = 0; i <= 50; i++) {
        double theta = M_PI * i / 50; // 0 to pi
        pts.push_back({2.0 * std::cos(theta), -2.0 * std::sin(theta) - 1});
    }
    auto fit = parametric_fit(pts, 12);
    auto d = fit.to_desmos();
    INFO("Mouth Desmos: " << d);
    assert_no_sci(d);

    // Start and end should be at roughly (2,-1) and (-2,-1)
    auto p0 = fit.eval(0);
    auto p1 = fit.eval(1);
    REQUIRE_THAT(p0.x, Catch::Matchers::WithinAbs(2.0, 0.2));
    REQUIRE_THAT(p1.x, Catch::Matchers::WithinAbs(-2.0, 0.2));
}

TEST_CASE("Fourier desmos: no sci notation", "[desmos]") {
    std::vector<Point2D> pts;
    for (int i = 0; i <= 40; i++) {
        double x = -3.0 + 6.0 * i / 40;
        pts.push_back({x, 0.001 * std::sin(x * 3)});
    }
    auto fit = fourier_fit(pts, 3);
    auto d = fit.to_desmos();
    INFO("Desmos: " << d);
    assert_no_sci(d);
}
