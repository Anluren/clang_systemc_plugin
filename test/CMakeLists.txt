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
# '!' must prefix the whole pipeline; '| ! grep' is not valid POSIX sh.
add_custom_target(check_no_violations
  COMMAND sh -c "! (${CLANG_BINARY} ${CMAKE_CURRENT_SOURCE_DIR}/test_no_violations.cpp $<JOIN:${PLUGIN_FLAGS}, > 2>&1 | grep -q 'not allowed')"
  DEPENDS ScIntAssignChecker
  COMMENT "Running plugin on test_no_violations.cpp (expect no warnings)"
  VERBATIM
)

# ---------------------------------------------------------------------------
# Object library per test source + test_compile_commands.json generation.
#
# file(GLOB) discovers test sources automatically. For each source an OBJECT
# library carries the plugin compile options. get_target_property reads those
# options at configure time (generator expressions are still unevaluated);
# file(GENERATE) resolves them when writing the JSON.
#
# SHELL: groups each -Xclang pair so CMake does not deduplicate the repeated
# -Xclang tokens. The prefix is stripped before writing to the JSON since it
# is a CMake-internal marker not recognised by the compiler.
#
# After (re)configuring, run:  python3 .vscode/gen_launch.py
# ---------------------------------------------------------------------------
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

set(_compdb_entries "")
set(_sep "")

foreach(SRC ${TEST_SOURCES})
  get_filename_component(TEST_NAME ${SRC} NAME_WE)

  add_library(${TEST_NAME}_obj OBJECT ${SRC})
  set_target_properties(${TEST_NAME}_obj PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    EXCLUDE_FROM_ALL TRUE
  )
  target_compile_options(${TEST_NAME}_obj PRIVATE
    "SHELL:-Xclang -load -Xclang $<TARGET_FILE:ScIntAssignChecker>"
    "SHELL:-Xclang -add-plugin -Xclang sc-int-assign-checker"
    -I${SYSTEMC_INCLUDE_DIR}
  )
  add_dependencies(${TEST_NAME}_obj ScIntAssignChecker)

  get_target_property(TARGET_COMPILE_OPTS ${TEST_NAME}_obj COMPILE_OPTIONS)

  list(JOIN TARGET_COMPILE_OPTS " " _opts)
  # Strip the SHELL: prefix — it is a CMake deduplication guard, not a
  # compiler flag, and must be removed before embedding in the JSON command.
  string(REPLACE "SHELL:" "" _opts "${_opts}")

  set(_cmd "${CLANG_BINARY} -std=c++17 -fsyntax-only ${_opts} ${SRC}")

  string(APPEND _compdb_entries
    "${_sep}  {\n"
    "    \"directory\": \"${CMAKE_BINARY_DIR}\",\n"
    "    \"command\": \"${_cmd}\",\n"
    "    \"file\": \"${SRC}\"\n"
    "  }")
  set(_sep ",\n")
endforeach()

file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/test_compile_commands.json"
  CONTENT "[\n${_compdb_entries}\n]\n"
)
