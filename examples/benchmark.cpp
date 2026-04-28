// Example: Google Benchmark microbench, runnable on Compiler Explorer.
//
// Compares scalar dot product vs lane-aware `dynamic_for<L>` for L in
// {4, 8}. The lane-aware path breaks the serial FMA dependency chain
// of the scalar loop, so wall-time-per-element drops on FMA-throughput-
// bound machines (Zen3/4, Skylake-X, Ice Lake, ...).
//
// On Compiler Explorer this example links against the Google Benchmark
// library (registered as the `benchmark` lib in the URL) and runs in
// "Execute" mode — the output pane shows actual timings.
//
// Build locally:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON -DPOET_BUILD_EXAMPLE_BENCHMARK=ON
//   cmake --build build --target example_benchmark
//   ./build/examples/example_benchmark

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>

namespace {

std::vector<double> make_data(std::size_t n, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> v(n);
    for (auto &x : v) x = dist(rng);
    return v;
}

}// namespace

static void BM_DotScalar(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto a = make_data(n, 1);
    auto b = make_data(n, 2);
    for (auto _ : state) {
        double acc = 0.0;
        for (std::size_t i = 0; i < n; ++i) acc += a[i] * b[i];
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template<std::size_t L> static void BM_DotLaneAware(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto a = make_data(n, 1);
    auto b = make_data(n, 2);
    for (auto _ : state) {
        std::array<double, L> lanes{};
        poet::dynamic_for<L>(std::size_t{ 0 }, n, [&](auto lane, std::size_t i) { lanes[lane] += a[i] * b[i]; });
        double acc = 0.0;
        for (double v : lanes) acc += v;
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

BENCHMARK(BM_DotScalar)->Arg(1024)->Arg(8192);
BENCHMARK_TEMPLATE(BM_DotLaneAware, 4)->Arg(1024)->Arg(8192);
BENCHMARK_TEMPLATE(BM_DotLaneAware, 8)->Arg(1024)->Arg(8192);

BENCHMARK_MAIN();
