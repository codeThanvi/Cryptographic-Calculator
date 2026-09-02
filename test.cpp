#include "level1.hpp"
#include <iostream>
#include <cassert>

int main() {
    BigNum a = BigNum::from_decimal("123456789012345678901234567890123456789012345678901234567890");
    BigNum b = BigNum::from_decimal("987654321098765432109876543210987654321098765432109876543210");

    BigNum sum = a + b;
    BigNum diff1 = a - b;
    BigNum diff2 = b - a;

    std::cout << "a+b = " << sum.to_decimal() << "\n";
    std::cout << "a-b = " << diff1.to_decimal() << "\n";
    std::cout << "b-a = " << diff2.to_decimal() << "\n";

    assert(sum.to_decimal()   == "1111111110111111111011111111101111111110111111111011111111100");
    assert(diff1.to_decimal() == "-864197532086419753208641975320864197532086419753208641975320");
    assert(diff2.to_decimal() == "864197532086419753208641975320864197532086419753208641975320");

    // round trip + comparisons
    assert(a < b);
    assert(b >= a);
    assert(!(a == b));
    assert((a + b) - b == a);

    // small numbers / edge cases
    assert((BigNum(5) - BigNum(5)).is_zero());
    assert((BigNum(0) - BigNum(3)).to_decimal() == "-3");
    assert((BigNum(-3) + BigNum(3)).is_zero());

    std::cout << "All tests passed.\n";
    return 0;
}