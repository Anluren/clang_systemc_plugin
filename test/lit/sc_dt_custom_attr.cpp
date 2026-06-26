// RUN: %clang -fsyntax-only -Xclang -ast-dump -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang sc-dt-type-annotator -I%systemc_inc %s 2>&1 | %filecheck %s --check-prefix=CUSTOM

[[sc_dt::sc_uint]] int direct_attr;
[[sc_dt::sc_biguint]] int big_attr;
[[sc_dt::sc_int]] int int_attr;

// CUSTOM: VarDecl{{.*}}direct_attr
// CUSTOM: AnnotateAttr{{.*}}"sc_dt::sc_uint"
// CUSTOM: VarDecl{{.*}}big_attr
// CUSTOM: AnnotateAttr{{.*}}"sc_dt::sc_biguint"
// CUSTOM: VarDecl{{.*}}int_attr
// CUSTOM: AnnotateAttr{{.*}}"sc_dt::sc_int"
