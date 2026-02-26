#include <systemc.h>

using namespace sc_dt;

void test_violations() {
    sc_int<8> si8;
    sc_uint<16> sui16;

    // Case 1: int variable to sc_int (SHOULD WARN)
    int i = 42;
    si8 = i;

    // Case 2: unsigned int variable to sc_uint (SHOULD WARN)
    unsigned int ui = 10;
    sui16 = ui;

    // Case 3: long variable to sc_int (SHOULD WARN)
    long l = 100L;
    si8 = l;

    // Case 4: double variable to sc_int (SHOULD WARN)
    double d = 3.14;
    si8 = d;

    // Case 5: compound assignment with int variable (SHOULD WARN)
    int addend = 5;
    si8 += addend;

    // Case 6: bitwise compound assignment with int variable (SHOULD WARN)
    int mask = 0xFF;
    sui16 &= mask;

    // Case 7: short variable (implicit conversion to int) (SHOULD WARN)
    short s = 3;
    si8 = s;

    // Case 8: bool variable to sc_int (SHOULD WARN)
    bool flag = true;
    si8 = flag;
}
