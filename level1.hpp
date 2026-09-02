#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

// ============================================================================
// BigNum — arbitrary precision integer, base 2^32, little-endian limbs,
// sign-magnitude.
//
// DONE (yours, verified working): trim, cmp_mag, add_mag, sub_mag
// NEXT UP: mul_small — implement this one first. The others below it
// (add_magnitude_small, from_decimal, divmod_small, to_decimal) are left
// as TODO stubs too, but we'll work through the concept behind each one
// before you write it — don't jump ahead to them yet.
// ============================================================================

class BigNum {
public:
    std::vector<uint32_t> limbs; // little-endian magnitude
    bool negative = false;

    BigNum() : limbs{0}, negative(false) {}

    BigNum(int64_t v) {
        negative = v < 0;
        uint64_t mag = negative ? static_cast<uint64_t>(-(v + 1)) + 1 : static_cast<uint64_t>(v);
        limbs.push_back(static_cast<uint32_t>(mag & 0xFFFFFFFFu));
        limbs.push_back(static_cast<uint32_t>(mag >> 32));
        trim();
    }

    // ---- DONE (yours) --------------------------------------------------
    void trim() {
        while(limbs.size() > 1 && limbs.back() == 0)limbs.pop_back();
        if(limbs.size() == 1 && limbs[0] == 0)negative = 0;
    }

    bool is_zero() const { return limbs.size() == 1 && limbs[0] == 0; }

    static int cmp_mag(const BigNum& a, const BigNum& b) {
        if(a.limbs.size() > b.limbs.size())return 1;
        if(a.limbs.size() < b.limbs.size())return -1;
        int n = a.limbs.size();
        for(int i = n-1 ; i >= 0 ; i--){
            if(a.limbs[i] < b.limbs[i] )return -1;
            if(a.limbs[i] > b.limbs[i]) return 1;
        }
        return 0;
    }

    static int cmp(const BigNum& a, const BigNum& b) {
        if (a.negative != b.negative) return a.negative ? -1 : 1;
        int m = cmp_mag(a, b);
        return a.negative ? -m : m;
    }

    static std::vector<uint32_t> add_mag(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
        std:: vector<uint32_t>out;
        uint64_t carry = 0;
        int i;
        for(i = 0; i < (int)std::min(a.size() , b.size()); i++){
            uint64_t sum = static_cast<uint64_t>(a[i]) + b[i] + carry;
            out.push_back(sum & 0xFFFFFFFF);
            carry = sum >> 32;
        }
        if(((int)a.size() == (int)b.size()) && carry == 1)out.push_back(1);
        else{
            while(i < (int)a.size()){
                out.push_back(a[i] + carry);
                carry = (a[i] + carry)>>32;
                i++;
            }
            while(i < (int)b.size()){
                out.push_back(b[i] + carry);
                carry = (b[i] + carry)>>32;
                i++;
            }
            if(carry != 0)
                out.push_back(carry);
        }
        return out;
    }

    static std::vector<uint32_t> sub_mag(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
        uint32_t borrow = 0;
        std:: vector<uint32_t>out;
        int i;
        for(i = 0; i< (int)std::min(a.size() , b.size()); i++)
        {
            int64_t difference = static_cast<int64_t>(a[i]) - b[i] - borrow;
            if(difference < 0){
                difference += (1ll << 32);
                borrow = 1;
            }else borrow = 0;
            out.push_back(difference);
        }
        while(i < (int)a.size()){
            int64_t difference = (int64_t)a[i] - borrow;
            if(difference < 0){
                difference += (1ll << 32);
                borrow = 1;
            }else borrow = 0;
            out.push_back(difference);
            i++;
        }
        while(out.size() > 1 && out.back() == 0)out.pop_back();
        return out;
    }

    friend BigNum operator+(const BigNum& a, const BigNum& b) {
        BigNum r;
        if (a.negative == b.negative) {
            r.limbs = add_mag(a.limbs, b.limbs);
            r.negative = a.negative;
        } else {
            int c = cmp_mag(a, b);
            if (c == 0) { r.limbs = {0}; r.negative = false; }
            else if (c > 0) { r.limbs = sub_mag(a.limbs, b.limbs); r.negative = a.negative; }
            else { r.limbs = sub_mag(b.limbs, a.limbs); r.negative = b.negative; }
        }
        r.trim();
        return r;
    }

