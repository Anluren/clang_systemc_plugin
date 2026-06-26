#include <systemc.h>

using namespace sc_dt;

struct Packet {
  sc_uint<16> id;
  int raw;
};

using Wide = sc_biguint<64>;

void test_sc_dt_annotations(sc_int<8> param) {
  sc_int<8> a = 1;
  sc_bv<4> bits;
  Wide w;
  (void)a;
  (void)bits;
  (void)w;
  (void)param;
}
