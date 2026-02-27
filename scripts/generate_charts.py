#!/usr/bin/env python3
"""Generate SVG benchmark charts from parsed JSON results.

Usage:
    python3 scripts/generate_charts.py --results-root results --output-dir docs/benchmarks

Reads results/<compiler>/default/<bench>.json and produces:
    docs/benchmarks/dynamic_for_speedup.svg
    docs/benchmarks/static_for_speedup.svg
    docs/benchmarks/dispatch_optimization.svg
    docs/benchmarks/cross_compiler_overview.svg
"""

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Styling ──────────────────────────────────────────────────────────────────

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

COMPILER_COLORS = {
    "gcc": "#4C72B0",
    "clang": "#DD8452",
}


def compiler_color(name: str) -> str:
    if name.startswith("gcc"):
        return COMPILER_COLORS["gcc"]
    return COMPILER_COLORS["clang"]


def compiler_sort_key(name: str) -> int:
    for i, c in enumerate(COMPILER_ORDER):
        if c == name:
            return i
    return 100


def style_chart(ax: plt.Axes, title: str):
    ax.set_title(title, fontsize=13, fontweight="bold", pad=12)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(axis="both", which="both", labelsize=9)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f"))


# ── Data loading ─────────────────────────────────────────────────────────────


def load_results(results_root: Path) -> dict[str, dict[str, dict]]:
    """Load all JSON results from results/<compiler>/default/<bench>.json.

    Returns: {bench_name: {compiler: parsed_json}}
    """
    data: dict[str, dict[str, dict]] = defaultdict(dict)

    for json_file in sorted(results_root.rglob("*.json")):
        parts = json_file.relative_to(results_root).parts
        if len(parts) < 3:
            continue
        compiler = parts[0]
        variant = parts[1]
        if variant != "default":
            continue
        bench_name = json_file.stem

        try:
            parsed = json.loads(json_file.read_text())
        except (json.JSONDecodeError, OSError) as e:
            print(f"Warning: failed to load {json_file}: {e}", file=sys.stderr)
            continue

        data[bench_name][compiler] = parsed

    return dict(data)


def find_rows(parsed: dict, table_title_pattern: str) -> list[dict]:
    """Find all rows in tables matching a title pattern."""
    rows = []
    for table in parsed.get("tables", []):
        title = table.get("title", "")
        if re.search(table_title_pattern, title, re.IGNORECASE):
            rows.extend(table.get("rows", []))
    return rows


def row_name(row: dict) -> str:
    """Extract the string name column from a row."""
    for v in row.values():
        if isinstance(v, str):
            return v
    return ""


def row_nsop(row: dict) -> float | None:
    """Extract ns/op from a row."""
    for key in ("ns/op", "ns"):
        if key in row and isinstance(row[key], (int, float)):
            return float(row[key])
    return None


def row_relative(row: dict) -> float | None:
    """Extract relative percentage from a row."""
    for key in ("relative", "relative_pct"):
        if key in row and isinstance(row[key], (int, float)):
            return float(row[key])
    return None


# ── Chart generators ─────────────────────────────────────────────────────────


