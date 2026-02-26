#include <systemc.h>

using namespace sc_dt;

void test_no_violations() {
  sc_int<8> si8;
  sc_uint<16> sui16;

  // Case 1: literal assignment (NOT a variable -- should NOT warn)
  si8 = 42;

  // Case 2: sc_int to sc_int (NOT builtin -- should NOT warn)
  sc_int<8> si8b;
  si8 = si8b;

  // Case 3: sc_uint to sc_uint (NOT builtin -- should NOT warn)
  sc_uint<16> sui16b;
  sui16 = sui16b;

  // Case 4: literal compound assignment (should NOT warn)
  si8 += 1;

  // Case 5: regular int-to-int assignment (LHS not sc_int -- should NOT warn)
  int a = 1, b = 2;
  a = b;

  // Case 6: constructor initialization with literal (not operator=)
  sc_int<8> si8c(42);

  // Case 7: copy init with literal (not operator= on variable)
  sc_int<8> si8d = 42;
}
