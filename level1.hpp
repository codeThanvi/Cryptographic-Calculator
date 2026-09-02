#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include<math.h>


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

    void trim() {
        
        while(limbs.size() > 1 && limbs.back() == 0)limbs.pop_back();
        if(limbs.size() == 1 && limbs[0] == 0)negative = 0;
        // TODO: implement
    }

    bool is_zero() const { return limbs.size() == 1 && limbs[0] == 0; }


    static int cmp_mag(const BigNum& a, const BigNum& b) {
        // TODO: implement
        if(a.limbs.size() > b.limbs.size())return 1;
        if(a.limbs.size() < b.limbs.size())return -1;

        int n = a.limbs.size();
        
        for(int i = n-1 ; i >= 0 ; i--){
            if(a.limbs[i] < b.limbs[i] )return -1;
            if(a.limbs[i] > b.limbs[i]) return 1;
        }

        return 0;
    }

    // Full signed compare — GIVEN, built on top of your cmp_mag.
    static int cmp(const BigNum& a, const BigNum& b) {
        if (a.negative != b.negative) return a.negative ? -1 : 1;
        int m = cmp_mag(a, b);
        return a.negative ? -m : m;
    }

    
    static std::vector<uint32_t> add_mag(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {

        
        std:: vector<uint32_t>out;
        uint64_t carry = 0;

        int i;

        for(i = 0; i < std :: min(a.size() , b.size()); i++){

            uint64_t sum = static_cast<uint64_t> (a[i]) + b[i] + carry;
            out.push_back(sum & 0xFFFFFFFF);
            carry = sum >> 32;

        }

        if((a.size() == b.size()) && carry == 1)out.push_back(1);

        else{
        while(i < a.size()){
            out.push_back(a[i] + carry);
            carry = (a[i] + carry)>>32;
            i++;
        }

        while(i < b.size()){
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
        // TODO: implement
        uint32_t borrow = 0;
        std:: vector<uint32_t>out;

        int i;

        for(i = 0; i< std :: min(a.size() , b.size()); i++)
        {
            int64_t difference = static_cast<int64_t> (a[i]) - b[i] - borrow;
            if(difference < 0){
                difference += (1ll << 32);
                borrow = 1;
            }else borrow = 0;
            out.push_back(difference);

        }
        while(i < a.size()){
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

    // ---- GIVEN: signed +/- dispatch onto your primitives above --------
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

    // ---- GIVEN: decimal string I/O (plumbing, not today's topic) ------
    static BigNum mul_small(const BigNum& a, uint32_t m) {
        BigNum r; r.limbs.clear();
        uint64_t carry = 0;
        for (uint32_t limb : a.limbs) {
            uint64_t prod = static_cast<uint64_t>(limb) * m + carry;
            r.limbs.push_back(static_cast<uint32_t>(prod & 0xFFFFFFFFu));
            carry = prod >> 32;
        }
        while (carry) { r.limbs.push_back(static_cast<uint32_t>(carry & 0xFFFFFFFFu)); carry >>= 32; }
        if (r.limbs.empty()) r.limbs.push_back(0);
        r.negative = a.negative;
        r.trim();
        return r;
    }

    static BigNum add_magnitude_small(const BigNum& a, uint32_t v) {
        BigNum r = a;
        uint64_t carry = v;
        for (size_t i = 0; i < r.limbs.size() && carry; ++i) {
            uint64_t sum = static_cast<uint64_t>(r.limbs[i]) + carry;
            r.limbs[i] = static_cast<uint32_t>(sum & 0xFFFFFFFFu);
            carry = sum >> 32;
        }
        if (carry) r.limbs.push_back(static_cast<uint32_t>(carry));
        return r;
    }

    static BigNum from_decimal(const std::string& s) {
        BigNum result(0);
        size_t i = 0;
        bool neg = false;
        if (!s.empty() && (s[0] == '-' || s[0] == '+')) { neg = (s[0] == '-'); i = 1; }
        if (i >= s.size()) throw std::invalid_argument("empty decimal string");
        for (; i < s.size(); ++i) {
            if (s[i] < '0' || s[i] > '9') throw std::invalid_argument("bad digit in decimal string");
            result = mul_small(result, 10);
            result = add_magnitude_small(result, static_cast<uint32_t>(s[i] - '0'));
        }
        result.negative = neg && !result.is_zero();
        return result;
    }

    static BigNum divmod_small(const BigNum& a, uint32_t d, uint32_t& remainder) {
        BigNum q; q.limbs.assign(a.limbs.size(), 0);
        uint64_t rem = 0;
        for (size_t i = a.limbs.size(); i-- > 0; ) {
            uint64_t cur = (rem << 32) | a.limbs[i];
            q.limbs[i] = static_cast<uint32_t>(cur / d);
            rem = cur % d;
        }
        q.negative = a.negative;
        q.trim();
        remainder = static_cast<uint32_t>(rem);
        return q;
    }

    std::string to_decimal() const {
        if (is_zero()) return "0";
        BigNum cur = *this;
        cur.negative = false;
        std::vector<uint32_t> chunks;
        while (!cur.is_zero()) {
            uint32_t rem;
            cur = divmod_small(cur, 1000000000u, rem);
            chunks.push_back(rem);
        }
        std::string out = negative ? "-" : "";
        out += std::to_string(chunks.back());
        for (size_t i = chunks.size() - 1; i-- > 0; ) {
            std::string chunk = std::to_string(chunks[i]);
            out += std::string(9 - chunk.size(), '0') + chunk;
        }
        return out;
    }
};