def generate_dynamic_for_chart(data: dict[str, dict], output: Path):
    """dynamic_for speedup: grouped bars per compiler."""
    bench_data = data.get("dynamic_for_bench")
    if not bench_data:
        print("Warning: no dynamic_for_bench data found", file=sys.stderr)
        return

    compilers = sorted(bench_data.keys(), key=compiler_sort_key)
    if not compilers:
        return

    # We look for the "Multi-acc" table, extracting the three methods
    method_patterns = [
        ("for loop (1 acc)", r"for loop \(1 acc\)"),
        ("hand-unrolled", r"for loop \(optimal"),
        ("dynamic_for", r"dynamic_for"),
    ]

    fig, ax = plt.subplots(figsize=(max(8, len(compilers) * 2.5), 5))

    import numpy as np

    x = np.arange(len(compilers))
    width = 0.25
    method_colors = ["#AAAAAA", "#7FBBDA", "#4C72B0"]

    for mi, (label, pattern) in enumerate(method_patterns):
        values = []
        for compiler in compilers:
            parsed = bench_data[compiler]
            rows = find_rows(parsed, r"Multi-acc.*lane")
            nsop = None
            for r in rows:
                if re.search(pattern, row_name(r)):
                    nsop = row_nsop(r)
                    break
            values.append(nsop if nsop is not None else 0)

        # Normalize to baseline (first method)
        if mi == 0:
            baseline = [v if v > 0 else 1 for v in values]

        speedups = [baseline[i] / v if v > 0 else 0 for i, v in enumerate(values)]
        bars = ax.bar(
            x + mi * width, speedups, width, label=label, color=method_colors[mi]
        )
        for bar, s in zip(bars, speedups):
            if s > 0:
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    bar.get_height() + 0.02,
                    f"{s:.2f}x",
                    ha="center",
                    va="bottom",
                    fontsize=7,
                )

    ax.set_xticks(x + width)
    ax.set_xticklabels(compilers, rotation=30, ha="right")
    ax.set_ylabel("Speedup vs for loop (1 acc)")
    ax.legend(loc="upper left", framealpha=0.9, fontsize=9)
    style_chart(ax, "dynamic_for: multi-accumulator speedup")
    ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)

    fig.tight_layout()
    fig.savefig(str(output), format="svg", bbox_inches="tight")
    plt.close(fig)
    print(f"  Wrote {output}")


def generate_static_for_chart(data: dict[str, dict], output: Path):
    """static_for speedup: two subplots (Map + Multi-acc)."""
    bench_data = data.get("static_for_bench")
    if not bench_data:
        print("Warning: no static_for_bench data found", file=sys.stderr)
        return

    compilers = sorted(bench_data.keys(), key=compiler_sort_key)
    if not compilers:
        return

    sections = [
        (
            "Map",
            r"Map.*static_for",
            [
                ("for loop", r"^for loop$"),
                ("static_for (tuned BS)", r"tuned BS"),
                ("static_for (default BS)", r"default BS"),
            ],
        ),
        (
            "Multi-acc",
            r"Multi-acc.*static_for",
            [
                ("for loop", r"^for loop$"),
                ("static_for (tuned BS)", r"tuned BS"),
                ("static_for (default BS)", r"default BS"),
            ],
        ),
    ]

    fig, axes = plt.subplots(1, 2, figsize=(max(12, len(compilers) * 2.5), 5))
    method_colors = ["#AAAAAA", "#4C72B0", "#7FBBDA"]

    import numpy as np

    for si, (section_title, table_pat, methods) in enumerate(sections):
        ax = axes[si]
        x = np.arange(len(compilers))
        width = 0.25
        baseline_vals = None

        for mi, (label, pattern) in enumerate(methods):
            values = []
            for compiler in compilers:
                parsed = bench_data[compiler]
                rows = find_rows(parsed, table_pat)
                nsop = None
                for r in rows:
                    if re.search(pattern, row_name(r)):
                        nsop = row_nsop(r)
                        break
                values.append(nsop if nsop is not None else 0)

            if mi == 0:
                baseline_vals = [v if v > 0 else 1 for v in values]

            speedups = [
                baseline_vals[i] / v if v > 0 else 0 for i, v in enumerate(values)
            ]
            bars = ax.bar(
                x + mi * width, speedups, width, label=label, color=method_colors[mi]
            )
            for bar, s in zip(bars, speedups):
                if s > 0:
                    ax.text(
                        bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + 0.02,
                        f"{s:.2f}x",
                        ha="center",
                        va="bottom",
                        fontsize=7,
                    )

        ax.set_xticks(x + width)
        ax.set_xticklabels(compilers, rotation=30, ha="right")
        ax.set_ylabel("Speedup vs for loop")
        ax.legend(loc="upper left", framealpha=0.9, fontsize=8)
        style_chart(ax, f"static_for: {section_title}")
        ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)

    fig.tight_layout()
    fig.savefig(str(output), format="svg", bbox_inches="tight")
    plt.close(fig)
    print(f"  Wrote {output}")


