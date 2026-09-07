#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <algorithm>



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
    friend bool operator<=(const BigNum &a , const BigNum &b){return cmp(a,b) <= 0; }
    friend bool operator==(const BigNum& a, const BigNum& b) { return cmp(a, b) == 0; }
    friend bool operator>=(const BigNum& a, const BigNum& b) { return cmp(a, b) >= 0; }

    static BigNum mul_small(const BigNum& a, uint32_t m) {
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

    static BigNum from_decimal(const std::string& s) {
        BigNum r;
        for(int i = 0 ; i < (int)s.length() ; i++){
            if(i == 0 && s[i] == '-')continue;
            r = mul_small(r ,10);
            r = add_magnitude_small(r , s[i] - '0');
        }

        if(s[0] == '-')r.negative = true;
        r.trim();

        return r;
    }

    static BigNum divmod_small(const BigNum& a, uint32_t d, uint32_t& remainder) {
        BigNum q;
        q.limbs.resize(a.limbs.size());
        remainder = 0;
        int n = a.limbs.size();
        for(int i = n - 1 ; i>=0 ; i--){
            uint64_t current = (uint64_t)remainder << 32 | a.limbs[i];
            q.limbs[i] = current/d;
            remainder = current % d;
        }
        q.trim();
        return q;
    }

    std::string to_decimal() const {
        if(this->is_zero())return "0";
        std :: string s = "";
        BigNum curr = *this;
        uint32_t remainder = 0;
        while(!curr.is_zero()){
              curr = divmod_small(curr,10,remainder);
              s += '0' + remainder;
        }
        std :: reverse(s.begin() , s.end());
        if(this->negative)s.insert(0,"-");
        return s;
    }

    static std::vector<uint32_t> mul_mag(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
        std :: vector<u_int32_t> result;
        BigNum temp;
        temp.limbs = a;
        temp.negative = false;

        for(int i = 0 ; i < b.size(); i++){
            BigNum res = mul_small(temp,b[i]);
            res.limbs.insert(res.limbs.begin(), i, 0u);
            result = add_mag(res.limbs,result);
        }

        return result;
    }

    friend BigNum operator*(const BigNum& a, const BigNum& b) {
        BigNum r;
        r.limbs = mul_mag(a.limbs, b.limbs);
        r.negative = (a.negative != b.negative);
        r.trim();
        return r;
    }


    static BigNum divmod_mag(const BigNum &a , const BigNum &b , BigNum &Remainder){
        Remainder = 0;
        BigNum q = 0;
        q.limbs.resize(a.limbs.size());
        BigNum temp = a;





        for(int i = a.limbs.size() - 1 ; i >= 0 ; i--){
              Remainder.limbs.insert(Remainder.limbs.begin(), 1 , 0u);
              Remainder = Remainder + a.limbs[i];
              uint32_t low = 0;
              uint32_t high = UINT32_MAX;
              while(low < high){
              uint32_t mid = (uint64_t)low + ((uint64_t)high - low + 1)/2;
              if(mul_small(b,mid) <= Remainder)low = mid;
              else high = mid - 1;
        }
        q.limbs[i] = low;
        Remainder = Remainder - mul_small(b,low);
    }
        q.negative = (a.negative != b.negative);
        q.trim();
        return q;

    }


    friend BigNum operator/(const BigNum& a, const BigNum& b) {
        BigNum remainder;
        BigNum q = divmod_mag(a, b, remainder);
        q.negative = (a.negative != b.negative);
        q.trim();
        return q;
    }
 
    friend BigNum operator%(const BigNum& a, const BigNum& b) {
        BigNum remainder;
        divmod_mag(a, b, remainder);
        remainder.negative = a.negative;
        remainder.trim();
        return remainder;
    }




    static BigNum Extended_gcd(const BigNum& a, const BigNum& b, BigNum &x , BigNum &y) {
        if(b == 0){
             x = 1;
             y = 0;
             return a;
        }

        BigNum x1 , y1;
        BigNum d = Extended_gcd(b,a%b,x1,y1);
        x = y1;
        y = x1 - y1 * (a/b);

        return d;

    }



};