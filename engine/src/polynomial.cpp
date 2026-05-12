#include "mathengine/polynomial.h"
#include "mathengine/simplify.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace mathengine {

static constexpr double EPS = 1e-10;

void Polynomial::normalize() {
    while (coeffs.size() > 1 && std::abs(coeffs.back()) < EPS)
        coeffs.pop_back();
    if (coeffs.empty()) coeffs.push_back(0.0);
}

int Polynomial::degree() const {
    for (int i = (int)coeffs.size() - 1; i >= 0; i--)
        if (std::abs(coeffs[i]) > EPS) return i;
    return 0;
}

double Polynomial::coeff(int deg) const {
    if (deg < 0 || deg >= (int)coeffs.size()) return 0.0;
    return coeffs[deg];
}

double Polynomial::eval(double x) const {
    // Horner's method
    double result = 0.0;
    for (int i = (int)coeffs.size() - 1; i >= 0; i--)
        result = result * x + coeffs[i];
    return result;
}

Polynomial Polynomial::operator+(const Polynomial& o) const {
    size_t n = std::max(coeffs.size(), o.coeffs.size());
    std::vector<double> r(n, 0.0);
    for (size_t i = 0; i < coeffs.size(); i++) r[i] += coeffs[i];
    for (size_t i = 0; i < o.coeffs.size(); i++) r[i] += o.coeffs[i];
    return Polynomial(std::move(r));
}

Polynomial Polynomial::operator-(const Polynomial& o) const {
    size_t n = std::max(coeffs.size(), o.coeffs.size());
    std::vector<double> r(n, 0.0);
    for (size_t i = 0; i < coeffs.size(); i++) r[i] += coeffs[i];
    for (size_t i = 0; i < o.coeffs.size(); i++) r[i] -= o.coeffs[i];
    return Polynomial(std::move(r));
}

Polynomial Polynomial::operator*(const Polynomial& o) const {
    if (coeffs.empty() || o.coeffs.empty()) return Polynomial({0.0});
    std::vector<double> r(coeffs.size() + o.coeffs.size() - 1, 0.0);
    for (size_t i = 0; i < coeffs.size(); i++)
        for (size_t j = 0; j < o.coeffs.size(); j++)
            r[i + j] += coeffs[i] * o.coeffs[j];
    return Polynomial(std::move(r));
}

Polynomial Polynomial::operator*(double scalar) const {
    std::vector<double> r = coeffs;
    for (auto& c : r) c *= scalar;
    return Polynomial(std::move(r));
}

Polynomial Polynomial::operator-() const {
    return (*this) * -1.0;
}

std::pair<Polynomial, Polynomial> Polynomial::divmod(const Polynomial& divisor) const {
    int dd = divisor.degree();
    if (dd == 0 && std::abs(divisor.coeff(0)) < EPS)
        throw std::runtime_error("Polynomial division by zero");

    if (degree() < dd)
        return {Polynomial({0.0}), *this};

    std::vector<double> rem = coeffs;
    std::vector<double> quot(degree() - dd + 1, 0.0);
    double lead = divisor.coeff(dd);

    for (int i = (int)rem.size() - 1; i >= dd; i--) {
        double c = rem[i] / lead;
        quot[i - dd] = c;
        for (int j = 0; j <= dd; j++)
            rem[i - dd + j] -= c * divisor.coeff(j);
    }

    return {Polynomial(std::move(quot)), Polynomial(std::move(rem))};
}

Polynomial Polynomial::monic() const {
    int d = degree();
    double lead = coeff(d);
    if (std::abs(lead) < EPS) return *this;
    return (*this) * (1.0 / lead);
}

Expr::Ptr Polynomial::to_expr(const std::string& var) const {
    Expr::Ptr result = nullptr;

    for (int i = degree(); i >= 0; i--) {
        double c = coeff(i);
        if (std::abs(c) < EPS) continue;

        Expr::Ptr term;
        if (i == 0) {
            term = Expr::num(c);
        } else if (i == 1) {
            if (std::abs(c - 1.0) < EPS)
                term = Expr::var(var);
            else if (std::abs(c + 1.0) < EPS)
                term = Expr::unary(Expr::var(var));
            else
                term = Expr::binop('*', Expr::num(c), Expr::var(var));
        } else {
            auto xn = Expr::binop('^', Expr::var(var), Expr::num(i));
            if (std::abs(c - 1.0) < EPS)
                term = xn;
            else if (std::abs(c + 1.0) < EPS)
                term = Expr::unary(xn);
            else
                term = Expr::binop('*', Expr::num(c), xn);
        }

        if (!result)
            result = term;
        else
            result = Expr::binop('+', result, term);
    }

    return result ? simplify(result) : Expr::num(0);
}

// --- expr_to_poly: convert AST to polynomial ---

std::optional<Polynomial> expr_to_poly(const Expr::Ptr& expr, const std::string& var) {
    if (!expr) return std::nullopt;

    return std::visit([&](const auto& node) -> std::optional<Polynomial> {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            return Polynomial({node.value});
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            if (node.name == var)
                return Polynomial({0.0, 1.0}); // x
            else
                return std::nullopt; // unknown symbolic variable
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            auto inner = expr_to_poly(node.operand, var);
            if (!inner) return std::nullopt;
            return -(*inner);
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            auto lp = expr_to_poly(node.lhs, var);
            auto rp = expr_to_poly(node.rhs, var);

            switch (node.op) {
                case '+':
                    if (lp && rp) return *lp + *rp;
                    return std::nullopt;
                case '-':
                    if (lp && rp) return *lp - *rp;
                    return std::nullopt;
                case '*':
                    if (lp && rp) return *lp * *rp;
                    return std::nullopt;
                case '/': {
                    if (!lp || !rp) return std::nullopt;
                    // Only handle division by constant
                    if (rp->degree() == 0) {
                        double d = rp->coeff(0);
                        if (std::abs(d) < EPS) return std::nullopt;
                        return *lp * (1.0 / d);
                    }
                    return std::nullopt;
                }
                case '^': {
                    if (!lp) return std::nullopt;
                    // Exponent must be a non-negative integer constant
                    if (auto* n = std::get_if<Expr::Num>(&node.rhs->node)) {
                        int exp = (int)std::round(n->value);
                        if (exp < 0 || std::abs(n->value - exp) > EPS) return std::nullopt;
                        if (exp == 0) return Polynomial({1.0});
                        Polynomial result = *lp;
                        for (int i = 1; i < exp; i++)
                            result = result * (*lp);
                        return result;
                    }
                    return std::nullopt;
                }
                default:
                    return std::nullopt;
            }
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            return std::nullopt; // functions are not polynomial
        }
        else {
            return std::nullopt;
        }
    }, expr->node);
}

Polynomial poly_gcd(const Polynomial& a, const Polynomial& b) {
    if (b.degree() == 0 && std::abs(b.coeff(0)) < EPS)
        return a;
    auto [q, r] = a.divmod(b);
    return poly_gcd(b, r);
}

} // namespace mathengine
