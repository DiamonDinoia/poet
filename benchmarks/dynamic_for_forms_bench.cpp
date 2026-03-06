/// \file dynamic_for_forms_bench.cpp
/// \brief Compares dynamic_for callable forms (lane vs index-only) across workloads.
///
/// Three sections:
///   1. Accumulation (serial dependency) — lane form breaks dep chains, should win
///   2. Element-wise independent work — body dominates, all roughly equal
///   3. Small N tail overhead — dispatch cost for tiny ranges

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

constexpr std::size_t kN = 10000;
static std::array<double, kN> out{};

}// namespace

// ════════════════════════════════════════════════════════════════════════
// Section 1: Accumulation (serial dependency)
// ════════════════════════════════════════════════════════════════════════

static void BM_acc_plain_for(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        double acc = 0.0;
        for (std::size_t i = 0; i < kN; ++i) acc += heavy_work(i, salt);
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kN));
}
BENCHMARK(BM_acc_plain_for);

static void BM_acc_index_only(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        double acc = 0.0;
        poet::dynamic_for<optimal_accs>(
          std::size_t{ 0 }, std::size_t{ kN }, [&acc, salt](std::size_t i) { acc += heavy_work(i, salt); });
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kN));
}
BENCHMARK(BM_acc_index_only);

static void BM_acc_lane_form(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, optimal_accs> accs{};
        poet::dynamic_for<optimal_accs>(
          std::size_t{ 0 }, std::size_t{ kN }, [&accs, salt](auto lane_c, std::size_t i) {
              constexpr auto lane = decltype(lane_c)::value;
              accs[lane] += heavy_work(i, salt);
          });
        benchmark::DoNotOptimize(reduce(accs));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kN));
}
BENCHMARK(BM_acc_lane_form);

// ════════════════════════════════════════════════════════════════════════
// Section 2: Element-wise independent work
// ════════════════════════════════════════════════════════════════════════

static void BM_elem_plain_for(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        for (std::size_t i = 0; i < kN; ++i) out[i] = heavy_work(i, salt);
        benchmark::DoNotOptimize(out[kN / 2]);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kN));
}
BENCHMARK(BM_elem_plain_for);

static void BM_elem_index_only(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        poet::dynamic_for<optimal_accs>(
          std::size_t{ 0 }, std::size_t{ kN }, [salt](std::size_t i) { out[i] = heavy_work(i, salt); });
        benchmark::DoNotOptimize(out[kN / 2]);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kN));
}
BENCHMARK(BM_elem_index_only);

static void BM_elem_lane_form(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        poet::dynamic_for<optimal_accs>(std::size_t{ 0 }, std::size_t{ kN }, [salt](auto /*lane_c*/, std::size_t i) {
            out[i] = heavy_work(i, salt);
        });
        benchmark::DoNotOptimize(out[kN / 2]);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kN));
}
BENCHMARK(BM_elem_lane_form);

// ════════════════════════════════════════════════════════════════════════
// Section 3: Small N tail overhead
// ════════════════════════════════════════════════════════════════════════

template<std::size_t SmallN> static void BM_small_plain_for(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::uint32_t acc = salt;
        for (std::size_t i = 0; i < SmallN; ++i) acc = xorshift32(acc + static_cast<std::uint32_t>(i));
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(SmallN));
}

template<std::size_t SmallN> static void BM_small_index_only(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::uint32_t acc = salt;
        poet::dynamic_for<8>(std::size_t{ 0 }, SmallN, [&acc](std::size_t i) {
            acc = xorshift32(acc + static_cast<std::uint32_t>(i));
        });
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(SmallN));
}

template<std::size_t SmallN> static void BM_small_lane_form(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::uint32_t acc = salt;
        poet::dynamic_for<8>(std::size_t{ 0 }, SmallN, [&acc](auto /*lane_c*/, std::size_t i) {
            acc = xorshift32(acc + static_cast<std::uint32_t>(i));
        });
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(SmallN));
}

BENCHMARK(BM_small_plain_for<3>)->Name("BM_small_plain_for_N3");
BENCHMARK(BM_small_index_only<3>)->Name("BM_small_index_only_N3");
BENCHMARK(BM_small_lane_form<3>)->Name("BM_small_lane_form_N3");
BENCHMARK(BM_small_plain_for<7>)->Name("BM_small_plain_for_N7");
BENCHMARK(BM_small_index_only<7>)->Name("BM_small_index_only_N7");
BENCHMARK(BM_small_lane_form<7>)->Name("BM_small_lane_form_N7");

BENCHMARK_MAIN();
