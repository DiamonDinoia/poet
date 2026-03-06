/// \file dynamic_for_bench.cpp
/// \brief Register-tuned dynamic_for benchmark.
///
/// Two benchmark groups:
///
/// 1. **Multi-acc ILP**: for loop (1-acc) vs for loop (optimal accs) vs
///    dynamic_for (optimal accs).
///
/// 2. **Unroll comparison**: plain for (1-acc) vs dynamic_for<optimal> vs
///    dynamic_for<spill>.

#include <array>
#include <cstddef>
#include <cstdint>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>

namespace {

// ── Register-aware tuning ────────────────────────────────────────────────────

constexpr auto regs = poet::available_registers();
constexpr std::size_t lanes_64 = regs.lanes_64bit;

constexpr std::size_t optimal_accs = lanes_64 * 2;
constexpr std::size_t spill_accs = optimal_accs * 4;
constexpr std::size_t N = 10000;

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

template<std::size_t M> double reduce(const std::array<double, M> &a) {
    double t = 0.0;
    for (double v : a) t += v;
    return t;
}

template<std::size_t NumAccs> double hand_unrolled_multi_acc(std::size_t count, std::uint32_t salt) {
    std::array<double, NumAccs> accs{};
    const std::size_t full = count - (count % NumAccs);

    std::size_t i = 0;
    for (; i < full; i += NumAccs)
        for (std::size_t lane = 0; lane < NumAccs; ++lane) accs[lane] += heavy_work(i + lane, salt);
    for (; i < count; ++i) accs[0] += heavy_work(i, salt);

    return reduce(accs);
}

template<std::size_t Unroll> double dynamic_for_multi_acc(std::size_t count, std::uint32_t salt) {
    std::array<double, Unroll> accs{};
    poet::dynamic_for<Unroll>(std::size_t{ 0 }, count, [&accs, salt](auto lane_c, std::size_t i) {
        constexpr auto lane = decltype(lane_c)::value;
        accs[lane] += heavy_work(i, salt);
    });
    return reduce(accs);
}

}// namespace

// ════════════════════════════════════════════════════════════════════════
// Multi-acc: for loop (1 acc) vs hand-unrolled vs dynamic_for
// ════════════════════════════════════════════════════════════════════════

static void BM_for_loop_1acc(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        double acc = 0.0;
        for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_for_loop_1acc);

static void BM_for_loop_optimal_accs(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        benchmark::DoNotOptimize(hand_unrolled_multi_acc<optimal_accs>(N, salt));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_for_loop_optimal_accs);

static void BM_dynamic_for_1acc(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        benchmark::DoNotOptimize(dynamic_for_multi_acc<1>(N, salt));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_dynamic_for_1acc);

static void BM_dynamic_for_optimal_accs(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        benchmark::DoNotOptimize(dynamic_for_multi_acc<optimal_accs>(N, salt));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_dynamic_for_optimal_accs);

// ════════════════════════════════════════════════════════════════════════
// Unroll comparison: plain for vs optimal vs spill
// ════════════════════════════════════════════════════════════════════════

static void BM_unroll_plain_for(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        double acc = 0.0;
        for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_unroll_plain_for);

static void BM_unroll_dynamic_for_optimal(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        benchmark::DoNotOptimize(dynamic_for_multi_acc<optimal_accs>(N, salt));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_unroll_dynamic_for_optimal);

static void BM_unroll_dynamic_for_spill(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        benchmark::DoNotOptimize(dynamic_for_multi_acc<spill_accs>(N, salt));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_unroll_dynamic_for_spill);

BENCHMARK_MAIN();
