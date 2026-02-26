#!/bin/bash
# run_tests.sh -- Validates checker behavior
set -euo pipefail

CHECKER="${1:?Usage: $0 <path-to-checker-binary> <systemc-include-dir> [clang-resource-dir]}"
SYSTEMC_INC="${2:?Usage: $0 <path-to-checker-binary> <systemc-include-dir> [clang-resource-dir]}"
RESOURCE_DIR="${3:-}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0
FAIL=0

EXTRA_FLAGS=(-std=c++17 -I"$SYSTEMC_INC")
if [ -n "$RESOURCE_DIR" ]; then
  EXTRA_FLAGS+=(-resource-dir "$RESOURCE_DIR")
fi

echo "=== Test 1: violations should be detected ==="
OUTPUT=$("$CHECKER" "$SCRIPT_DIR/test_basic_violations.cpp" -- "${EXTRA_FLAGS[@]}" 2>&1) || true
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
OUTPUT=$("$CHECKER" "$SCRIPT_DIR/test_no_violations.cpp" -- "${EXTRA_FLAGS[@]}" 2>&1) || true
if echo "$OUTPUT" | grep -q "not allowed"; then
  echo "FAIL: false positives detected in test_no_violations.cpp"
  echo "$OUTPUT"
  FAIL=$((FAIL + 1))
else
  echo "PASS: no false positives"
  PASS=$((PASS + 1))
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
exit $FAIL
