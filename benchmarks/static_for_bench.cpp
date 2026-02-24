/// \file static_for_bench.cpp
/// \brief static_for benchmark: register-tuned BlockSize.
///
/// Two sections:
///   1. Map: apply heavy_work element-wise (no serial deps, pure ILP).
///   2. Multi-accumulator: for loop vs tuned BS vs default BS at N=256.
///
///   optimal_bs = vec_regs × lanes_64 / 2
///              SSE2  → 16   AVX2  → 32   AVX-512 → 128

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include <nanobench.h>

#include <poet/poet.hpp>

using namespace std::chrono_literals;

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

/// 5-deep multiply-add chain (~20 cycle latency per call).
/// The FMA chain needs 5 broadcast constants + intermediates in vector registers.
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

// Tuned: balances ILP, register pressure, and icache footprint.
//   optimal_bs ≈ vec_regs × lanes_64 / 2
// Clamped to [4, 128] to avoid degenerate cases.
constexpr std::size_t tuned_bs = [] {
    auto v = vec_regs * lanes_64 / 2;
    return v < 4 ? 4 : (v > 128 ? 128 : v);
}();

}// namespace

int main() {
    {
        std::cout << "\n=== Register Info ===\n";
        std::cout << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cout << "Vector registers: " << vec_regs << "\n";
        std::cout << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cout << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cout << "Tuned BS:         " << tuned_bs << "  (vec_regs * lanes_64 / 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Section 1: Map (N=256, heavy body)
    //
    // Element-wise transform: out[i] = heavy_work(i).  No serial dependency
    // chain — every iteration is independent, so unrolling unlocks real ILP.
    // ════════════════════════════════════════════════════════════════════════
    {
        ankerl::nanobench::Bench b;
        b.minEpochTime(20ms).relative(true);
        b.title("Map: static_for tuned vs default (N=" + std::to_string(kSweepN) + ", heavy body)");

        run(b, kSweepN, "for loop", [salt] {
            std::array<double, kSweepN> out{};
            for (std::size_t i = 0; i < kSweepN; ++i) out[i] = heavy_work(i, salt);
            return reduce(out);
        });

        run(b, kSweepN, "static_for (tuned BS)", [salt] {
            std::array<double, kSweepN> out{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN), 1, tuned_bs>(
              MapFunctor<kSweepN>{ .out = out, .salt = salt });
            return reduce(out);
        });

        run(b, kSweepN, "static_for (default BS)", [salt] {
            std::array<double, kSweepN> out{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN)>(MapFunctor<kSweepN>{ .out = out, .salt = salt });
            return reduce(out);
        });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Section 2: Multi-accumulator (N=256, heavy body)
    //
    // for loop baseline vs static_for tuned (BS=optimal) vs static_for
    // default (BS=N, fully inlined).  Shows the benefit of register-aware
    // block sizing.
    // ════════════════════════════════════════════════════════════════════════
    {
        ankerl::nanobench::Bench b;
        b.minEpochTime(20ms).relative(true);
        b.title("Multi-acc: static_for tuned vs default (N=" + std::to_string(kSweepN) + ", heavy body)");

        run(b, kSweepN, "for loop", [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < kSweepN; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        run(b, kSweepN, "static_for (tuned BS)", [salt] {
            std::array<double, tuned_bs> accs{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN), 1, tuned_bs>(
              MultiAccFunctor<tuned_bs>{ .accs = accs, .salt = salt });
            return reduce(accs);
        });

        run(b, kSweepN, "static_for (default BS)", [salt] {
            // Default BS = N (fully inlined, no isolation).
            // Uses single accumulator like the for loop — no multi-acc ILP.
            std::array<double, kSweepN> accs{};
            poet::static_for<0, static_cast<std::intmax_t>(kSweepN)>(
              MultiAccFunctor<kSweepN>{ .accs = accs, .salt = salt });
            return reduce(accs);
        });
    }
}
