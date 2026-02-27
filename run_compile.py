#!/usr/bin/env python3
"""run_compile.py - Run compiler commands from a compilation database.

Reads a compile_commands.json (or any compilation database in that format),
filters entries by an optional fnmatch pattern on the source file path, and
executes each matching compiler command.

Usage:
    python3 run_compile.py [--db PATH] [--dry-run] [-v] [PATTERN] [EXTRA...]

Arguments:
    PATTERN     fnmatch pattern matched against the basename or full path of
                each entry's 'file' field (e.g. 'test_*.cpp', '*violations*').
                Omit to run all entries in the database.
    EXTRA       Extra flags appended to every compiler invocation, separated
                from script options by '--' (e.g. -- -Wextra -DDEBUG).

Options:
    --db PATH   Compilation database to read (default: compile_commands.json)
    --dry-run   Print commands without executing them
    -v          Show each command before running it

Examples:
    # Run every entry in the default compile_commands.json
    python3 run_compile.py

    # Run only entries whose filename matches test_*.cpp
    python3 run_compile.py 'test_*.cpp'

    # Use a custom database and filter by pattern
    python3 run_compile.py --db build/test_compile_commands.json '*violations*'

    # Preview what would be run without executing
    python3 run_compile.py --db build/test_compile_commands.json --dry-run

    # Append extra compiler flags to every invocation
    python3 run_compile.py --db build/test_compile_commands.json -- -Wextra -Werror

    # Combine pattern and extra flags
    python3 run_compile.py --db build/test_compile_commands.json '*basic*' -- -Wextra
"""

import argparse
import fnmatch
import json
import os
import shlex
import subprocess
import sys


def load_db(path: str) -> list:
    """Load and return all entries from a compile_commands.json file."""
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def matches(entry: dict, pattern: str | None) -> bool:
    """Return True if entry's file matches pattern (or no pattern given)."""
    if pattern is None:
        return True
    file_path = entry["file"]
    return (
        fnmatch.fnmatch(os.path.basename(file_path), pattern)
        or fnmatch.fnmatch(file_path, pattern)
    )


def command_argv(entry: dict) -> list[str]:
    """Return the compiler invocation as a list of strings."""
    if "arguments" in entry:
        return list(entry["arguments"])
    return shlex.split(entry["command"])


def run_entry(
    entry: dict,
    extra_flags: list[str],
    *,
    dry_run: bool,
    verbose: bool,
) -> bool:
    """Run (or preview) the compiler command for one database entry.

    extra_flags are appended to the command from the database before running.
    Returns True on success (exit code 0) or when dry_run is True.
    """
    argv = command_argv(entry) + extra_flags
    cwd = entry["directory"]

    if dry_run or verbose:
        quoted = " ".join(shlex.quote(a) for a in argv)
        print(f"  cwd : {cwd}")
        print(f"  cmd : {quoted}")

    if dry_run:
        return True

    result = subprocess.run(argv, cwd=cwd, check=False)  # returncode checked below
    return result.returncode == 0


def main() -> None:
    """Parse arguments, filter the compilation database, and run each entry."""
    parser = argparse.ArgumentParser(
        description="Run compiler commands from a compilation database.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("Examples:")[1].strip(),
    )
    parser.add_argument(
        "pattern",
        nargs="?",
        default=None,
        metavar="PATTERN",
        help="fnmatch pattern to filter source files (default: match all)",
    )
    parser.add_argument(
        "--db",
        default="compile_commands.json",
        metavar="PATH",
        help="path to compilation database (default: %(default)s)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print commands without executing them",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="print each command before running it",
    )
    args, extra_flags = parser.parse_known_args()

    # '--' is required before any extra compiler flags.  argparse consumes '--'
    # internally (it never appears in extra_flags), so check sys.argv directly.
    if extra_flags and "--" not in sys.argv[1:]:
        quoted = " ".join(shlex.quote(f) for f in extra_flags)
        parser.error(f"pass extra compiler flags after '--': {quoted}")
    extra_flags = [f for f in extra_flags if f != "--"]  # drop stray '--'

    if not os.path.exists(args.db):
        sys.exit(f"error: database not found: {args.db}")

    all_entries = load_db(args.db)
    entries = [e for e in all_entries if matches(e, args.pattern)]

    if not entries:
        label = repr(args.pattern) if args.pattern else "(all)"
        sys.exit(f"error: no entries match pattern {label} in {args.db}")

    extra_label = f"  extra flags: {' '.join(extra_flags)}\n" if extra_flags else ""
    print(f"Running {len(entries)} file(s) from {args.db}\n{extra_label}")

    passed = 0
    failed = 0

    for entry in entries:
        label = os.path.basename(entry["file"])
        print(f"==> {label}")
        ok = run_entry(entry, extra_flags, dry_run=args.dry_run, verbose=args.verbose)
        if ok:
            print("PASS\n")
            passed += 1
        else:
            print("FAIL\n")
            failed += 1

    summary = f"{passed} passed, {failed} failed"
    print("=" * len(summary))
    print(summary)
    sys.exit(failed)


if __name__ == "__main__":
    main()
