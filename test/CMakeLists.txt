# Custom targets to run the checker on test files.
# These do NOT compile the test files as executables -- they invoke the checker tool.

# check_violations: the tool is expected to find violations (exit code 1).
# We invert the exit code so make sees success when violations are found.
add_custom_target(check_violations
  COMMAND sh -c "$<TARGET_FILE:sc_int_assign_checker> ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_violations.cpp -- -std=c++17 -resource-dir ${CLANG_RESOURCE_DIR} -I${SYSTEMC_INCLUDE_DIR} 2>&1 | grep -q 'not allowed'"
  DEPENDS sc_int_assign_checker
  COMMENT "Running checker on test_basic_violations.cpp (expect warnings)"
  VERBATIM
)

# check_no_violations: the tool is expected to find no violations (exit code 0).
add_custom_target(check_no_violations
  COMMAND $<TARGET_FILE:sc_int_assign_checker>
    ${CMAKE_CURRENT_SOURCE_DIR}/test_no_violations.cpp
    --
    -std=c++17
    -resource-dir ${CLANG_RESOURCE_DIR}
    -I${SYSTEMC_INCLUDE_DIR}
  DEPENDS sc_int_assign_checker
  COMMENT "Running checker on test_no_violations.cpp (expect no warnings)"
  VERBATIM
)
