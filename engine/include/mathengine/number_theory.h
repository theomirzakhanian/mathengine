#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace mathengine {

bool is_prime(int64_t n);
std::vector<std::pair<int64_t, int>> prime_factorize(int64_t n);
int64_t gcd(int64_t a, int64_t b);
int64_t lcm(int64_t a, int64_t b);
int64_t mod_pow(int64_t base, int64_t exp, int64_t mod);
std::vector<int64_t> primes_up_to(int64_t n);
int64_t fibonacci(int n);
int64_t factorial(int n);
int64_t binomial(int n, int k);

std::string factorization_string(int64_t n);

} // namespace mathengine
