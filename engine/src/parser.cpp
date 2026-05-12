#include "mathengine/parser.h"
#include <cctype>
#include <cmath>
#include <algorithm>

namespace mathengine {

namespace {

enum class TokenKind {
    Number, Ident,
    Plus, Minus, Star, Slash, Caret,
    LParen, RParen, Comma,
    End
};

struct Token {
    TokenKind kind;
    double num_val = 0.0;
    std::string str_val;
};

class Tokenizer {
public:
    explicit Tokenizer(const std::string& input) : src_(input), pos_(0) {}

    Token next() {
        skip_whitespace();
        if (pos_ >= src_.size()) return {TokenKind::End};

        char c = src_[pos_];

        if (std::isdigit(c) || c == '.') return read_number();
        if (std::isalpha(c) || c == '_') return read_ident();

        pos_++;
        switch (c) {
            case '+': return {TokenKind::Plus};
            case '-': return {TokenKind::Minus};
            case '*': return {TokenKind::Star};
            case '/': return {TokenKind::Slash};
            case '^': return {TokenKind::Caret};
            case '(': return {TokenKind::LParen};
            case ')': return {TokenKind::RParen};
            case ',': return {TokenKind::Comma};
            default:
                throw ParseError("Unexpected character: '" + std::string(1, c) + "'");
        }
    }

    Token peek() {
        size_t saved = pos_;
        Token t = next();
        pos_ = saved;
        return t;
    }

private:
    void skip_whitespace() {
        while (pos_ < src_.size() && std::isspace(src_[pos_])) pos_++;
    }

    Token read_number() {
        size_t start = pos_;
        while (pos_ < src_.size() && (std::isdigit(src_[pos_]) || src_[pos_] == '.'))
            pos_++;
        // Handle scientific notation: 1e5, 2.3e-4
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            pos_++;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-'))
                pos_++;
            while (pos_ < src_.size() && std::isdigit(src_[pos_]))
                pos_++;
        }
        Token t;
        t.kind = TokenKind::Number;
        t.num_val = std::stod(src_.substr(start, pos_ - start));
        return t;
    }

    Token read_ident() {
        size_t start = pos_;
        while (pos_ < src_.size() && (std::isalnum(src_[pos_]) || src_[pos_] == '_'))
            pos_++;
        Token t;
        t.kind = TokenKind::Ident;
        t.str_val = src_.substr(start, pos_ - start);
        return t;
    }

    std::string src_;
    size_t pos_;
};

// Known function names
bool is_function(const std::string& name) {
    static const char* funcs[] = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "exp", "ln", "log", "sqrt", "abs",
        "sinh", "cosh", "tanh"
    };
    for (auto f : funcs)
        if (name == f) return true;
    return false;
}

class Parser {
public:
    explicit Parser(const std::string& input) : tok_(input) {
        advance();
    }

    Expr::Ptr parse_expr() {
        auto result = parse_additive();
        if (cur_.kind != TokenKind::End)
            throw ParseError("Unexpected token after expression");
        return result;
    }

private:
    void advance() { cur_ = tok_.next(); }

    // expr := term (('+' | '-') term)*
    Expr::Ptr parse_additive() {
        auto left = parse_multiplicative();
        while (cur_.kind == TokenKind::Plus || cur_.kind == TokenKind::Minus) {
            char op = (cur_.kind == TokenKind::Plus) ? '+' : '-';
            advance();
            auto right = parse_multiplicative();
            left = Expr::binop(op, std::move(left), std::move(right));
        }
        return left;
    }

    // term := exponent (('*' | '/') exponent)*
    Expr::Ptr parse_multiplicative() {
        auto left = parse_exponent();
        while (cur_.kind == TokenKind::Star || cur_.kind == TokenKind::Slash) {
            char op = (cur_.kind == TokenKind::Star) ? '*' : '/';
            advance();
            auto right = parse_exponent();
            left = Expr::binop(op, std::move(left), std::move(right));
        }
        return left;
    }

