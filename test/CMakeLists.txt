# Custom targets to run clang++ with the plugin loaded on test files.

set(PLUGIN_FLAGS
  -std=c++17
  -fsyntax-only
  -Xclang -load -Xclang $<TARGET_FILE:ScIntAssignChecker>
  -Xclang -add-plugin -Xclang sc-int-assign-checker
  -I${SYSTEMC_INCLUDE_DIR}
)

# check_violations: expect warnings (grep for "not allowed" in output).
add_custom_target(check_violations
  COMMAND sh -c "${CLANG_BINARY} ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_violations.cpp $<JOIN:${PLUGIN_FLAGS}, > 2>&1 | grep -q 'not allowed'"
  DEPENDS ScIntAssignChecker
  COMMENT "Running plugin on test_basic_violations.cpp (expect warnings)"
  VERBATIM
)

# check_no_violations: expect clean output (exit 0, no warnings).
add_custom_target(check_no_violations
  COMMAND sh -c "${CLANG_BINARY} ${CMAKE_CURRENT_SOURCE_DIR}/test_no_violations.cpp $<JOIN:${PLUGIN_FLAGS}, > 2>&1 | ! grep -q 'not allowed'"
  DEPENDS ScIntAssignChecker
  COMMENT "Running plugin on test_no_violations.cpp (expect no warnings)"
  VERBATIM
)
