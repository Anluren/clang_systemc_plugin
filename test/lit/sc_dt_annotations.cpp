// RUN: %clang -fsyntax-only -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang sc-dt-type-annotator -Xclang -plugin-arg-sc-dt-type-annotator -Xclang report-side-table -I%systemc_inc %s 2>&1 | %filecheck %s --check-prefix=TABLE

#include <systemc.h>

using namespace sc_dt;

struct Packet {
  sc_uint<16> id;
  int raw;
};

sc_int<8> produce_value(sc_uint<4> p) {
  return sc_int<8>(p);
}

int plain_function(int x) {
  return x;
}

void test_annotations(sc_int<8> p) {
  sc_bv<4> bits;
  (void)p;
  (void)bits;
}

// TABLE: remark: side table cached sc_dt type 'sc_dt::sc_uint'
// TABLE: remark: side table cached sc_dt type 'sc_dt::sc_int'
// TABLE: remark: side table cached sc_dt type 'sc_dt::sc_bv'
// TABLE-NOT: remark: side table cached sc_dt type 'sc_dt::int'
