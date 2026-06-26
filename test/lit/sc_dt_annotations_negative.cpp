// RUN: %clang -fsyntax-only -Xclang -ast-dump -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang sc-dt-type-annotator -I%systemc_inc %s 2>&1 | %filecheck %s --check-prefix=NEG

int global_counter = 0;

struct PlainData {
  int id;
  double value;
};

using LocalAlias = long;

void test_no_sc_dt_annotations(int p, LocalAlias q) {
  PlainData d{p, static_cast<double>(q)};
  (void)d;
}

// NEG: TranslationUnitDecl
// NEG-NOT: AnnotateAttr{{.*}}"sc_dt::
