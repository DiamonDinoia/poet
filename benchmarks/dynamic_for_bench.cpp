/// \file dynamic_for_bench.cpp
/// \brief Register-tuned dynamic_for benchmark.
///
/// Two benchmark groups:
///
/// 1. **Multi-acc ILP**: for loop (1-acc) vs for loop (optimal accs) vs
///    dynamic_for (optimal accs).  Shows that dynamic_for's compile-time lane
///    indices enable independent accumulator chains that break serial
///    dependency bottlenecks.
///
/// 2. **Unroll comparison**: plain for (1-acc) vs dynamic_for<optimal> vs
///    dynamic_for<spill>.  Contrasts the empirically validated sweet spot
///    against a value confirmed to be in spill territory by assembly inspection.
///
/// Tuning constants (AVX2, validated by unroll sweep + objdump analysis):
///   optimal_accs = lanes_64 * 2 = 8   — peak throughput at 2 SIMD regs of
///                                        accumulators; hot loop still reloads
///                                        one acc from the stack but OOO hides it
///   spill_accs   = optimal_accs * 4   — 2667 rsp refs in hot loop (vs 62 for
///                                        optimal); deep spill territory

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <benchmark/benchmark.h>
#pragma GCC diagnostic pop

#include <poet/poet.hpp>

namespace {

// ── Register-aware tuning ────────────────────────────────────────────────────

constexpr auto regs = poet::available_registers();
constexpr std::size_t vec_regs = regs.vector_registers;
constexpr std::size_t lanes_64 = regs.lanes_64bit;

constexpr std::size_t optimal_accs = lanes_64 * 2;
constexpr std::size_t spill_accs = optimal_accs * 4;

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

template<typename Fn> void reg(const char *name, std::uint64_t batch, Fn &&fn) {
    benchmark::RegisterBenchmark(name, [fn = std::forward<Fn>(fn), batch](benchmark::State &state) mutable {
        for (auto _ : state) benchmark::DoNotOptimize(fn());
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch));
    })->MinTime(0.1);
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

int main(int argc, char **argv) {
    {
        std::cerr << "\n=== Register-Aware Tuning ===\n";
        std::cerr << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cerr << "Vector registers: " << vec_regs << "\n";
        std::cerr << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cerr << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cerr << "Optimal accums:   " << optimal_accs << "  (lanes_64 * 2)\n";
        std::cerr << "Spill accums:     " << spill_accs << "  (optimal_accs * 4)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Multi-acc: for loop (1 acc) vs hand-unrolled vs dynamic_for
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        reg("Multi-acc/for_loop_1_acc", N, [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        reg("Multi-acc/for_loop_optimal_accs", N, [salt] { return hand_unrolled_multi_acc<optimal_accs>(N, salt); });

        reg("Multi-acc/dynamic_for_1_acc", N, [salt] { return dynamic_for_multi_acc<1>(N, salt); });

        reg("Multi-acc/dynamic_for_optimal_accs", N, [salt] { return dynamic_for_multi_acc<optimal_accs>(N, salt); });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Unroll comparison: plain for vs optimal vs spill
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        reg("Unroll/plain_for_1_acc", N, [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        reg("Unroll/dynamic_for_optimal", N, [salt] { return dynamic_for_multi_acc<optimal_accs>(N, salt); });

        reg("Unroll/dynamic_for_spill", N, [salt] { return dynamic_for_multi_acc<spill_accs>(N, salt); });
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
