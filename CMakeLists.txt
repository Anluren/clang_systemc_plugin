cmake_minimum_required(VERSION 3.16)
project(ScIntAssignChecker C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Clang 17 (transitively finds LLVM via ClangConfig.cmake)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Match LLVM/Clang RTTI setting
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# SystemC include path (needed for test compilation)
if(DEFINED ENV{SYSTEMC_HOME} AND NOT SYSTEMC_HOME)
  set(SYSTEMC_HOME "$ENV{SYSTEMC_HOME}")
endif()
set(SYSTEMC_HOME "${SYSTEMC_HOME}" CACHE PATH "SystemC install root")
set(SYSTEMC_INCLUDE_DIR "${SYSTEMC_HOME}/include")

# Clang binary for running tests with the plugin
set(CLANG_BINARY "${CLANG_INSTALL_PREFIX}/bin/clang++")

# Build as shared library (clang plugin).
# Do NOT link against Clang/LLVM libraries — the plugin is loaded into clang++
# which already contains these symbols. Linking them again causes duplicate
# command-line option registration crashes.
add_library(ScIntAssignChecker SHARED
  src/BuiltinToScIntChecker.cpp
)

# Allow undefined symbols (resolved from host clang++ at load time)
if(APPLE)
  target_link_options(ScIntAssignChecker PRIVATE -undefined dynamic_lookup)
endif()

add_subdirectory(test)
