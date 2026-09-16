
#include "level1.hpp"
#include <gmpxx.h>
#include <iostream>
#include <random>
#include <string>

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

static void expect_eq(const std::string& label, const std::string& got, const std::string& want) {
    total++;
    if (got != want) {
        failures++;
        std::cout << "  MISMATCH [" << label << "]\n    got:  " << got << "\n    want: " << want << "\n";
    }
}

int main() {
    std::cout << "Running BigNum vs GMP cross-verification...\n\n";

  
    std::cout << "add / sub / mul / div / mod (512+ bit operands)\n";
    for (int trial = 0; trial < 30; trial++) {
        std::string a_str = random_decimal(512 + (trial % 64));
        std::string b_str = random_decimal(256 + (trial % 128) + 1);

        BigNum a = BigNum::from_decimal(a_str);
        BigNum b = BigNum::from_decimal(b_str);
        mpz_class ga(a_str), gb(b_str);

        expect_eq("add", (a + b).to_decimal(), mpz_class(ga + gb).get_str(10));
        expect_eq("sub (a-b)", (a - b).to_decimal(), mpz_class(ga - gb).get_str(10));
        expect_eq("sub (b-a)", (b - a).to_decimal(), mpz_class(gb - ga).get_str(10));
        expect_eq("mul", (a * b).to_decimal(), mpz_class(ga * gb).get_str(10));

        BigNum rem;
        BigNum q = BigNum::divmod_mag(a, b, rem);
        mpz_class gq = ga / gb;
        mpz_class gr = ga % gb;
        expect_eq("div", q.to_decimal(), gq.get_str(10));
        expect_eq("mod", rem.to_decimal(), gr.get_str(10));
    }
    std::cout << "  " << total << " checks so far, " << failures << " failed\n\n";

    
    std::cout << "gcd / extended_gcd\n";
    for (int trial = 0; trial < 30; trial++) {
        std::string a_str = random_decimal(300 + trial);
        std::string b_str = random_decimal(200 + trial);
        BigNum a = BigNum::from_decimal(a_str);
        BigNum b = BigNum::from_decimal(b_str);
        mpz_class ga(a_str), gb(b_str);

        BigNum g = BigNum::gcd(a, b);
        mpz_class gg;
        mpz_gcd(gg.get_mpz_t(), ga.get_mpz_t(), gb.get_mpz_t());
        expect_eq("gcd", g.to_decimal(), gg.get_str(10));

        BigNum x, y;
        BigNum d = BigNum::Extended_gcd(a, b, x, y);
        mpz_class gx, gy, gd;
        mpz_gcdext(gd.get_mpz_t(), gx.get_mpz_t(), gy.get_mpz_t(), ga.get_mpz_t(), gb.get_mpz_t());
        expect_eq("extended_gcd d", d.to_decimal(), gd.get_str(10));

 
        mpz_class check = ga * mpz_class(x.to_decimal()) + gb * mpz_class(y.to_decimal());
        expect_eq("extended_gcd identity (a*x+b*y=gcd)", check.get_str(10), gd.get_str(10));
    }
    std::cout << "  " << total << " checks so far, " << failures << " failed\n\n";

    std::cout << "modular_inverse\n";
    int inv_tested = 0;
    for (int trial = 0; trial < 200 && inv_tested < 20; trial++) {
        std::string n_str = random_decimal(64 + (trial % 20) * 8);
        mpz_class gn(n_str);
        if (gn <= 1) continue;
        std::string a_str = random_decimal(64 + (trial % 20) * 4);
        mpz_class ga(a_str);
        ga = ((ga % gn) + gn) % gn;

        mpz_class g;
        mpz_gcd(g.get_mpz_t(), ga.get_mpz_t(), gn.get_mpz_t());
        if (g != 1) continue; // only coprime pairs have an inverse

        mpz_class ginv;
        mpz_invert(ginv.get_mpz_t(), ga.get_mpz_t(), gn.get_mpz_t());

        BigNum bn_a = BigNum::from_decimal(ga.get_str(10));
        BigNum bn_n = BigNum::from_decimal(gn.get_str(10));
        BigNum inv = BigNum::modular_inverse(bn_a, bn_n);
        expect_eq("modular_inverse", inv.to_decimal(), ginv.get_str(10));
        inv_tested++;
    }
    std::cout << "  (" << inv_tested << " coprime pairs tested)  " << total << " checks so far, " << failures << " failed\n\n";

  
    std::cout << "mod_pow_naive / mod_pow_fast (512-bit)\n";
    {
        std::string base_str = random_decimal(512);
        std::string exp_str  = "65537"; // standard RSA public exponent
        std::string mod_str  = random_decimal(512);

        BigNum base = BigNum::from_decimal(base_str);
        BigNum exp  = BigNum::from_decimal(exp_str);
        BigNum mod  = BigNum::from_decimal(mod_str);
        BigNum result_fast = BigNum::mod_pow_fast(base, exp, mod);

        mpz_class gbase(base_str), gexp(exp_str), gmod(mod_str), gresult;
        mpz_powm(gresult.get_mpz_t(), gbase.get_mpz_t(), gexp.get_mpz_t(), gmod.get_mpz_t());
        expect_eq("mod_pow_fast (e=65537)", result_fast.to_decimal(), gresult.get_str(10));
    }
    {
      
        std::string base_str = random_decimal(512);
        std::string exp_str  = "97";
        std::string mod_str  = random_decimal(512);

        BigNum base = BigNum::from_decimal(base_str);
        BigNum exp  = BigNum::from_decimal(exp_str);
        BigNum mod  = BigNum::from_decimal(mod_str);
        BigNum result_naive = BigNum::mod_pow_naive(base, exp, mod);
        BigNum result_fast  = BigNum::mod_pow_fast(base, exp, mod);

        mpz_class gbase(base_str), gexp(exp_str), gmod(mod_str), gresult;
        mpz_powm(gresult.get_mpz_t(), gbase.get_mpz_t(), gexp.get_mpz_t(), gmod.get_mpz_t());
        expect_eq("mod_pow_naive", result_naive.to_decimal(), gresult.get_str(10));
        expect_eq("mod_pow_fast", result_fast.to_decimal(), gresult.get_str(10));
    }
    std::cout << "  " << total << " checks so far, " << failures << " failed\n\n";

    
    std::cout << "modular_add / modular_mul (including negative operands)\n";
    for (int trial = 0; trial < 15; trial++) {
        std::string a_str = (trial % 3 == 0 ? "-" : "") + random_decimal(200);
        std::string b_str = (trial % 4 == 0 ? "-" : "") + random_decimal(200);
        std::string m_str = random_decimal(150 + trial);

        BigNum a = BigNum::from_decimal(a_str);
        BigNum b = BigNum::from_decimal(b_str);
        BigNum m = BigNum::from_decimal(m_str);

        BigNum radd = BigNum::modular_add(a, b, m);
        BigNum rmul = BigNum::modular_mul(a, b, m);

        mpz_class ga(a_str), gb(b_str), gm(m_str);
        mpz_class gadd = ((ga + gb) % gm + gm) % gm; 
        mpz_class gmul = ((ga * gb) % gm + gm) % gm;

        expect_eq("modular_add", radd.to_decimal(), gadd.get_str(10));
        expect_eq("modular_mul", rmul.to_decimal(), gmul.get_str(10));
    }
    std::cout << "  " << total << " checks so far, " << failures << " failed\n\n";

    std::cout << "========================================\n";
    std::cout << (total - failures) << " / " << total << " checks passed against GMP.\n";
    if (failures == 0) {
        std::cout << "ALL CHECKS PASSED.\n";
    } else {
        std::cout << failures << " CHECKS FAILED.\n";
    }
    return failures == 0 ? 0 : 1;
}