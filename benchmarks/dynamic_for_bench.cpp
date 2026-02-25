/// \file dynamic_for_bench.cpp
/// \brief Register-tuned dynamic_for benchmark.
///
/// Two benchmark groups:
///
/// 1. **Multi-acc ILP**: for loop (1-acc) vs for loop (tuned accs) vs
///    dynamic_for (tuned accs).  Shows that dynamic_for's compile-time lane
///    indices enable independent accumulator chains that break serial
///    dependency bottlenecks.
///
/// 2. **Unroll sweep**: plain for (1-acc) vs dynamic_for at Unroll = 2, 4, 8,
///    tuned.  Finds the sweet-spot unroll factor for the current ISA.

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <nanobench.h>

#include <poet/poet.hpp>

using namespace std::chrono_literals;

namespace {

// ── Register-aware tuning ────────────────────────────────────────────────────

constexpr auto regs = poet::available_registers();
constexpr std::size_t vec_regs = regs.vector_registers;
constexpr std::size_t lanes_64 = regs.lanes_64bit;

// Tuned unroll factor: 2 vector registers worth of 64-bit lanes.
//
// GCC packs the N accumulators from the carried-index fold into
// ceil(N / lanes_per_reg) vector registers.  Going beyond 2 regs
// works when N is a clean multiple of lanes_per_reg, but on SSE2
// (2 lanes) GCC collapses folds wider than ~4 to a scalar stack
// loop.  2 * lanes_64 = 8 (AVX2) / 4 (SSE2) is the sweet spot
// that stays vectorised on both ISAs.
constexpr std::size_t tuned_accs = lanes_64 * 2;

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

template<typename Fn> void run(ankerl::nanobench::Bench &b, std::uint64_t batch, const char *name, Fn &&fn) {
    b.batch(batch).run(name, [fn = std::forward<Fn>(fn)]() mutable { ankerl::nanobench::doNotOptimizeAway(fn()); });
}

template<std::size_t N> double reduce(const std::array<double, N> &a) {
    double t = 0.0;
    for (double v : a) t += v;
    return t;
}

// ── Hand-unrolled multi-acc for loop ─────────────────────────────────────────

template<std::size_t NumAccs> double hand_unrolled_multi_acc(std::size_t count, std::uint32_t salt) {
    std::array<double, NumAccs> accs{};
    const std::size_t full = count - (count % NumAccs);

    std::size_t i = 0;
    for (; i < full; i += NumAccs)
        for (std::size_t lane = 0; lane < NumAccs; ++lane) accs[lane] += heavy_work(i + lane, salt);
    for (; i < count; ++i) accs[0] += heavy_work(i, salt);

    return reduce(accs);
}

// ── dynamic_for multi-acc wrapper (templated on Unroll) ──────────────────────

template<std::size_t Unroll> double dynamic_for_multi_acc(std::size_t count, std::uint32_t salt) {
    std::array<double, Unroll> accs{};
    poet::dynamic_for<Unroll>(std::size_t{ 0 }, count, [&accs, salt](auto lane_c, std::size_t i) {
        constexpr auto lane = decltype(lane_c)::value;
        accs[lane] += heavy_work(i, salt);
    });
    return reduce(accs);
}

}// namespace

int main() {
    {
        std::cout << "\n=== Register-Aware Tuning ===\n";
        std::cout << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cout << "Vector registers: " << vec_regs << "\n";
        std::cout << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cout << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cout << "Tuned accums:     " << tuned_accs << "  (lanes_64 * 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Multi-acc: for loop (1 acc) vs hand-unrolled vs dynamic_for
    //
    // Shows the progression:
    //   1. Single accumulator — serial dependency chain, no ILP
    //   2. Hand-unrolled multi-acc — manual lane splitting
    //   3. dynamic_for with lane callbacks — POET handles the splitting
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        ankerl::nanobench::Bench b;
        b.minEpochTime(50ms).relative(true);
        b.title("Multi-acc: dynamic_for lane callbacks (N=10000)");

        run(b, N, "for loop (1 acc)", [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        run(b, N, "for loop (tuned accs)", [salt] { return hand_unrolled_multi_acc<tuned_accs>(N, salt); });

        run(b, N, "dynamic_for (tuned accs)", [salt] { return dynamic_for_multi_acc<tuned_accs>(N, salt); });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Unroll comparison: plain for vs tuned vs over-unrolled
    //
    // 1. plain for (1-acc)  — serial dependency chain, baseline
    // 2. dynamic_for<tuned> — register-aware unroll (lanes_64 * 2)
    // 3. dynamic_for<4x>    — deliberate over-unroll to show spill cost
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;
        constexpr std::size_t over_unroll = tuned_accs * 4;

        ankerl::nanobench::Bench b;
        b.minEpochTime(50ms).relative(true);
        b.title("Unroll: plain for vs tuned vs over-unrolled (N=10000)");

        run(b, N, "plain for (1 acc)", [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        run(b, N, "dynamic_for<tuned>", [salt] { return dynamic_for_multi_acc<tuned_accs>(N, salt); });

        run(b, N, "dynamic_for<4x over>", [salt] { return dynamic_for_multi_acc<over_unroll>(N, salt); });
    }
}
