#include "bignum.hpp"
#include <emscripten.h>
#include <cstring>
#include <cstdlib>
#include <string>

extern "C" {

// Validates that a string is a plain non-negative decimal number
// (this calculator's from_decimal doesn't handle a leading sign, so we
// reject anything that isn't purely digits before calling into it).
static bool is_valid_decimal(const char* s) {
    if (!s || !*s) return false;
    for (const char* p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    return true;
}

EMSCRIPTEN_KEEPALIVE
char* bignum_calculate(const char* aStr, const char* bStr, const char* op) {
    std::string result;

    if (!is_valid_decimal(aStr) || !is_valid_decimal(bStr)) {
        result = "Error: enter non-negative whole numbers only";
    } else {
        BigNum a = BigNum::from_decimal(aStr);
        BigNum b = BigNum::from_decimal(bStr);
        std::string o(op);

        if (o == "add") {
            result = (a + b).to_decimal();
        } else if (o == "sub") {
            result = (a - b).to_decimal();
        } else if (o == "mul") {
            result = (a * b).to_decimal();
        } else if (o == "div") {
            if (b.is_zero()) {
                result = "Error: division by zero";
            } else {
                BigNum rem;
                BigNum q = BigNum::divmod_mag(a, b, rem);
                result = q.to_decimal();
            }
        } else if (o == "mod") {
            if (b.is_zero()) {
                result = "Error: division by zero";
            } else {
                BigNum rem;
                BigNum q = a/b;
                result = rem.to_decimal();
            }
        } else {
            result = "Error: unknown operation";
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
