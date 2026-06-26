#!/bin/bash
# run_tests.sh -- Validates plugin behavior
set -euo pipefail

CLANGXX="${1:?Usage: $0 <clang++-binary> <plugin.so> <systemc-include-dir>}"
PLUGIN="${2:?Usage: $0 <clang++-binary> <plugin.so> <systemc-include-dir>}"
SYSTEMC_INC="${3:?Usage: $0 <clang++-binary> <plugin.so> <systemc-include-dir>}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0
FAIL=0

PLUGIN_FLAGS="-std=c++17 -fsyntax-only -Xclang -load -Xclang $PLUGIN -Xclang -add-plugin -Xclang sc-int-assign-checker -I$SYSTEMC_INC"
ANNOTATOR_FLAGS="-std=c++17 -fsyntax-only -Xclang -ast-dump -Xclang -load -Xclang $PLUGIN -Xclang -add-plugin -Xclang sc-dt-type-annotator -I$SYSTEMC_INC"

echo "=== Test 1: violations should be detected ==="
OUTPUT=$("$CLANGXX" $PLUGIN_FLAGS "$SCRIPT_DIR/test_basic_violations.cpp" 2>&1) || true
if echo "$OUTPUT" | grep -q "not allowed"; then
  COUNT=$(echo "$OUTPUT" | grep -c "not allowed")
  echo "PASS: $COUNT violation(s) detected"
  PASS=$((PASS + 1))
else
  echo "FAIL: no violations detected in test_basic_violations.cpp"
  echo "$OUTPUT"
  FAIL=$((FAIL + 1))
fi

echo ""
echo "=== Test 2: no false positives ==="
OUTPUT=$("$CLANGXX" $PLUGIN_FLAGS "$SCRIPT_DIR/test_no_violations.cpp" 2>&1) || true
if echo "$OUTPUT" | grep -q "not allowed"; then
  echo "FAIL: false positives detected in test_no_violations.cpp"
  echo "$OUTPUT"
  FAIL=$((FAIL + 1))
else
  echo "PASS: no false positives"
  PASS=$((PASS + 1))
fi

echo ""
echo "=== Test 3: sc_dt type declarations should be annotated ==="
OUTPUT=$("$CLANGXX" $ANNOTATOR_FLAGS "$SCRIPT_DIR/test_sc_dt_annotations.cpp" 2>&1) || true
if echo "$OUTPUT" | grep -q "AnnotateAttr.*sc_dt::sc_int" \
  && echo "$OUTPUT" | grep -q "AnnotateAttr.*sc_dt::sc_uint" \
  && echo "$OUTPUT" | grep -q "AnnotateAttr.*sc_dt::sc_biguint"; then
  echo "PASS: sc_dt annotations are present in AST dump"
  PASS=$((PASS + 1))
else
  echo "FAIL: missing expected sc_dt annotations in AST dump"
  echo "$OUTPUT"
  FAIL=$((FAIL + 1))
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
exit $FAIL
