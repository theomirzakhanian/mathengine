#include "mathengine/number_theory.h"
#include <cmath>
#include <sstream>

namespace mathengine {

bool is_prime(int64_t n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int64_t i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

std::vector<std::pair<int64_t, int>> prime_factorize(int64_t n) {
    std::vector<std::pair<int64_t, int>> factors;
    if (n < 0) n = -n;
    for (int64_t d = 2; d * d <= n; d++) {
        int count = 0;
        while (n % d == 0) { n /= d; count++; }
        if (count > 0) factors.push_back({d, count});
    }
    if (n > 1) factors.push_back({n, 1});
    return factors;
}

int64_t gcd(int64_t a, int64_t b) {
    a = std::abs(a); b = std::abs(b);
    while (b) { a %= b; std::swap(a, b); }
    return a;
}

int64_t lcm(int64_t a, int64_t b) {
    return (a / gcd(a, b)) * b;
}

int64_t mod_pow(int64_t base, int64_t exp, int64_t mod) {
    int64_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) result = result * base % mod;
        exp >>= 1;
        base = base * base % mod;
    }
    return result;
}

std::vector<int64_t> primes_up_to(int64_t n) {
    std::vector<bool> sieve(n + 1, true);
    sieve[0] = sieve[1] = false;
    for (int64_t i = 2; i * i <= n; i++)
        if (sieve[i])
            for (int64_t j = i * i; j <= n; j += i) sieve[j] = false;
    std::vector<int64_t> result;
    for (int64_t i = 2; i <= n; i++) if (sieve[i]) result.push_back(i);
    return result;
}

int64_t fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    int64_t a = 0, b = 1;
    for (int i = 2; i <= n; i++) { int64_t t = a + b; a = b; b = t; }
    return b;
}

int64_t factorial(int n) {
    if (n < 0) return 0;
    int64_t r = 1;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}

int64_t binomial(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k > n - k) k = n - k;
    int64_t r = 1;
    for (int i = 0; i < k; i++) r = r * (n - i) / (i + 1);
    return r;
}

std::string factorization_string(int64_t n) {
    auto factors = prime_factorize(n);
    std::ostringstream ss;
    ss << n << " = ";
    for (size_t i = 0; i < factors.size(); i++) {
        if (i > 0) ss << " * ";
        ss << factors[i].first;
        if (factors[i].second > 1) ss << "^" << factors[i].second;
    }
    return ss.str();
}

} // namespace mathengine