def generate_dispatch_optimization_chart(data: dict[str, dict], output: Path):
    """Dispatch optimization: runtime vs dispatched for each N, per compiler."""
    bench_data = data.get("dispatch_optimization_bench")
    if not bench_data:
        print("Warning: no dispatch_optimization_bench data found", file=sys.stderr)
        return

    compilers = sorted(bench_data.keys(), key=compiler_sort_key)
    if not compilers:
        return

    n_values = [4, 8, 16, 32]

    fig, ax = plt.subplots(figsize=(max(8, len(compilers) * len(n_values) * 0.8), 5))

    import numpy as np

    group_labels = []
    for compiler in compilers:
        for n in n_values:
            group_labels.append(f"{compiler}\nN={n}")

    x = np.arange(len(group_labels))
    width = 0.35

    runtime_vals = []
    dispatched_vals = []

    for compiler in compilers:
        parsed = bench_data[compiler]
        rows = find_rows(parsed, r"")
        for n in n_values:
            rt_nsop = None
            disp_nsop = None
            for r in rows:
                name = row_name(r)
                if f"N={n}" in name and "runtime" in name:
                    rt_nsop = row_nsop(r)
                elif f"N={n}" in name and "dispatched" in name:
                    disp_nsop = row_nsop(r)
            runtime_vals.append(rt_nsop if rt_nsop is not None else 0)
            dispatched_vals.append(disp_nsop if disp_nsop is not None else 0)

    ax.bar(x - width / 2, runtime_vals, width, label="runtime N", color="#AAAAAA")
    ax.bar(x + width / 2, dispatched_vals, width, label="dispatched N", color="#4C72B0")

    # Add speedup annotations
    for i in range(len(x)):
        if runtime_vals[i] > 0 and dispatched_vals[i] > 0:
            speedup = runtime_vals[i] / dispatched_vals[i]
            ax.text(
                x[i] + width / 2,
                dispatched_vals[i] + 0.1,
                f"{speedup:.1f}x",
                ha="center",
                va="bottom",
                fontsize=7,
                color="#2a5a8a",
                fontweight="bold",
            )

    ax.set_xticks(x)
    ax.set_xticklabels(group_labels, rotation=45, ha="right", fontsize=7)
    ax.set_ylabel("ns/op")
    ax.legend(loc="upper left", framealpha=0.9, fontsize=9)
    style_chart(
        ax, "Compile-time specialization: runtime N vs dispatched N (Horner polynomial)"
    )

    fig.tight_layout()
    fig.savefig(str(output), format="svg", bbox_inches="tight")
    plt.close(fig)
    print(f"  Wrote {output}")