    friend BigNum operator-(const BigNum& a, const BigNum& b) {
        BigNum nb = b;
        if (!nb.is_zero()) nb.negative = !nb.negative;
        return a + nb;
    }

    friend BigNum operator-(const BigNum& a) {
        BigNum r = a;
        if (!r.is_zero()) r.negative = !r.negative;
        return r;
    }

    friend bool operator<(const BigNum& a, const BigNum& b)  { return cmp(a, b) < 0; }
    friend bool operator==(const BigNum& a, const BigNum& b) { return cmp(a, b) == 0; }
    friend bool operator>=(const BigNum& a, const BigNum& b) { return cmp(a, b) >= 0; }

    // ------------------------------------------------------------------
    // TODO (up next): mul_small(a, m)
    // Multiply a whole BigNum `a` by a single scalar `m` (a uint32_t).
    // Same shape as add_mag's loop, but each step is a MULTIPLY-with-carry
    // instead of an add-with-carry:
    //
    //   carry = 0
    //   for each limb L in a.limbs:
    //       product = (uint64_t)L * m + carry     // widen L to 64-bit FIRST
    //       push (product & 0xFFFFFFFF) as the next result limb
    //       carry = product >> 32
    //   IMPORTANT: unlike add_mag, carry here can be a large number (up to
    //   nearly 2^32), not just 0 or 1 — because L * m can be almost as big
    //   as 2^64. So after the loop, you can't just push one leftover limb;
    //   you need a `while (carry) { push low 32 bits; carry >>= 32; }` to
    //   peel off however many extra limbs the leftover carry needs.
    //
    // Return a BigNum (not just a vector) — set r.negative = a.negative,
    // and call r.trim() before returning (handles the case m == 0, and
    // strips any accidental leading zero limb).
    // ------------------------------------------------------------------
    static BigNum mul_small(const BigNum& a, uint32_t m) {
        // TODO: implement
        BigNum r;
        r.limbs.pop_back();
        uint64_t carry = 0;
        int n = a.limbs.size();
        for(int i = 0 ; i < n ; i++){
            uint64_t product = (uint64_t)a.limbs[i] * m + carry;
            r.limbs.push_back(product & 0xFFFFFFFF);
            carry = product >> 32;
        }
        r.limbs.push_back(carry);
        r.negative = a.negative;
        r.trim();

        return r;
    }

    // ------------------------------------------------------------------
    // TODO (later): add_magnitude_small(a, v)
    // We'll cover the concept when we get here — don't implement yet.
    // ------------------------------------------------------------------
    static BigNum add_magnitude_small(const BigNum& a, uint32_t v) {
        BigNum r = a;
        uint32_t carry = v;
        
        int n = a.limbs.size();

        for(int i = 0 ; i < n ; i++){
            uint64_t sum = (uint64_t)r.limbs[i] + carry;
            r.limbs[i] = (sum & 0xFFFFFFFF);
            carry = sum >> 32;
        }   

        r.limbs.push_back(carry);

        r.negative = a.negative;
        r.trim();
        return r;
    }

    // ------------------------------------------------------------------
    // TODO (later): from_decimal(s)
    // We'll cover the concept when we get here — don't implement yet.
    // ------------------------------------------------------------------
    static BigNum from_decimal(const std::string& s) {
        // TODO: implement
        BigNum r;
    
        for(int i = 0 ; i < s.length() ; i++){
            r = mul_small(r ,10);
            r = add_magnitude_small(r , s[i] - '0');
        }

        return r;
    }

    // ------------------------------------------------------------------
    // TODO (later): divmod_small(a, d, remainder)
    // We'll cover the concept when we get here — don't implement yet.
    // ------------------------------------------------------------------
    static BigNum divmod_small(const BigNum& a, uint32_t d, uint32_t& remainder) {
        // TODO: implement
        remainder = 0;
        

        return BigNum(0);
    }

    // ------------------------------------------------------------------
    // TODO (later): to_decimal()
    // We'll cover the concept when we get here — don't implement yet.
    // ------------------------------------------------------------------
    std::string to_decimal() const {
        // TODO: implement
        return "";
    }
};