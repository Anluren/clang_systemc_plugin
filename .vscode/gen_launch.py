#!/usr/bin/env python3
"""gen_launch.py - Generate .vscode/launch.json from test_compile_commands.json.

Reads build/test_compile_commands.json (emitted by cmake via file(GENERATE)) and
writes .vscode/launch.json with a cppdbg/GDB configuration for each test file.

The compile_commands 'command' field is shell-split; the first token becomes
'program' (the clang++ binary) and the remainder become 'args'.

Usage:
    python3 .vscode/gen_launch.py [path/to/test_compile_commands.json]

Run this after every cmake reconfigure to keep launch.json in sync.

NOTE: Build the plugin with debug symbols for useful backtraces:
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    cmake --build .
"""

import json
import os
import shlex
import sys

WORKSPACE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_COMPDB = os.path.join(WORKSPACE, "build", "test_compile_commands.json")
LAUNCH_JSON = os.path.join(WORKSPACE, ".vscode", "launch.json")


def load_compdb(path: str) -> list:
    with open(path) as f:
        return json.load(f)


def parse_entry(entry: dict) -> tuple[str, list[str]]:
    """Return (compiler_path, args_list) from a compile_commands entry.

    Handles both 'command' (shell string) and 'arguments' (list) formats.
    Strips -o/-c flags that are irrelevant to the plugin invocation.
    """
    if "arguments" in entry:
        parts = list(entry["arguments"])
    else:
        parts = shlex.split(entry["command"])

    compiler = parts[0]
    args: list[str] = []
    i = 1
    while i < len(parts):
        arg = parts[i]
        if arg == "-o":       # -o <file>: skip pair
            i += 2
            continue
        if arg.startswith("-o") and len(arg) > 2:   # -o<file>: skip single
            i += 1
            continue
        if arg == "-c":       # explicit compile-only flag
            i += 1
            continue
        args.append(arg)
        i += 1
    return compiler, args


def make_config(entry: dict) -> dict:
    compiler, args = parse_entry(entry)
    source_file = entry["file"]
    return {
        "name": f"Debug plugin: {os.path.basename(source_file)}",
        "type": "cppdbg",
        "request": "launch",
        "program": compiler,
        "args": args,
        "stopAtEntry": False,
        "cwd": entry["directory"],
        "environment": [],
        "externalConsole": False,
        "MIMode": "gdb",
        "setupCommands": [
            {
                "description": "Enable pretty-printing for gdb",
                "text": "-enable-pretty-printing",
                "ignoreFailures": True,
            }
        ],
        "preLaunchTask": "Build plugin",
    }


def main() -> None:
    compdb_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_COMPDB

    if not os.path.exists(compdb_path):
        sys.exit(
            f"Error: {compdb_path} not found.\n"
            "Run 'cmake ..' in your build directory first, then retry."
        )

    entries = load_compdb(compdb_path)
    configs = [make_config(e) for e in entries]
    launch = {"version": "0.2.0", "configurations": configs}

    os.makedirs(os.path.dirname(LAUNCH_JSON), exist_ok=True)
    with open(LAUNCH_JSON, "w") as f:
        json.dump(launch, f, indent=4)
        f.write("\n")

    print(f"Wrote {len(configs)} debug configuration(s) to {LAUNCH_JSON}:")
    for c in configs:
        print(f"  - {c['name']}")


if __name__ == "__main__":
    main()