def generate_cross_compiler_chart(data: dict[str, dict], output: Path):
    """Cross-compiler overview: speedup of POET vs baseline across all benches."""
    # Collect speedup ratios for each compiler from each benchmark
    # We define "POET" as the best POET method and "baseline" as the plain for loop

    bench_configs = {
        "dynamic_for_bench": {
            "table_pattern": r"Multi-acc.*lane",
            "baseline_pattern": r"for loop \(1 acc\)",
            "poet_pattern": r"dynamic_for",
            "label": "dynamic_for",
        },
        "static_for_bench": {
            "table_pattern": r"Multi-acc.*static_for",
            "baseline_pattern": r"^for loop$",
            "poet_pattern": r"tuned BS",
            "label": "static_for",
        },
        "dispatch_optimization_bench": {
            "table_pattern": r"",
            "baseline_pattern": r"N=16.*runtime",
            "poet_pattern": r"N=16.*dispatched",
            "label": "dispatch (N=16)",
        },
    }

    all_compilers: set[str] = set()
    for bench_name in bench_configs:
        if bench_name in data:
            all_compilers.update(data[bench_name].keys())

    compilers = sorted(all_compilers, key=compiler_sort_key)
    if not compilers:
        print("Warning: no data for cross-compiler chart", file=sys.stderr)
        return

    import numpy as np

    bench_labels = []
    speedup_matrix = []  # list of lists: [bench][compiler]

    for bench_name, cfg in bench_configs.items():
        if bench_name not in data:
            continue
        bench_data = data[bench_name]
        bench_labels.append(cfg["label"])
        row_speedups = []
        for compiler in compilers:
            if compiler not in bench_data:
                row_speedups.append(0)
                continue
            parsed = bench_data[compiler]
            rows = find_rows(parsed, cfg["table_pattern"])
            baseline_nsop = None
            poet_nsop = None
            for r in rows:
                name = row_name(r)
                if re.search(cfg["baseline_pattern"], name):
                    baseline_nsop = row_nsop(r)
                if re.search(cfg["poet_pattern"], name):
                    poet_nsop = row_nsop(r)
            if baseline_nsop and poet_nsop and poet_nsop > 0:
                row_speedups.append(baseline_nsop / poet_nsop)
            else:
                row_speedups.append(0)
        speedup_matrix.append(row_speedups)

    if not bench_labels:
        return

    fig, ax = plt.subplots(figsize=(max(8, len(compilers) * 2), 5))
    x = np.arange(len(compilers))
    n_benches = len(bench_labels)
    width = 0.7 / n_benches
    colors = ["#4C72B0", "#DD8452", "#55A868", "#C44E52"]

    for bi, label in enumerate(bench_labels):
        offset = (bi - n_benches / 2 + 0.5) * width
        vals = speedup_matrix[bi]
        bars = ax.bar(
            x + offset, vals, width, label=label, color=colors[bi % len(colors)]
        )
        for bar, v in zip(bars, vals):
            if v > 0:
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    bar.get_height() + 0.02,
                    f"{v:.2f}x",
                    ha="center",
                    va="bottom",
                    fontsize=7,
                )

    ax.set_xticks(x)
    ax.set_xticklabels(compilers, rotation=30, ha="right")
    ax.set_ylabel("Speedup vs baseline")
    ax.legend(loc="upper left", framealpha=0.9, fontsize=9)
    style_chart(ax, "Cross-compiler: POET speedup over baseline")
    ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)

    fig.tight_layout()
    fig.savefig(str(output), format="svg", bbox_inches="tight")
    plt.close(fig)
    print(f"  Wrote {output}")


# ── Main ─────────────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="Generate SVG benchmark charts")
    parser.add_argument(
        "--results-root",
        default="results",
        help="Root directory of results (default: results)",
    )
    parser.add_argument(
        "--output-dir",
        default="docs/benchmarks",
        help="Output directory for SVG charts (default: docs/benchmarks)",
    )
    args = parser.parse_args()

    results_root = Path(args.results_root)
    output_dir = Path(args.output_dir)

    if not results_root.exists():
        print(f"Error: results root not found: {results_root}", file=sys.stderr)
        sys.exit(1)

    output_dir.mkdir(parents=True, exist_ok=True)

    print("Loading benchmark results...")
    data = load_results(results_root)
    if not data:
        print("Error: no benchmark JSON data found", file=sys.stderr)
        sys.exit(1)

    print(f"Found benchmarks: {', '.join(sorted(data.keys()))}")
    compilers_found: set[str] = set()
    for bench_data in data.values():
        compilers_found.update(bench_data.keys())
    print(
        f"Found compilers: {', '.join(sorted(compilers_found, key=compiler_sort_key))}"
    )

    print("\nGenerating charts...")
    generate_dynamic_for_chart(data, output_dir / "dynamic_for_speedup.svg")
    generate_static_for_chart(data, output_dir / "static_for_speedup.svg")
    generate_dispatch_optimization_chart(data, output_dir / "dispatch_optimization.svg")
    generate_cross_compiler_chart(data, output_dir / "cross_compiler_overview.svg")
    print("\nDone.")


if __name__ == "__main__":
    main()
