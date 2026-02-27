#!/usr/bin/env python3
"""Extract hot-path assembly from benchmark binaries.

Usage:
    python3 extract_asm.py BINARY OUTPUT.asm [--compiler clang-22] [--bench compiler_comparison_bench]

Selects objdump binary: llvm-objdump-N for clang-N, else objdump.
Filters to hot functions via configurable regex patterns per benchmark.
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

# Per-benchmark hot function patterns
BENCH_PATTERNS: dict[str, list[str]] = {
    "compiler_comparison_bench": [
        r"saxpy_",
        r"dispatch_if_else",
        r"dispatch_switch",
        r"dispatch_fnptr",
        r"dispatch_work",
        r"dispatch_stub",
        r"run_sweep",
        r"run_inline_test",
        r"hand_unrolled_multi_acc",
        r"heavy_work",
        r"InlineAccFunctor",
    ],
    "dispatch_bench": [
        r"_FUN\(",
        r"dispatch_table_builder",
        r"next_noise",
        r"simple_kernel",
    ],
    "static_for_bench": [
        r"MapFunctor",
        r"MultiAccFunctor",
        r"heavy_work",
    ],
    "dynamic_for_bench": [
        r"hand_unrolled_multi_acc",
        r"heavy_work",
        r"lambda.*#[123]",
        r"'lambda'",
        r"dynamic_for",
        r"execute_block",
        r"dispatch_tail",
    ],
}

# Max stubs to keep for dispatch bench
MAX_STUBS = 20


def find_objdump(compiler: str) -> str:
    """Find the appropriate objdump for the given compiler."""
    if compiler.startswith("clang-"):
        version = compiler.split("-", 1)[1]
        # Try llvm-objdump-N first
        candidate = f"llvm-objdump-{version}"
        if shutil.which(candidate):
            return candidate
        # Fall back to llvm-objdump
        if shutil.which("llvm-objdump"):
            return "llvm-objdump"
    # Default
    return "objdump"


def run_objdump(objdump_bin: str, binary: str) -> str:
    """Run objdump and return the disassembly text."""
    cmd = [objdump_bin, "-d", "--demangle", "-j", ".text", binary]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        if proc.returncode != 0:
            # Some objdump versions don't support -j .text; retry without
            cmd_fallback = [objdump_bin, "-d", "--demangle", binary]
            proc = subprocess.run(
                cmd_fallback, capture_output=True, text=True, timeout=120
            )
        return proc.stdout
    except FileNotFoundError:
        print(f"Warning: {objdump_bin} not found, trying objdump", file=sys.stderr)
        cmd = ["objdump", "-d", "--demangle", binary]
        return subprocess.run(cmd, capture_output=True, text=True, timeout=120).stdout


def split_functions(disasm: str) -> list[tuple[str, str]]:
    """Split disassembly into (function_name, body) tuples."""
    functions = []
    current_name = None
    current_lines: list[str] = []

    for line in disasm.splitlines():
        # Function label: address <name>:
        m = re.match(r"^[0-9a-f]+ <(.+)>:\s*$", line)
        if m:
            if current_name and current_lines:
                functions.append((current_name, "\n".join(current_lines)))
            current_name = m.group(1)
            current_lines = [line]
        elif current_name is not None:
            current_lines.append(line)

    if current_name and current_lines:
        functions.append((current_name, "\n".join(current_lines)))

    return functions


def filter_functions(
    functions: list[tuple[str, str]], patterns: list[str]
) -> list[tuple[str, str]]:
    """Filter functions matching any of the given regex patterns."""
    compiled = [re.compile(p, re.IGNORECASE) for p in patterns]
    matched = []
    stub_count = 0

    for name, body in functions:
        for pat in compiled:
            if pat.search(name):
                # Cap stub functions
                if "stub" in name.lower() or "_FUN" in name:
                    stub_count += 1
                    if stub_count > MAX_STUBS:
                        continue
                matched.append((name, body))
                break

    return matched


def main():
    parser = argparse.ArgumentParser(
        description="Extract hot-path assembly from benchmark binaries"
    )
    parser.add_argument("binary", help="Path to the benchmark binary")
    parser.add_argument("output", help="Output .asm file path")
    parser.add_argument(
        "--compiler", default="gcc", help="Compiler name (e.g., gcc-15, clang-22)"
    )
    parser.add_argument(
        "--bench", default="", help="Benchmark name for pattern selection"
    )
    args = parser.parse_args()

    binary_path = Path(args.binary)
    if not binary_path.exists():
        print(f"Error: binary not found: {binary_path}", file=sys.stderr)
        sys.exit(1)

    # Select patterns
    bench_name = args.bench
    if not bench_name:
        bench_name = binary_path.stem.replace("poet_", "").replace("_native", "")
    patterns = BENCH_PATTERNS.get(bench_name, [r".*"])

    # Find and run objdump
    objdump_bin = find_objdump(args.compiler)
    disasm = run_objdump(objdump_bin, str(binary_path))

    if not disasm:
        print(f"Warning: empty disassembly for {binary_path}", file=sys.stderr)
        sys.exit(1)

    # Split and filter
    functions = split_functions(disasm)
    hot_functions = filter_functions(functions, patterns)

    # Write output
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_path, "w") as f:
        f.write(f"; Hot-path assembly: {binary_path.name}\n")
        f.write(f"; Compiler: {args.compiler}\n")
        f.write(f"; Objdump: {objdump_bin}\n")
        f.write(f"; Patterns: {patterns}\n")
        f.write(f"; Functions matched: {len(hot_functions)} / {len(functions)}\n")
        f.write(f"; {'=' * 72}\n\n")

        for name, body in hot_functions:
            f.write(f"; --- {name} ---\n")
            f.write(body)
            f.write("\n\n")

    print(f"Extracted {len(hot_functions)} functions -> {output_path}")


if __name__ == "__main__":
    main()
