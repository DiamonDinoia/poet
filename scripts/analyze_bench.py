#!/usr/bin/env python3
"""Aggregate benchmark JSON results into comparison tables and ASM analysis.

Usage:
    python3 analyze_bench.py [--results-root results]
                             [--output-md results/summary/bench_comparison.md]
                             [--output-csv results/summary/bench_comparison.csv]
                             [--asm-md results/summary/asm_analysis.md]
"""

import argparse
import csv
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

# Compiler sort order: GCC (12→15) then Clang (18→22)
COMPILER_ORDER = [
    "gcc-12",
    "gcc-13",
    "gcc-14",
    "gcc-15",
    "clang-18",
    "clang-19",
    "clang-20",
    "clang-21",
    "clang-22",
]

VARIANT_ORDER = ["default", "native"]


def compiler_sort_key(name: str) -> tuple[int, int]:
    """Sort key: (family_order, version)."""
    for i, c in enumerate(COMPILER_ORDER):
        if c == name:
            return (i, 0)
    # Unknown compilers go last
    return (100, 0)


def column_key(compiler: str, variant: str) -> str:
    return f"{compiler} {variant}"


def load_results(results_root: Path) -> dict[str, dict[str, dict]]:
    """Load all JSON results.

    Returns: {bench_name: {column_key: parsed_json}}
    """
    data: dict[str, dict[str, dict]] = defaultdict(dict)

    for json_file in sorted(results_root.rglob("*.json")):
        # Path: results/<compiler>/<variant>/<bench>.json
        parts = json_file.relative_to(results_root).parts
        if len(parts) < 3:
            continue
        compiler = parts[0]
        variant = parts[1]
        bench_name = json_file.stem

        try:
            parsed = json.loads(json_file.read_text())
        except (json.JSONDecodeError, OSError) as e:
            print(f"Warning: failed to load {json_file}: {e}", file=sys.stderr)
            continue

        col = column_key(compiler, variant)
        data[bench_name][col] = parsed

    return dict(data)


def build_comparison_table(
    data: dict[str, dict[str, dict]],
) -> tuple[list[str], list[dict]]:
    """Build a flat comparison table.

    Returns: (columns, rows) where each row is a dict with keys:
        bench, table_title, name, <column>_nsop, <column>_rel
    """
    # Collect all columns
    all_cols = set()
    for bench_data in data.values():
        all_cols.update(bench_data.keys())

    # Sort columns
    sorted_cols = sorted(
        all_cols,
        key=lambda c: (
            VARIANT_ORDER.index(c.split()[-1])
            if c.split()[-1] in VARIANT_ORDER
            else 99,
            compiler_sort_key(c.rsplit(" ", 1)[0]),
        ),
    )

    rows = []
    for bench_name in sorted(data.keys()):
        bench_data = data[bench_name]

        # Collect all tables across all compilers for this bench
        # Use the first available compiler's tables as the template
        table_templates = {}
        for col, parsed in bench_data.items():
            for table in parsed.get("tables", []):
                title = table.get("title", "")
                if title not in table_templates:
                    table_templates[title] = table

        for table_title, template in table_templates.items():
            for row_data in template.get("rows", []):
                # Find the name column
                name = None
                for key in row_data:
                    if isinstance(row_data[key], str):
                        name = row_data[key]
                        break
                if name is None:
                    continue

                flat_row = {
                    "bench": bench_name,
                    "table_title": table_title,
                    "name": name,
                }

                for col in sorted_cols:
                    parsed = bench_data.get(col)
                    if not parsed:
                        flat_row[f"{col}_nsop"] = ""
                        flat_row[f"{col}_rel"] = ""
                        continue

                    # Find matching table and row
                    found = False
                    for table in parsed.get("tables", []):
                        if table.get("title", "") == table_title:
                            for r in table.get("rows", []):
                                row_name = None
                                for key in r:
                                    if isinstance(r[key], str):
                                        row_name = r[key]
                                        break
                                if row_name == name:
                                    # Extract ns/op and relative
                                    nsop = r.get("ns/op", r.get("ns", ""))
                                    rel = r.get("relative", r.get("relative_pct", ""))
                                    flat_row[f"{col}_nsop"] = nsop
                                    flat_row[f"{col}_rel"] = rel
                                    found = True
                                    break
                            if found:
                                break

                    if not found:
                        flat_row[f"{col}_nsop"] = ""
                        flat_row[f"{col}_rel"] = ""

                rows.append(flat_row)

    return sorted_cols, rows


