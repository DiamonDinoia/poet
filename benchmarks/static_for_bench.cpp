/// \file static_for_bench.cpp
/// \brief static_for benchmark: register-tuned BlockSize.
///
/// Two sections:
///   1. Map: apply heavy_work element-wise (no serial deps, pure ILP).
///   2. Multi-accumulator: for loop vs tuned BS vs default BS at N=256.

#include <array>
#include <cstddef>
#include <cstdint>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>

namespace {

// ── Register-aware tuning ────────────────────────────────────────────────────

constexpr auto regs = poet::available_registers();
constexpr std::size_t vec_regs = regs.vector_registers;
constexpr std::size_t lanes_64 = regs.lanes_64bit;

// ── Workload ─────────────────────────────────────────────────────────────────

static inline std::uint32_t xorshift32(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static inline double heavy_work(std::size_t i, std::uint32_t salt) noexcept {
    double x = static_cast<double>(static_cast<std::int32_t>(xorshift32(static_cast<std::uint32_t>(i) ^ salt)));
    x = x * 1.0000001192092896 + 0.3333333333333333;
    x = x * 0.9999998807907104 + 0.14285714285714285;
    x = x * 1.0000000596046448 + -0.0625;
    x = x * 1.0000001192092896 + 0.25;
    x = x * 0.9999998807907104 + -0.125;
    return x;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

volatile std::uint32_t g_salt = 1;

std::uint32_t next_salt() noexcept {
    auto s = xorshift32(g_salt);
    g_salt = s;
    return s;
}

template<std::size_t N> double reduce(const std::array<double, N> &a) {
    double t = 0.0;
    for (double v : a) t += v;
    return t;
}

// ── Functors ─────────────────────────────────────────────────────────────────

template<std::size_t N> struct MapFunctor {
    std::array<double, N> &out;
    std::uint32_t salt;
    template<auto I> void operator()() {
        constexpr auto idx = static_cast<std::size_t>(I);
        out[idx] = heavy_work(idx, salt);
    }
};

template<std::size_t NumAccs> struct MultiAccFunctor {
    std::array<double, NumAccs> &accs;
    std::uint32_t salt;
    template<auto I> void operator()() {
        constexpr auto lane = static_cast<std::size_t>(I) % NumAccs;
        accs[lane] += heavy_work(static_cast<std::size_t>(I), salt);
    }
};

// ── Multi-acc helpers ────────────────────────────────────────────────────────

constexpr std::size_t kSweepN = 256;

constexpr std::size_t optimal_bs_map = [] {
    auto v = vec_regs * lanes_64 / 2;
    return v < 4 ? 4 : (v > 128 ? 128 : v);
}();

constexpr std::size_t optimal_bs_multiacc = lanes_64 * 2;

}// namespace

// ════════════════════════════════════════════════════════════════════════
// Section 1: Map (N=256, heavy body)
// ════════════════════════════════════════════════════════════════════════

static void BM_map_for_loop(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, kSweepN> out{};
        for (std::size_t i = 0; i < kSweepN; ++i) out[i] = heavy_work(i, salt);
        benchmark::DoNotOptimize(reduce(out));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepN));
}
BENCHMARK(BM_map_for_loop);

static void BM_map_static_for_tuned(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, kSweepN> out{};
        poet::static_for<0, static_cast<std::intmax_t>(kSweepN), 1, optimal_bs_map>(
          MapFunctor<kSweepN>{ .out = out, .salt = salt });
        benchmark::DoNotOptimize(reduce(out));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepN));
}
BENCHMARK(BM_map_static_for_tuned);

static void BM_map_static_for_default(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, kSweepN> out{};
        poet::static_for<0, static_cast<std::intmax_t>(kSweepN)>(MapFunctor<kSweepN>{ .out = out, .salt = salt });
        benchmark::DoNotOptimize(reduce(out));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepN));
}
BENCHMARK(BM_map_static_for_default);

// ════════════════════════════════════════════════════════════════════════
// Section 2: Multi-accumulator (N=256, heavy body)
// ════════════════════════════════════════════════════════════════════════

static void BM_multiacc_for_loop(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        double acc = 0.0;
        for (std::size_t i = 0; i < kSweepN; ++i) acc += heavy_work(i, salt);
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepN));
}
BENCHMARK(BM_multiacc_for_loop);

static void BM_multiacc_static_for_tuned(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, optimal_bs_multiacc> accs{};
        poet::static_for<0, static_cast<std::intmax_t>(kSweepN), 1, optimal_bs_multiacc>(
          MultiAccFunctor<optimal_bs_multiacc>{ .accs = accs, .salt = salt });
        benchmark::DoNotOptimize(reduce(accs));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepN));
}
BENCHMARK(BM_multiacc_static_for_tuned);

static void BM_multiacc_static_for_default(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, kSweepN> accs{};
        poet::static_for<0, static_cast<std::intmax_t>(kSweepN)>(
          MultiAccFunctor<kSweepN>{ .accs = accs, .salt = salt });
        benchmark::DoNotOptimize(reduce(accs));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepN));
}
BENCHMARK(BM_multiacc_static_for_default);

BENCHMARK_MAIN();
