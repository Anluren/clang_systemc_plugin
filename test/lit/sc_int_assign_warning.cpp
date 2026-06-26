// RUN: %clang -fsyntax-only -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang sc-int-assign-checker -I%systemc_inc %s 2>&1 | %filecheck %s

#include <systemc.h>

using namespace sc_dt;

void test_assign_warning() {
  sc_int<8> lhs;
  int rhs = 7;
  lhs = rhs;
}

// CHECK: warning: assignment of builtin-type variable 'rhs' (type 'int') to sc_int/sc_uint type 'sc_int<8>' is not allowed
