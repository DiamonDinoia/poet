/// \file static_for_bench.cpp
/// \brief static_for benchmark: register-tuned BlockSize.
///
/// Two sections:
///   1. Map: apply heavy_work element-wise (no serial deps, pure ILP).
///   2. Multi-accumulator: for loop vs tuned BS vs default BS at N=256.
///
/// Different heuristics apply to each section:
///
///   Map (no serial deps — maximize ILP):
///     optimal_bs_map = vec_regs × lanes_64 / 2
///                  SSE2 → 16   AVX2 → 32   AVX-512 → 128
///
///   MultiAcc (serial dep per chain — avoid accumulator register spill):
///     optimal_bs_multiacc = lanes_64 × 2   (2 SIMD regs of accumulators)
///                       SSE2 → 4   AVX2 → 8   AVX-512 → 16
///     heavy_work needs ~10 registers for its FMA constants/intermediates;
///     keeping accumulators to 2 SIMD regs leaves ample headroom.

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

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

int main(int argc, char **argv) {
    {
        std::cerr << "\n=== Register Info ===\n";
        std::cerr << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cerr << "Vector registers: " << vec_regs << "\n";
        std::cerr << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cerr << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cerr << "Map BS:           " << optimal_bs_map << "  (vec_regs * lanes_64 / 2)\n";
        std::cerr << "MultiAcc BS:      " << optimal_bs_multiacc << "  (lanes_64 * 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Section 1: Map (N=256, heavy body)
    // ════════════════════════════════════════════════════════════════════════
    {
        reg("Map/for_loop", kSweepN, [salt] {
            std::array<double, kSweepN> out{};
            for (std::size_t i = 0; i < kSweepN; ++i) out[i] = heavy_work(i, salt);
            return reduce(out);
        });

        reg("Map/static_for_tuned_BS", kSweepN, [salt] {
            std::array<double, kSweepN> out{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN), 1, optimal_bs_map>(
              MapFunctor<kSweepN>{ .out = out, .salt = salt });
            return reduce(out);
        });

        reg("Map/static_for_default_BS", kSweepN, [salt] {
            std::array<double, kSweepN> out{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN)>(MapFunctor<kSweepN>{ .out = out, .salt = salt });
            return reduce(out);
        });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Section 2: Multi-accumulator (N=256, heavy body)
    // ════════════════════════════════════════════════════════════════════════
    {
        reg("MultiAcc/for_loop", kSweepN, [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < kSweepN; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        reg("MultiAcc/static_for_tuned_BS", kSweepN, [salt] {
            std::array<double, optimal_bs_multiacc> accs{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN), 1, optimal_bs_multiacc>(
              MultiAccFunctor<optimal_bs_multiacc>{ .accs = accs, .salt = salt });
            return reduce(accs);
        });

        reg("MultiAcc/static_for_default_BS", kSweepN, [salt] {
            std::array<double, kSweepN> accs{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN)>(
              MultiAccFunctor<kSweepN>{ .accs = accs, .salt = salt });
            return reduce(accs);
        });
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
