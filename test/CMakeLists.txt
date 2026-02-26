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

# ---------------------------------------------------------------------------
# Generate test_compile_commands.json for IDE/debugger integration.
# file(GENERATE) supports generator expressions, so $<TARGET_FILE:ScIntAssignChecker>
# resolves to the actual .so path at cmake generation time.
# After (re)configuring, run:  python3 .vscode/gen_launch.py
# ---------------------------------------------------------------------------
set(_plugin_cmd "${CLANG_BINARY} -std=c++17 -fsyntax-only")
string(APPEND _plugin_cmd " -Xclang -load -Xclang $<TARGET_FILE:ScIntAssignChecker>")
string(APPEND _plugin_cmd " -Xclang -add-plugin -Xclang sc-int-assign-checker")
string(APPEND _plugin_cmd " -I${SYSTEMC_INCLUDE_DIR}")

string(CONCAT _compdb_content
  "[\n"
  "  {\n"
  "    \"directory\": \"${CMAKE_BINARY_DIR}\",\n"
  "    \"command\": \"${_plugin_cmd} ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_violations.cpp\",\n"
  "    \"file\": \"${CMAKE_CURRENT_SOURCE_DIR}/test_basic_violations.cpp\"\n"
  "  },\n"
  "  {\n"
  "    \"directory\": \"${CMAKE_BINARY_DIR}\",\n"
  "    \"command\": \"${_plugin_cmd} ${CMAKE_CURRENT_SOURCE_DIR}/test_no_violations.cpp\",\n"
  "    \"file\": \"${CMAKE_CURRENT_SOURCE_DIR}/test_no_violations.cpp\"\n"
  "  }\n"
  "]\n"
)

file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/test_compile_commands.json"
  CONTENT "${_compdb_content}"
)