def write_markdown(columns: list[str], rows: list[dict], output: Path):
    """Write Markdown comparison table."""
    output.parent.mkdir(parents=True, exist_ok=True)

    with open(output, "w") as f:
        f.write("# Benchmark Comparison\n\n")
        f.write(f"*Generated from {len(columns)} compiler/variant combinations*\n\n")

        # Group rows by bench + table_title
        groups: dict[str, list[dict]] = defaultdict(list)
        for row in rows:
            key = f"{row['bench']} / {row['table_title']}"
            groups[key].append(row)

        for group_key, group_rows in groups.items():
            f.write(f"## {group_key}\n\n")

            # Header
            header = "| Benchmark |"
            sep = "|:----------|"
            for col in columns:
                header += f" {col} |"
                sep += "--------:|"
            f.write(header + "\n")
            f.write(sep + "\n")

            for row in group_rows:
                line = f"| {row['name']} |"
                for col in columns:
                    nsop = row.get(f"{col}_nsop", "")
                    rel = row.get(f"{col}_rel", "")
                    if nsop != "" and rel != "":
                        cell = f"{nsop} ({rel}%)"
                    elif nsop != "":
                        cell = str(nsop)
                    else:
                        cell = "-"
                    line += f" {cell} |"
                f.write(line + "\n")

            f.write("\n")


def write_csv(columns: list[str], rows: list[dict], output: Path):
    """Write flat CSV."""
    output.parent.mkdir(parents=True, exist_ok=True)

    fieldnames = ["bench", "table_title", "name"]
    for col in columns:
        fieldnames.append(f"{col}_nsop")
        fieldnames.append(f"{col}_rel")

    with open(output, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def analyze_asm(results_root: Path, output: Path):
    """Analyze assembly files for vectorization and inlining quality."""
    output.parent.mkdir(parents=True, exist_ok=True)

    asm_files = sorted(results_root.rglob("asm/*.asm"))
    if not asm_files:
        output.write_text("# ASM Analysis\n\nNo assembly files found.\n")
        return

    with open(output, "w") as f:
        f.write("# Assembly Analysis\n\n")

        for asm_file in asm_files:
            parts = asm_file.relative_to(results_root).parts
            if len(parts) < 4:
                continue
            compiler = parts[0]
            variant = parts[1]
            bench = asm_file.stem.replace("_hot", "")

            text = asm_file.read_text()

            # Count register usage
            ymm_count = len(re.findall(r"%ymm\d+", text))
            xmm_count = len(re.findall(r"%xmm\d+", text))
            zmm_count = len(re.findall(r"%zmm\d+", text))

            # Count call instructions (inlining quality)
            call_count = len(re.findall(r"\bcall\b", text, re.IGNORECASE))

            # Detect vectorization
            vectorized = ymm_count > 0 or zmm_count > 0
            vec_width = (
                "512-bit"
                if zmm_count > 0
                else (
                    "256-bit"
                    if ymm_count > 0
                    else ("128-bit" if xmm_count > 0 else "scalar")
                )
            )

            f.write(f"## {compiler} {variant} / {bench}\n\n")
            f.write(f"- Vector width: **{vec_width}**\n")
            f.write(
                f"- Register usage: zmm={zmm_count}, ymm={ymm_count}, xmm={xmm_count}\n"
            )
            f.write(f"- Call instructions: {call_count}\n")
            if not vectorized and (
                "saxpy" in bench.lower() or "compiler_comparison" in bench.lower()
            ):
                f.write("- **WARNING: saxpy probe may not be vectorized**\n")
            f.write("\n")

        # Summary table
        f.write("## Summary\n\n")
        f.write("| Compiler | Variant | Bench | Vec Width | Calls |\n")
        f.write("|:---------|:--------|:------|:----------|------:|\n")

        for asm_file in asm_files:
            parts = asm_file.relative_to(results_root).parts
            if len(parts) < 4:
                continue
            compiler = parts[0]
            variant = parts[1]
            bench = asm_file.stem.replace("_hot", "")

            text = asm_file.read_text()
            ymm = len(re.findall(r"%ymm\d+", text))
            xmm = len(re.findall(r"%xmm\d+", text))
            zmm = len(re.findall(r"%zmm\d+", text))
            calls = len(re.findall(r"\bcall\b", text, re.IGNORECASE))
            vec = (
                "512"
                if zmm > 0
                else ("256" if ymm > 0 else ("128" if xmm > 0 else "scalar"))
            )

            f.write(f"| {compiler} | {variant} | {bench} | {vec} | {calls} |\n")

        f.write("\n")


def main():
    parser = argparse.ArgumentParser(description="Aggregate benchmark results")
    parser.add_argument(
        "--results-root", default="results", help="Root directory of results"
    )
    parser.add_argument("--output-md", default="results/summary/bench_comparison.md")
    parser.add_argument("--output-csv", default="results/summary/bench_comparison.csv")
    parser.add_argument("--asm-md", default="results/summary/asm_analysis.md")
    args = parser.parse_args()

    results_root = Path(args.results_root)
    if not results_root.exists():
        print(f"Error: results root not found: {results_root}", file=sys.stderr)
        sys.exit(1)

    # Load and aggregate benchmark data
    data = load_results(results_root)
    if data:
        columns, rows = build_comparison_table(data)
        write_markdown(columns, rows, Path(args.output_md))
        write_csv(columns, rows, Path(args.output_csv))
        print(f"Wrote {args.output_md} ({len(rows)} rows)")
        print(f"Wrote {args.output_csv}")
    else:
        print("Warning: no benchmark JSON data found", file=sys.stderr)

    # Analyze assembly
    analyze_asm(results_root, Path(args.asm_md))
    print(f"Wrote {args.asm_md}")


if __name__ == "__main__":
    main()
