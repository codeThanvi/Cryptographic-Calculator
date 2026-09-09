// Experiment (per lab spec): compare naive modular exponentiation against
// square-and-multiply, timing both across increasingly large exponents.
#include "bignum.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

int main() {
    BigNum base = BigNum::from_decimal("123456789");
    BigNum mod  = BigNum::from_decimal("1000000000000000000000000000000000000000000000000000000000063");

    std::vector<int> exponents = {10, 100, 1000, 5000, 10000, 20000};

    std::cout << std::left << std::setw(10) << "exponent"
              << std::setw(18) << "naive (ms)"
              << std::setw(18) << "fast (ms)"
              << "results match?\n";
    std::cout << std::string(56, '-') << "\n";

    for (int e : exponents) {
        BigNum exp = BigNum::from_decimal(std::to_string(e));

        auto t0 = std::chrono::high_resolution_clock::now();
        BigNum r_naive = BigNum::mod_pow_naive(base, exp, mod);
        auto t1 = std::chrono::high_resolution_clock::now();
        BigNum r_fast = BigNum::mod_pow_fast(base, exp, mod);
        auto t2 = std::chrono::high_resolution_clock::now();

        double naive_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double fast_ms  = std::chrono::duration<double, std::milli>(t2 - t1).count();

        std::cout << std::left << std::setw(10) << e
                   << std::setw(18) << naive_ms
                   << std::setw(18) << fast_ms
                   << (r_naive == r_fast ? "yes" : "MISMATCH") << "\n";
    }
    return 0;
}
