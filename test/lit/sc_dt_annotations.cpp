// RUN: %clang -fsyntax-only -Xclang -ast-dump -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang sc-dt-type-annotator -I%systemc_inc %s 2>&1 | %filecheck %s --check-prefix=ANN

#include <systemc.h>

using namespace sc_dt;

struct Packet {
  sc_uint<16> id;
  int raw;
};

void test_annotations(sc_int<8> p) {
  sc_bv<4> bits;
  (void)p;
  (void)bits;
}

// ANN: AnnotateAttr{{.*}}"sc_dt::sc_uint"
// ANN: AnnotateAttr{{.*}}"sc_dt::sc_int"
// ANN: AnnotateAttr{{.*}}"sc_dt::sc_bv"
// ANN-NOT: AnnotateAttr{{.*}}"sc_dt::int"
