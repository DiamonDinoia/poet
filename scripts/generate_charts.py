#!/usr/bin/env python3
"""Generate SVG benchmark charts from Google Benchmark JSON results.

Usage:
    python3 scripts/generate_charts.py --results-root results --output-dir docs/benchmarks

Reads results/<compiler>/default/<bench>.json (Google Benchmark JSON) and produces:
    docs/benchmarks/dynamic_for_speedup.svg
    docs/benchmarks/static_for_speedup.svg
    docs/benchmarks/dispatch_optimization.svg
    docs/benchmarks/cross_compiler_overview.svg
"""

import argparse
import json
import math
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


def load_results(results_root: Path) -> dict[str, dict[str, list[dict]]]:
    """Load all Google Benchmark JSON results from results/<compiler>/default/<bench>.json.

    Returns: {bench_name: {compiler: [benchmark_entries]}}
    """
    data: dict[str, dict[str, list[dict]]] = defaultdict(dict)

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

        benchmarks = parsed.get("benchmarks", [])
        if benchmarks:
            data[bench_name][compiler] = benchmarks

    return dict(data)


def clean_name(name: str) -> str:
    """Strip Google Benchmark suffixes like '/min_time:0.100'."""
    return re.sub(r"/min_time:[0-9.]+$", "", name)


def find_entry(entries: list[dict], pattern: str) -> dict | None:
    """Find first benchmark entry whose name matches pattern."""
    for entry in entries:
        if re.search(pattern, clean_name(entry.get("name", ""))):
            return entry
    return None


def entry_nsop(entry: dict | None) -> float | None:
    """Extract cpu_time (ns) from a benchmark entry."""
    if entry is None:
        return None
    cpu_time = entry.get("cpu_time")
    if isinstance(cpu_time, (int, float)):
        return float(cpu_time)
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

    method_patterns = [
        ("for loop (1 acc)", r"Multi-acc/for_loop_1_acc"),
        ("hand-unrolled", r"Multi-acc/for_loop_optimal_accs"),
        ("dynamic_for (1 acc)", r"Multi-acc/dynamic_for_1_acc"),
        ("dynamic_for (optimal)", r"Multi-acc/dynamic_for_optimal_accs"),
    ]

    fig, ax = plt.subplots(figsize=(max(8, len(compilers) * 2.5), 5))

    import numpy as np

    x = np.arange(len(compilers))
    width = 0.19
    method_colors = ["#AAAAAA", "#7FBBDA", "#8FBC8F", "#4C72B0"]

    baseline = [1.0] * len(compilers)
    for mi, (label, pattern) in enumerate(method_patterns):
        values = []
        for compiler in compilers:
            entries = bench_data[compiler]
            entry = find_entry(entries, pattern)
            nsop = entry_nsop(entry)
            values.append(nsop if nsop is not None else 0)

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

    ax.set_xticks(x + width * 1.5)
    ax.set_xticklabels(compilers, rotation=30, ha="right")
    ax.set_ylabel("Speedup vs for loop (1 acc)")
    style_chart(ax, "dynamic_for: multi-accumulator speedup")
    ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)
    ax.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.15),
        ncol=4,
        framealpha=0.9,
        fontsize=9,
    )

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
            [
                ("for loop", r"Map/for_loop"),
                ("static_for (tuned BS)", r"Map/static_for_tuned_BS"),
                ("static_for (default BS)", r"Map/static_for_default_BS"),
            ],
        ),
        (
            "Multi-acc",
            [
                ("for loop", r"MultiAcc/for_loop"),
                ("static_for (tuned BS)", r"MultiAcc/static_for_tuned_BS"),
                ("static_for (default BS)", r"MultiAcc/static_for_default_BS"),
            ],
        ),
    ]

    fig, axes = plt.subplots(1, 2, figsize=(max(12, len(compilers) * 2.5), 5))
    method_colors = ["#AAAAAA", "#4C72B0", "#7FBBDA"]

    import numpy as np

    for si, (section_title, methods) in enumerate(sections):
        ax = axes[si]
        x = np.arange(len(compilers))
        width = 0.25
        baseline_vals = None

        for mi, (label, pattern) in enumerate(methods):
            values = []
            for compiler in compilers:
                entries = bench_data[compiler]
                entry = find_entry(entries, pattern)
                nsop = entry_nsop(entry)
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
        style_chart(ax, f"static_for: {section_title}")
        ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)
        ax.legend(
            loc="upper center",
            bbox_to_anchor=(0.5, -0.15),
            ncol=3,
            framealpha=0.9,
            fontsize=8,
        )

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
        entries = bench_data[compiler]
        for n in n_values:
            rt = find_entry(entries, rf"Horner/N={n}_runtime")
            disp = find_entry(entries, rf"Horner/N={n}_dispatched")
            runtime_vals.append(entry_nsop(rt) or 0)
            dispatched_vals.append(entry_nsop(disp) or 0)

    speedups = []
    for rt, disp in zip(runtime_vals, dispatched_vals):
        if rt > 0 and disp > 0:
            speedups.append(rt / disp)
        else:
            speedups.append(0)

    bars = ax.bar(x, speedups, width * 1.5, color="#4C72B0")
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

    ax.set_xticks(x)
    ax.set_xticklabels(group_labels, rotation=45, ha="right", fontsize=7)
    ax.set_ylabel("Speedup (dispatched vs runtime)")
    ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)
    style_chart(
        ax, "Compile-time specialization: dispatched N speedup (Horner polynomial)"
    )

    fig.tight_layout()
    fig.savefig(str(output), format="svg", bbox_inches="tight")
    plt.close(fig)
    print(f"  Wrote {output}")


