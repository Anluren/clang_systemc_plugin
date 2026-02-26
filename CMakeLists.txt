cmake_minimum_required(VERSION 3.16)
project(SystemCBuiltinAssignChecker C CXX)

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

# Match LLVM/Clang RTTI setting -- they are typically built with -fno-rtti
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# SystemC include path (needed for test compilation, not the tool itself)
if(DEFINED ENV{SYSTEMC_HOME} AND NOT SYSTEMC_HOME)
  set(SYSTEMC_HOME "$ENV{SYSTEMC_HOME}")
endif()
set(SYSTEMC_HOME "${SYSTEMC_HOME}" CACHE PATH "SystemC install root")
set(SYSTEMC_INCLUDE_DIR "${SYSTEMC_HOME}/include")

add_executable(sc_int_assign_checker
  src/main.cpp
  src/BuiltinToScIntChecker.cpp
)

target_link_libraries(sc_int_assign_checker PRIVATE
  clangTooling
  clangASTMatchers
  clangFrontend
  clangSerialization
  clangDriver
  clangParse
  clangSema
  clangAnalysis
  clangAST
  clangBasic
  clangEdit
  clangLex
  clangRewrite
  clangToolingCore
)

llvm_map_components_to_libnames(llvm_libs support core)
target_link_libraries(sc_int_assign_checker PRIVATE ${llvm_libs})

# Determine Clang resource directory (needed for standalone tool to find builtin headers)
set(CLANG_RESOURCE_DIR "${CLANG_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION_MAJOR}")

add_subdirectory(test)
