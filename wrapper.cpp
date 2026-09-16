#include "level1.hpp"
#include <emscripten.h>
#include <cstring>
#include <cstdlib>
#include <string>

extern "C" {

// Validates that a string is a plain decimal integer, optionally signed
// (an optional leading '-' followed by at least one digit).
static bool is_valid_decimal(const char* s) {
    if (!s || !*s) return false;
    const char* p = s;
    if (*p == '-') p++;
    if (!*p) return false; // "-" alone isn't a number
    for (; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    return true;
}

EMSCRIPTEN_KEEPALIVE
char* bignum_calculate(const char* aStr, const char* bStr, const char* cStr, const char* op) {
    std::string result;
    std::string o(op);

    bool needs_c = (o == "modadd" || o == "modmul" || o == "modpow");
    bool a_ok = is_valid_decimal(aStr);
    bool b_ok = is_valid_decimal(bStr);
    bool c_ok = !needs_c || is_valid_decimal(cStr);

    if (!a_ok || !b_ok || !c_ok) {
        result = "Error: enter whole numbers only";
    } else {
        try {
            BigNum a = BigNum::from_decimal(aStr);
            BigNum b = BigNum::from_decimal(bStr);

            if (o == "add") {
                result = (a + b).to_decimal();
            } else if (o == "sub") {
                result = (a - b).to_decimal();
            } else if (o == "mul") {
                result = (a * b).to_decimal();
            } else if (o == "div") {
                if (b.is_zero()) result = "Error: division by zero";
                else { BigNum rem; result = BigNum::divmod_mag(a, b, rem).to_decimal(); }
            } else if (o == "mod") {
                if (b.is_zero()) result = "Error: division by zero";
                else { BigNum rem; BigNum::divmod_mag(a, b, rem); result = rem.to_decimal(); }
            } else if (o == "modinv") {
                // here b is the modulus n
                if (b.is_zero()) result = "Error: modulus must be nonzero";
                else result = BigNum::modular_inverse(a, b).to_decimal();
            } else if (o == "modadd") {
                BigNum c = BigNum::from_decimal(cStr);
                if (c.is_zero()) result = "Error: modulus must be nonzero";
                else result = BigNum::modular_add(a, b, c).to_decimal();
            } else if (o == "modmul") {
                BigNum c = BigNum::from_decimal(cStr);
                if (c.is_zero()) result = "Error: modulus must be nonzero";
                else result = BigNum::modular_mul(a, b, c).to_decimal();
            } else if (o == "modpow") {
                // here a=base, b=exponent, c=modulus
                BigNum c = BigNum::from_decimal(cStr);
                if (c.is_zero()) result = "Error: modulus must be nonzero";
                else if (b.negative) result = "Error: exponent must be non-negative";
                else result = BigNum::mod_pow_fast(a, b, c).to_decimal();
            } else if (o == "gcd") {
                result = BigNum::gcd(a, b).to_decimal();
            } else {
                result = "Error: unknown operation";
            }
        } catch (const std::exception& e) {
            result = std::string("Error: ") + e.what();
        }
    }

    char* out = (char*)malloc(result.size() + 1);
    std::strcpy(out, result.c_str());
    return out;
}

EMSCRIPTEN_KEEPALIVE
void free_result(char* ptr) {
    free(ptr);
}

}