    // exponent := unary ('^' exponent)?   (right-associative)
    Expr::Ptr parse_exponent() {
        auto left = parse_implicit_mul();
        if (cur_.kind == TokenKind::Caret) {
            advance();
            auto right = parse_exponent(); // right-recursive for right-assoc
            left = Expr::binop('^', std::move(left), std::move(right));
        }
        return left;
    }

    // Implicit multiplication: 2x, 2(x+1), x(x+1), (x)(x+1)
    Expr::Ptr parse_implicit_mul() {
        auto left = parse_unary();
        while (cur_.kind == TokenKind::LParen ||
               cur_.kind == TokenKind::Ident ||
               (cur_.kind == TokenKind::Number && false)) {
            // 2x or 2sin(x) — but only if previous was not an operator
            // Check if this looks like implicit multiplication
            if (cur_.kind == TokenKind::Ident && is_function(cur_.str_val)) {
                auto right = parse_unary();
                left = Expr::binop('*', std::move(left), std::move(right));
            } else if (cur_.kind == TokenKind::Ident) {
                auto right = parse_unary();
                left = Expr::binop('*', std::move(left), std::move(right));
            } else if (cur_.kind == TokenKind::LParen) {
                auto right = parse_unary();
                left = Expr::binop('*', std::move(left), std::move(right));
            } else {
                break;
            }
        }
        return left;
    }

    // unary := '-' unary | call
    Expr::Ptr parse_unary() {
        if (cur_.kind == TokenKind::Minus) {
            advance();
            auto operand = parse_unary();
            return Expr::unary(std::move(operand));
        }
        if (cur_.kind == TokenKind::Plus) {
            advance();
            return parse_unary();
        }
        return parse_call();
    }

    // call := IDENT '(' expr ')' | atom
    Expr::Ptr parse_call() {
        if (cur_.kind == TokenKind::Ident && is_function(cur_.str_val)) {
            std::string name = cur_.str_val;
            advance();
            if (cur_.kind != TokenKind::LParen)
                throw ParseError("Expected '(' after function '" + name + "'");
            advance();
            auto arg = parse_additive();
            if (cur_.kind != TokenKind::RParen)
                throw ParseError("Expected ')' after function argument");
            advance();
            return Expr::func(name, std::move(arg));
        }
        return parse_atom();
    }

    // atom := NUMBER | IDENT | '(' expr ')'
    Expr::Ptr parse_atom() {
        if (cur_.kind == TokenKind::Number) {
            double val = cur_.num_val;
            advance();
            return Expr::num(val);
        }

        if (cur_.kind == TokenKind::Ident) {
            std::string name = cur_.str_val;
            advance();
            // Built-in constants
            if (name == "pi") return Expr::num(M_PI);
            if (name == "e") return Expr::num(M_E);
            return Expr::var(name);
        }

        if (cur_.kind == TokenKind::LParen) {
            advance();
            auto inner = parse_additive();
            if (cur_.kind != TokenKind::RParen)
                throw ParseError("Expected ')'");
            advance();
            return inner;
        }

        throw ParseError("Unexpected token");
    }

    Tokenizer tok_;
    Token cur_;
};

} // anonymous namespace

Expr::Ptr parse(const std::string& input) {
    if (input.empty())
        throw ParseError("Empty expression");
    Parser p(input);
    return p.parse_expr();
}

// Pretty-printer
std::string to_string(const Expr::Ptr& expr) {
    if (!expr) return "null";

    return std::visit([](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            // Clean up trailing zeros
            std::string s = std::to_string(node.value);
            size_t dot = s.find('.');
            if (dot != std::string::npos) {
                size_t last_nonzero = s.find_last_not_of('0');
                if (last_nonzero == dot)
                    s = s.substr(0, dot); // integer
                else
                    s = s.substr(0, last_nonzero + 1);
            }
            return s;
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            return node.name;
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            std::string l = to_string(node.lhs);
            std::string r = to_string(node.rhs);
            if (node.op == '^')
                return "(" + l + "^" + r + ")";
            return "(" + l + " " + std::string(1, node.op) + " " + r + ")";
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            return "(-" + to_string(node.operand) + ")";
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            return node.name + "(" + to_string(node.arg) + ")";
        }
        else {
            return "?";
        }
    }, expr->node);
}

} // namespace mathengine