def generate_cross_compiler_chart(data: dict[str, dict], output: Path):
    """Cross-compiler overview: speedup of POET vs baseline across all benches."""
    bench_configs = {
        "dynamic_for_bench": {
            "baseline_pattern": r"Multi-acc/for_loop_1_acc",
            "poet_pattern": r"Multi-acc/dynamic_for_optimal_accs",
            "label": "dynamic_for",
        },
        "static_for_bench": {
            "baseline_pattern": r"MultiAcc/for_loop",
            "poet_pattern": r"MultiAcc/static_for_tuned_BS",
            "label": "static_for",
        },
        "dispatch_optimization_bench": {
            "baseline_pattern": r"Horner/N=16_runtime",
            "poet_pattern": r"Horner/N=16_dispatched",
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
    speedup_matrix = []

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
            entries = bench_data[compiler]
            baseline = find_entry(entries, cfg["baseline_pattern"])
            poet = find_entry(entries, cfg["poet_pattern"])
            b_nsop = entry_nsop(baseline)
            p_nsop = entry_nsop(poet)
            if b_nsop and p_nsop and p_nsop > 0:
                row_speedups.append(b_nsop / p_nsop)
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
    style_chart(ax, "Cross-compiler: POET speedup over baseline")
    ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)
    ax.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.15),
        ncol=3,
        framealpha=0.9,
        fontsize=9,
    )

    fig.tight_layout()
    fig.savefig(str(output), format="svg", bbox_inches="tight")
    plt.close(fig)
    print(f"  Wrote {output}")


def generate_average_improvement_chart(data: dict[str, dict], output: Path):
    """Average improvement: geometric mean speedup per compiler across all benchmarks."""
    bench_configs = {
        "dynamic_for_bench": {
            "baseline_pattern": r"Multi-acc/for_loop_1_acc",
            "poet_pattern": r"Multi-acc/dynamic_for_optimal_accs",
        },
        "static_for_bench": {
            "baseline_pattern": r"MultiAcc/for_loop",
            "poet_pattern": r"MultiAcc/static_for_tuned_BS",
        },
        "dispatch_optimization_bench": {
            "baseline_pattern": r"Horner/N=16_runtime",
            "poet_pattern": r"Horner/N=16_dispatched",
        },
    }

    all_compilers: set[str] = set()
    for bench_name in bench_configs:
        if bench_name in data:
            all_compilers.update(data[bench_name].keys())

    compilers = sorted(all_compilers, key=compiler_sort_key)
    if not compilers:
        print("Warning: no data for average improvement chart", file=sys.stderr)
        return

    import numpy as np

    geo_means = []
    for compiler in compilers:
        speedups = []
        for bench_name, cfg in bench_configs.items():
            if bench_name not in data or compiler not in data[bench_name]:
                continue
            entries = data[bench_name][compiler]
            baseline = find_entry(entries, cfg["baseline_pattern"])
            poet = find_entry(entries, cfg["poet_pattern"])
            b_nsop = entry_nsop(baseline)
            p_nsop = entry_nsop(poet)
            if b_nsop and p_nsop and p_nsop > 0:
                speedups.append(b_nsop / p_nsop)

        if speedups:
            geo_mean = math.exp(sum(math.log(s) for s in speedups) / len(speedups))
        else:
            geo_mean = 0
        geo_means.append(geo_mean)

    if not any(g > 0 for g in geo_means):
        print(
            "Warning: no valid speedup data for average improvement chart",
            file=sys.stderr,
        )
        return

    fig, ax = plt.subplots(figsize=(max(8, len(compilers) * 1.5), 5))
    x = np.arange(len(compilers))
    colors = [compiler_color(c) for c in compilers]

    bars = ax.bar(x, geo_means, 0.6, color=colors)
    for bar, g in zip(bars, geo_means):
        if g > 0:
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 0.02,
                f"{g:.2f}x",
                ha="center",
                va="bottom",
                fontsize=9,
                fontweight="bold",
            )

    valid = [g for g in geo_means if g > 0]
    if valid:
        overall = math.exp(sum(math.log(g) for g in valid) / len(valid))
        ax.axhline(
            y=overall,
            color="#C44E52",
            linestyle="--",
            linewidth=1.5,
            label=f"overall average: {overall:.2f}x",
        )

    ax.axhline(y=1.0, color="#CCCCCC", linestyle="--", linewidth=0.8)
    ax.set_xticks(x)
    ax.set_xticklabels(compilers, rotation=30, ha="right")
    ax.set_ylabel("Geometric mean speedup vs baseline")
    style_chart(ax, "Average POET improvement across all benchmarks")
    ax.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.15),
        ncol=2,
        framealpha=0.9,
        fontsize=9,
    )

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
    generate_average_improvement_chart(data, output_dir / "average_improvement.svg")
    print("\nDone.")


if __name__ == "__main__":
    main()
