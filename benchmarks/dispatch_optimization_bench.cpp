/// \file dispatch_optimization_bench.cpp
/// \brief Dispatch optimization benchmark: compile-time N vs runtime n.
///
/// Demonstrates that `poet::dispatch(F<N>, ...)` lets the compiler optimize
/// the *body* better than `f(runtime_n)`.  When N is known at compile time,
/// the compiler can fully unroll loops, constant-fold coefficients, and
/// schedule instructions optimally.
///
/// Workload: Horner polynomial evaluation of degree N.  With compile-time N
/// the compiler unrolls the evaluation chain and can interleave independent
/// operations; with runtime N it emits a counted loop with a data dependency
/// on every iteration.

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <benchmark/benchmark.h>
#pragma GCC diagnostic pop

#include <poet/poet.hpp>

namespace {

// ── PRNG & anti-optimization ────────────────────────────────────────────────

static inline std::uint32_t xorshift32(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

volatile std::uint32_t g_salt = 1;

std::uint32_t next_salt() noexcept {
    auto s = xorshift32(g_salt);
    g_salt = s;
    return s;
}

// ── Horner polynomial evaluation ────────────────────────────────────────────

inline double horner_runtime(const double *coeffs, int n, double x) noexcept {
    double result = coeffs[n - 1];
    for (int i = n - 2; i >= 0; --i) { result = result * x + coeffs[i]; }
    return result;
}

template<int N> inline double horner_compiletime(const double *coeffs, double x) noexcept {
    double result = coeffs[N - 1];
    poet::static_for<0, N - 1>([&](auto I) {
        constexpr int i = N - 2 - static_cast<int>(decltype(I)::value);
        result = result * x + coeffs[i];
    });
    return result;
}

struct HornerDispatch {
    const double *coeffs;
    double x;

    template<int N> double operator()() const { return horner_compiletime<N>(coeffs, x); }
};

// ── Coefficient generation ──────────────────────────────────────────────────

template<std::size_t N> std::array<double, N> make_coeffs(std::uint32_t salt) {
    std::array<double, N> c{};
    std::uint32_t s = salt;
    for (std::size_t i = 0; i < N; ++i) {
        s = xorshift32(s);
        c[i] = static_cast<double>(static_cast<std::int32_t>(s)) * 1e-10;
    }
    return c;
}

using dispatch_range = poet::make_range<4, 32>;

// ── Bench helper ────────────────────────────────────────────────────────────

template<typename Fn> void reg(const char *name, Fn &&fn) {
    benchmark::RegisterBenchmark(name, [fn = std::forward<Fn>(fn)](benchmark::State &state) mutable {
        for (auto _ : state) benchmark::DoNotOptimize(fn());
    })->MinTime(0.1);
}

// ── Per-N benchmark pair ────────────────────────────────────────────────────

template<int N> void bench_pair(std::uint32_t salt) {
    auto coeffs = make_coeffs<N>(salt);
    double x = static_cast<double>(static_cast<std::int32_t>(xorshift32(salt))) * 1e-10;

    // Baseline: runtime N
    {
        constexpr const char *names[] = {
            "Horner/N=4_runtime",
            "Horner/N=5_runtime",
            "Horner/N=6_runtime",
            "Horner/N=7_runtime",
            "Horner/N=8_runtime",
            "Horner/N=9_runtime",
            "Horner/N=10_runtime",
            "Horner/N=11_runtime",
            "Horner/N=12_runtime",
            "Horner/N=13_runtime",
            "Horner/N=14_runtime",
            "Horner/N=15_runtime",
            "Horner/N=16_runtime",
            "Horner/N=17_runtime",
            "Horner/N=18_runtime",
            "Horner/N=19_runtime",
            "Horner/N=20_runtime",
            "Horner/N=21_runtime",
            "Horner/N=22_runtime",
            "Horner/N=23_runtime",
            "Horner/N=24_runtime",
            "Horner/N=25_runtime",
            "Horner/N=26_runtime",
            "Horner/N=27_runtime",
            "Horner/N=28_runtime",
            "Horner/N=29_runtime",
            "Horner/N=30_runtime",
            "Horner/N=31_runtime",
            "Horner/N=32_runtime",
        };
        static_assert(N >= 4 && N <= 32, "N out of label range");
        reg(names[N - 4], [coeffs, x] {
            volatile int n = N;
            return horner_runtime(coeffs.data(), n, x);
        });
    }

    // Dispatched: compile-time N
    {
        constexpr const char *names[] = {
            "Horner/N=4_dispatched",
            "Horner/N=5_dispatched",
            "Horner/N=6_dispatched",
            "Horner/N=7_dispatched",
            "Horner/N=8_dispatched",
            "Horner/N=9_dispatched",
            "Horner/N=10_dispatched",
            "Horner/N=11_dispatched",
            "Horner/N=12_dispatched",
            "Horner/N=13_dispatched",
            "Horner/N=14_dispatched",
            "Horner/N=15_dispatched",
            "Horner/N=16_dispatched",
            "Horner/N=17_dispatched",
            "Horner/N=18_dispatched",
            "Horner/N=19_dispatched",
            "Horner/N=20_dispatched",
            "Horner/N=21_dispatched",
            "Horner/N=22_dispatched",
            "Horner/N=23_dispatched",
            "Horner/N=24_dispatched",
            "Horner/N=25_dispatched",
            "Horner/N=26_dispatched",
            "Horner/N=27_dispatched",
            "Horner/N=28_dispatched",
            "Horner/N=29_dispatched",
            "Horner/N=30_dispatched",
            "Horner/N=31_dispatched",
            "Horner/N=32_dispatched",
        };
        static_assert(N >= 4 && N <= 32, "N out of label range");
        reg(names[N - 4], [coeffs, x] {
            return poet::dispatch(HornerDispatch{ coeffs.data(), x }, poet::DispatchParam<dispatch_range>{ N });
        });
    }
}

}// namespace

int main(int argc, char **argv) {
    const auto salt = next_salt();

    bench_pair<4>(salt);
    bench_pair<8>(salt);
    bench_pair<16>(salt);
    bench_pair<32>(salt);

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
