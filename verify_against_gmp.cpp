#include "level1.hpp"
#include <gmpxx.h>
#include <iostream>
#include <random>
#include <string>
#include <chrono>

static std::mt19937_64 rng(0xC0FFEE);
static std::string random_decimal(int bits) {
    mpz_class v = 0;
    int words = (bits + 63) / 64;
    for (int i = 0; i < words; i++) {
        v <<= 64;
        v += (uint64_t)rng();
    }
    mpz_class mask = (mpz_class(1) << bits) - 1;
    v &= mask;
    if (v == 0) v = 1;
    return v.get_str(10);
}

static int failures = 0;
static int total = 0;

// Cumulative time spent inside BigNum vs GMP calls, across the whole run.
static double bignum_total_us = 0.0;
static double gmp_total_us = 0.0;

using Clock = std::chrono::high_resolution_clock;
static double elapsed_us(Clock::time_point t0, Clock::time_point t1) {
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

static void check(const std::string& label, const std::string& got, const std::string& want,
                   double bignum_us, double gmp_us) {
    total++;
    bool ok = (got == want);
    if (!ok) failures++;
    bignum_total_us += bignum_us;
    gmp_total_us += gmp_us;
    std::cout << "  " << label << ": " << (ok ? "PASS" : "FAIL")
               << "   [BigNum: " << bignum_us << " us | GMP: " << gmp_us << " us]\n";
    std::cout << "    got:  " << got << "\n";
    std::cout << "    want: " << want << "\n";
}

int main() {
    std::cout << "Running BigNum vs GMP cross-verification (with timing)...\n";
    std::cout << "========================================\n\n";

    // ---- add / sub / mul / div / mod ---------------------------------
    std::cout << "-- add / sub / mul / div / mod --\n\n";
    for (int trial = 0; trial < 3; trial++) {
        std::string a_str = random_decimal(512);
        std::string b_str = random_decimal(256);

        std::cout << "Test " << (trial + 1) << ":\n";
        std::cout << "  a = " << a_str << "\n";
        std::cout << "  b = " << b_str << "\n\n";

        BigNum a = BigNum::from_decimal(a_str);
        BigNum b = BigNum::from_decimal(b_str);
        mpz_class ga(a_str), gb(b_str);

        {
            auto t0 = Clock::now(); BigNum r = a + b; auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gr = ga + gb; auto g1 = Clock::now();
            check("add", r.to_decimal(), gr.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        }
        {
            auto t0 = Clock::now(); BigNum r = a - b; auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gr = ga - gb; auto g1 = Clock::now();
            check("sub (a-b)", r.to_decimal(), gr.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        }
        {
            auto t0 = Clock::now(); BigNum r = a * b; auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gr = ga * gb; auto g1 = Clock::now();
            check("mul", r.to_decimal(), gr.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        }
        {
            BigNum rem;
            auto t0 = Clock::now(); BigNum q = BigNum::divmod_mag(a, b, rem); auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gq = ga / gb; auto g1 = Clock::now();
            check("div", q.to_decimal(), gq.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));

            auto g2 = Clock::now(); mpz_class gr = ga % gb; auto g3 = Clock::now();
            check("mod", rem.to_decimal(), gr.get_str(10), 0.0, elapsed_us(g2,g3));
        }
        std::cout << "\n";
    }

    // ---- gcd / extended gcd -------------------------------------------
    std::cout << "-- gcd / extended_gcd --\n\n";
    for (int trial = 0; trial < 3; trial++) {
        std::string a_str = random_decimal(200);
        std::string b_str = random_decimal(150);

        std::cout << "Test " << (trial + 1) << ":\n";
        std::cout << "  a = " << a_str << "\n";
        std::cout << "  b = " << b_str << "\n\n";

        BigNum a = BigNum::from_decimal(a_str);
        BigNum b = BigNum::from_decimal(b_str);
        mpz_class ga(a_str), gb(b_str);

        {
            auto t0 = Clock::now(); BigNum g = BigNum::gcd(a, b); auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gg; mpz_gcd(gg.get_mpz_t(), ga.get_mpz_t(), gb.get_mpz_t()); auto g1 = Clock::now();
            check("gcd", g.to_decimal(), gg.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        }

        BigNum x, y;
        auto t0 = Clock::now(); BigNum d = BigNum::Extended_gcd(a, b, x, y); auto t1 = Clock::now();
        mpz_class gd, gx, gy;
        auto g0 = Clock::now(); mpz_gcdext(gd.get_mpz_t(), gx.get_mpz_t(), gy.get_mpz_t(), ga.get_mpz_t(), gb.get_mpz_t()); auto g1 = Clock::now();
        check("extended_gcd (d)", d.to_decimal(), gd.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));

        std::cout << "  x = " << x.to_decimal() << "\n";
        std::cout << "  y = " << y.to_decimal() << "\n";
        mpz_class identity = ga * mpz_class(x.to_decimal()) + gb * mpz_class(y.to_decimal());
        check("extended_gcd identity (a*x + b*y = gcd)", identity.get_str(10), gd.get_str(10), 0.0, 0.0);
        std::cout << "\n";
    }

    // ---- modular inverse ------------------------------------------------
    std::cout << "-- modular_inverse --\n\n";
    int inv_tested = 0;
    for (int trial = 0; trial < 100 && inv_tested < 3; trial++) {
        std::string n_str = random_decimal(64);
        mpz_class gn(n_str);
        if (gn <= 1) continue;
        std::string a_str = random_decimal(64);
        mpz_class ga(a_str);
        ga = ((ga % gn) + gn) % gn;

        mpz_class g;
        mpz_gcd(g.get_mpz_t(), ga.get_mpz_t(), gn.get_mpz_t());
        if (g != 1) continue;

        std::cout << "Test " << (inv_tested + 1) << ":\n";
        std::cout << "  a = " << ga.get_str(10) << "\n";
        std::cout << "  n = " << gn.get_str(10) << "\n\n";

        BigNum bn_a = BigNum::from_decimal(ga.get_str(10));
        BigNum bn_n = BigNum::from_decimal(gn.get_str(10));

        auto t0 = Clock::now(); BigNum inv = BigNum::modular_inverse(bn_a, bn_n); auto t1 = Clock::now();
        mpz_class ginv;
        auto g0 = Clock::now(); mpz_invert(ginv.get_mpz_t(), ga.get_mpz_t(), gn.get_mpz_t()); auto g1 = Clock::now();
        check("modular_inverse", inv.to_decimal(), ginv.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        std::cout << "\n";
        inv_tested++;
    }

    // ---- modular exponentiation, RSA-scale -----------------------------
    std::cout << "-- mod_pow_naive / mod_pow_fast (512-bit) --\n\n";
    {
        std::string base_str = random_decimal(512);
        std::string exp_str  = "65537"; // standard RSA public exponent
        std::string mod_str  = random_decimal(512);

        std::cout << "Test (large exponent, e=65537):\n";
        std::cout << "  base = " << base_str << "\n";
        std::cout << "  exp  = " << exp_str << "\n";
        std::cout << "  mod  = " << mod_str << "\n\n";

        BigNum base = BigNum::from_decimal(base_str);
        BigNum exp  = BigNum::from_decimal(exp_str);
        BigNum mod  = BigNum::from_decimal(mod_str);

        auto t0 = Clock::now(); BigNum result_fast = BigNum::mod_pow_fast(base, exp, mod); auto t1 = Clock::now();
        mpz_class gbase(base_str), gexp(exp_str), gmod(mod_str), gresult;
        auto g0 = Clock::now(); mpz_powm(gresult.get_mpz_t(), gbase.get_mpz_t(), gexp.get_mpz_t(), gmod.get_mpz_t()); auto g1 = Clock::now();
        check("mod_pow_fast", result_fast.to_decimal(), gresult.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        std::cout << "\n";
    }
    {
        std::string base_str = random_decimal(512);
        std::string exp_str  = "97";
        std::string mod_str  = random_decimal(512);

        std::cout << "Test (small exponent, so naive is quick too):\n";
        std::cout << "  base = " << base_str << "\n";
        std::cout << "  exp  = " << exp_str << "\n";
        std::cout << "  mod  = " << mod_str << "\n\n";

        BigNum base = BigNum::from_decimal(base_str);
        BigNum exp  = BigNum::from_decimal(exp_str);
        BigNum mod  = BigNum::from_decimal(mod_str);

        auto t0 = Clock::now(); BigNum result_naive = BigNum::mod_pow_naive(base, exp, mod); auto t1 = Clock::now();
        auto t2 = Clock::now(); BigNum result_fast  = BigNum::mod_pow_fast(base, exp, mod);  auto t3 = Clock::now();

        mpz_class gbase(base_str), gexp(exp_str), gmod(mod_str), gresult;
        auto g0 = Clock::now(); mpz_powm(gresult.get_mpz_t(), gbase.get_mpz_t(), gexp.get_mpz_t(), gmod.get_mpz_t()); auto g1 = Clock::now();

        check("mod_pow_naive", result_naive.to_decimal(), gresult.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        check("mod_pow_fast", result_fast.to_decimal(), gresult.get_str(10), elapsed_us(t2,t3), elapsed_us(g0,g1));
        std::cout << "\n";
    }

    // ---- modular add / mul, including a negative-operand case -----------
    std::cout << "-- modular_add / modular_mul --\n\n";
    for (int trial = 0; trial < 2; trial++) {
        std::string a_str = (trial == 1 ? "-" : "") + random_decimal(100);
        std::string b_str = random_decimal(100);
        std::string m_str = random_decimal(80);

        std::cout << "Test " << (trial + 1) << (trial == 1 ? " (negative operand)" : "") << ":\n";
        std::cout << "  a = " << a_str << "\n";
        std::cout << "  b = " << b_str << "\n";
        std::cout << "  m = " << m_str << "\n\n";

        BigNum a = BigNum::from_decimal(a_str);
        BigNum b = BigNum::from_decimal(b_str);
        BigNum m = BigNum::from_decimal(m_str);
        mpz_class ga(a_str), gb(b_str), gm(m_str);

        {
            auto t0 = Clock::now(); BigNum radd = BigNum::modular_add(a, b, m); auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gadd = ((ga + gb) % gm + gm) % gm; auto g1 = Clock::now();
            check("modular_add", radd.to_decimal(), gadd.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        }
        {
            auto t0 = Clock::now(); BigNum rmul = BigNum::modular_mul(a, b, m); auto t1 = Clock::now();
            auto g0 = Clock::now(); mpz_class gmul = ((ga * gb) % gm + gm) % gm; auto g1 = Clock::now();
            check("modular_mul", rmul.to_decimal(), gmul.get_str(10), elapsed_us(t0,t1), elapsed_us(g0,g1));
        }
        std::cout << "\n";
    }

    std::cout << "========================================\n";
    std::cout << (total - failures) << " / " << total << " checks passed against GMP.\n";
    std::cout << "Total time -- BigNum: " << bignum_total_us << " us   GMP: " << gmp_total_us << " us"
               << "   (BigNum is " << (bignum_total_us / (gmp_total_us > 0 ? gmp_total_us : 1)) << "x GMP's time)\n";
    if (failures == 0) {
        std::cout << "ALL CHECKS PASSED.\n";
    } else {
        std::cout << failures << " CHECKS FAILED.\n";
    }
    return failures == 0 ? 0 : 1;
}