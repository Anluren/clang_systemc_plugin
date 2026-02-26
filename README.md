# SystemC sc_int/sc_uint Builtin Assignment Checker

A Clang plugin that detects assignments of C++ builtin-type variables (`int`, `long`, `double`, etc.) to SystemC `sc_int<W>`/`sc_uint<W>` types, which can silently lose precision or change signedness.

## Example

Given this code:

```cpp
#include <systemc.h>

void example() {
    sc_int<8> x;
    int y = 42;
    x = y;    // warning: assignment of builtin-type variable 'y' (type 'int')
              //          to sc_int/sc_uint type 'sc_int<8>' is not allowed
}
```

The plugin catches:
- Simple assignments (`x = y`)
- Compound assignments (`x += y`, `x &= y`, etc.)
- All builtin types: `int`, `unsigned int`, `long`, `short`, `double`, `bool`, etc.

It does **not** flag:
- Literal assignments (`x = 42`)
- SystemC-to-SystemC assignments (`x = other_sc_int`)

## Prerequisites

- **Clang/LLVM 17** with development headers
- **SystemC** (tested with 3.0.2)
- **CMake** >= 3.16

Set environment variables:

```bash
export Clang_DIR=/path/to/llvm-17/lib/cmake/clang
export SYSTEMC_HOME=/path/to/systemc/install
```

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

This produces `libScIntAssignChecker.so`.

## Usage

Load the plugin into `clang++` during normal compilation:

```bash
clang++ -Xclang -load -Xclang /path/to/libScIntAssignChecker.so \
        -Xclang -add-plugin -Xclang sc-int-assign-checker \
        -I$SYSTEMC_HOME/include -std=c++17 your_file.cpp
```

Use `-fsyntax-only` to check without generating object files:

```bash
clang++ -fsyntax-only \
        -Xclang -load -Xclang /path/to/libScIntAssignChecker.so \
        -Xclang -add-plugin -Xclang sc-int-assign-checker \
        -I$SYSTEMC_HOME/include -std=c++17 your_file.cpp
```

## Running Tests

```bash
# From the build directory
bash ../test/run_tests.sh \
    /path/to/clang++ \
    ./libScIntAssignChecker.so \
    $SYSTEMC_HOME/include
```

Or via CMake targets:

```bash
make check_violations      # expects 8 warnings
make check_no_violations   # expects 0 warnings
```

## How It Works

The plugin uses a `RecursiveASTVisitor` to walk the AST and inspect every `CXXOperatorCallExpr`. For each assignment operator, it checks:

1. The callee method belongs to a class derived from `sc_dt::sc_int_base` or `sc_dt::sc_uint_base`
2. The RHS operand (after stripping implicit casts) is a `DeclRefExpr` referring to a variable with a builtin type

Matches in system headers are skipped to avoid flagging SystemC's own internals.
