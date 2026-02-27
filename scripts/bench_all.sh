#!/usr/bin/env bash
# bench_all.sh — Build and run all POET benchmarks across multiple compilers.
#
# Usage:
#   bash scripts/bench_all.sh                           # all detected compilers
#   POET_COMPILERS="gcc-15 clang-22" bash scripts/bench_all.sh  # subset
#
# Outputs:
#   build_bench/<compiler>/<variant>/   — isolated CMake build dirs
#   results/<compiler>/<variant>/       — benchmark text, JSON, and ASM
#   results/summary/                    — aggregated Markdown/CSV/ASM reports

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# ── Compiler discovery ────────────────────────────────────────────────────────

ALL_COMPILERS=(
    gcc-12 gcc-13 gcc-14 gcc-15
    clang-18 clang-19 clang-20 clang-21 clang-22
)

if [[ -n "${POET_COMPILERS:-}" ]]; then
    IFS=' ' read -ra COMPILERS <<< "$POET_COMPILERS"
else
    COMPILERS=()
    for c in "${ALL_COMPILERS[@]}"; do
        bin="${c/gcc-/g++-}"
        bin="${bin/clang-/clang++-}"
        if command -v "$bin" &>/dev/null; then
            COMPILERS+=("$c")
        fi
    done
fi

if [[ ${#COMPILERS[@]} -eq 0 ]]; then
    echo "ERROR: No compilers found. Install g++-{12..15} or clang++-{18..22}."
    exit 1
fi

echo "=== POET Multi-Compiler Benchmark ==="
echo "Compilers: ${COMPILERS[*]}"
echo "Project:   $PROJECT_ROOT"
echo ""

# ── Variants ──────────────────────────────────────────────────────────────────

VARIANTS=("default" "native")

# ── Benchmark targets ─────────────────────────────────────────────────────────

BENCH_TARGETS=(
    poet_compiler_comparison_bench
    poet_dispatch_bench
    poet_dispatch_optimization_bench
    poet_static_for_bench
    poet_dynamic_for_bench
)

BENCH_NAMES=(
    compiler_comparison_bench
    dispatch_bench
    dispatch_optimization_bench
    static_for_bench
    dynamic_for_bench
)

# ── Build & run loop ─────────────────────────────────────────────────────────

CPM_CACHE="${HOME}/.cpm"

for compiler in "${COMPILERS[@]}"; do
    # Map compiler name to binary
    cxx_bin="${compiler/gcc-/g++-}"
    cxx_bin="${cxx_bin/clang-/clang++-}"

    if ! command -v "$cxx_bin" &>/dev/null; then
        echo "WARNING: $cxx_bin not found, skipping $compiler"
        continue
    fi

    for variant in "${VARIANTS[@]}"; do
        build_dir="build_bench/${compiler}/${variant}"
        result_dir="results/${compiler}/${variant}"
        asm_dir="${result_dir}/asm"

        echo "──────────────────────────────────────────────────"
        echo "Building: $compiler / $variant"
        echo "  CXX:       $cxx_bin"
        echo "  Build dir: $build_dir"
        echo "  Results:   $result_dir"
        echo ""

        mkdir -p "$build_dir" "$result_dir" "$asm_dir"

        # ── Configure ─────────────────────────────────────────────────────
        cmake_extra_flags=""
        if [[ "$variant" == "native" ]]; then
            cmake_extra_flags="-DCMAKE_CXX_FLAGS=-march=native"
        fi

        if ! cmake -S . -B "$build_dir" -G Ninja \
            -DCMAKE_CXX_COMPILER="$cxx_bin" \
            -DCMAKE_BUILD_TYPE=Release \
            -DPOET_BUILD_BENCHMARKS=ON \
            -DPOET_BUILD_TESTS=OFF \
            -DPOET_ENABLE_SANITIZERS=OFF \
            -DPOET_WARNINGS_AS_ERRORS=OFF \
            -DCPM_SOURCE_CACHE="$CPM_CACHE" \
            $cmake_extra_flags \
            2>&1 | tail -5; then
            echo "WARNING: CMake configure failed for $compiler/$variant, skipping"
            continue
        fi

        # ── Build ─────────────────────────────────────────────────────────
        if ! cmake --build "$build_dir" --target "${BENCH_TARGETS[@]}" -j"$(nproc)" 2>&1 | tail -5; then
            echo "WARNING: Build failed for $compiler/$variant, running available benchmarks"
        fi

        # ── Run benchmarks ────────────────────────────────────────────────
        for idx in "${!BENCH_TARGETS[@]}"; do
            target="${BENCH_TARGETS[$idx]}"
            name="${BENCH_NAMES[$idx]}"
            binary="${build_dir}/benchmarks/${target}"

            # For native variant, check for _native binary too
            if [[ "$variant" == "native" ]]; then
                native_binary="${build_dir}/benchmarks/${target}_native"
                if [[ -f "$native_binary" ]]; then
                    binary="$native_binary"
                fi
            fi

            if [[ ! -f "$binary" ]]; then
                echo "  SKIP: $binary not found"
                continue
            fi

            echo "  Running: $name ($binary)"
            txt_file="${result_dir}/${name}.txt"
            json_file="${result_dir}/${name}.json"

            # Run benchmark, capture output
            if "$binary" > "$txt_file" 2>&1; then
                echo "    OK: $txt_file"
            else
                echo "    WARNING: $name crashed (partial output saved)"
            fi

            # Parse to JSON
            if [[ -f "$txt_file" ]]; then
                python3 "$SCRIPT_DIR/parse_bench.py" "$txt_file" "$json_file" 2>/dev/null || \
                    echo "    WARNING: parse_bench.py failed for $txt_file"
            fi

            # Extract hot-path assembly
            if [[ -f "$binary" ]]; then
                asm_file="${asm_dir}/${name}_hot.asm"
                python3 "$SCRIPT_DIR/extract_asm.py" "$binary" "$asm_file" \
                    --compiler "$compiler" --bench "$name" 2>/dev/null || \
                    echo "    WARNING: extract_asm.py failed for $binary"
            fi
        done

        echo ""
    done
done

# ── Aggregate results ─────────────────────────────────────────────────────────

echo "=== Generating summary reports ==="
python3 "$SCRIPT_DIR/analyze_bench.py" \
    --results-root results \
    --output-md results/summary/bench_comparison.md \
    --output-csv results/summary/bench_comparison.csv \
    --asm-md results/summary/asm_analysis.md

echo ""
echo "=== Done ==="
echo "Summary:  results/summary/bench_comparison.md"
echo "CSV:      results/summary/bench_comparison.csv"
echo "ASM:      results/summary/asm_analysis.md"
