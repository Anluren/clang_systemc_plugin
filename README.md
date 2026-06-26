# SystemC Clang Plugins

This repository now contains two plugins in the same shared object:

1. `sc-int-assign-checker`: Detects assignments of C++ builtin-type variables (`int`, `long`, `double`, etc.) to SystemC `sc_int<W>`/`sc_uint<W>` types, which can silently lose precision or change signedness.
2. `sc-dt-type-annotator`: Recognizes declarations that use SystemC `sc_dt` types and adds an implicit AST annotation attribute `sc_dt::<type>` to the declaration node.

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

The `sc-int-assign-checker` plugin catches:
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

Load the builtin assignment checker plugin into `clang++` during normal compilation:

```bash
clang++ -Xclang -load -Xclang /path/to/libScIntAssignChecker.so \
        -Xclang -add-plugin -Xclang sc-int-assign-checker \
        -I$SYSTEMC_HOME/include -std=c++17 your_file.cpp
```

Load the `sc_dt` type annotator plugin:

```bash
clang++ -Xclang -load -Xclang /path/to/libScIntAssignChecker.so \
        -Xclang -add-plugin -Xclang sc-dt-type-annotator \
        -I$SYSTEMC_HOME/include -std=c++17 your_file.cpp
```

To inspect annotations in the AST:

```bash
clang++ -fsyntax-only -Xclang -ast-dump \
        -Xclang -load -Xclang /path/to/libScIntAssignChecker.so \
        -Xclang -add-plugin -Xclang sc-dt-type-annotator \
        -I$SYSTEMC_HOME/include -std=c++17 your_file.cpp
```

Look for lines like:

```text
AnnotateAttr ... "sc_dt::sc_int"
AnnotateAttr ... "sc_dt::sc_uint"
AnnotateAttr ... "sc_dt::sc_biguint"
```

You can also add explicit scoped attributes in source code and the plugin maps
them to the same internal annotation metadata:

```cpp
[[sc_dt::sc_int]] int tagged_a;
[[sc_dt::sc_biguint]] int tagged_b;
```

Supported explicit spellings include:
`sc_int`, `sc_uint`, `sc_bigint`, `sc_biguint`, `sc_bv`, `sc_lv`,
`sc_fixed`, and `sc_ufixed` (all under `sc_dt::`).

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
make check_sc_dt_annotations  # expects sc_dt::<type> annotations in AST dump
make check_lit             # runs lit/FileCheck tests under test/lit/
```

## lit/FileCheck Tests

The repository includes lit tests in `test/lit/`:

- `sc_int_assign_warning.cpp`: checks warning text from `sc-int-assign-checker`
- `sc_dt_annotations.cpp`: checks AST `AnnotateAttr` output from `sc-dt-type-annotator`

Run from the build directory:

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build . --target check_lit
```

Or run lit directly using the generated site config:

```bash
lit -sv build/test/lit
```

## How It Works

The plugin uses a `RecursiveASTVisitor` to walk the AST and inspect every `CXXOperatorCallExpr`. For each assignment operator, it checks:

1. The callee method belongs to a class derived from `sc_dt::sc_int_base` or `sc_dt::sc_uint_base`
2. The RHS operand (after stripping implicit casts) is a `DeclRefExpr` referring to a variable with a builtin type

Matches in system headers are skipped to avoid flagging SystemC's own internals.
