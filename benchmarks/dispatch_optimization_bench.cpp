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
#include <string>
#include <tuple>

#include <benchmark/benchmark.h>

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

/// Runtime N version: compiler sees a loop with trip count in a register.
inline double horner_runtime(const double *coeffs, int n, double x) noexcept {
    double result = coeffs[n - 1];
    for (int i = n - 2; i >= 0; --i) { result = result * x + coeffs[i]; }
    return result;
}

/// Compile-time N version: compiler unrolls completely and schedules optimally.
template<int N> inline double horner_compiletime(const double *coeffs, double x) noexcept {
    double result = coeffs[N - 1];
    poet::static_for<0, N - 1>([&](auto I) {
        constexpr int i = N - 2 - static_cast<int>(decltype(I)::value);
        result = result * x + coeffs[i];
    });
    return result;
}

/// Functor for dispatch: receives compile-time N, calls horner_compiletime<N>.
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

// ── Dispatch range covering all tested N values ─────────────────────────────

using dispatch_range = poet::make_range<4, 32>;

// ── Per-N benchmark templates ───────────────────────────────────────────────

template<int N> void BM_runtime(benchmark::State &state) {
    const auto salt = next_salt();
    auto coeffs = make_coeffs<N>(salt);
    double x = static_cast<double>(static_cast<std::int32_t>(xorshift32(salt))) * 1e-10;
    for (auto _ : state) {
        volatile int n = N;
        benchmark::DoNotOptimize(horner_runtime(coeffs.data(), n, x));
    }
}

template<int N> void BM_dispatched(benchmark::State &state) {
    const auto salt = next_salt();
    auto coeffs = make_coeffs<N>(salt);
    double x = static_cast<double>(static_cast<std::int32_t>(xorshift32(salt))) * 1e-10;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
          poet::dispatch(HornerDispatch{ coeffs.data(), x }, poet::DispatchParam<dispatch_range>{ N }));
    }
}

}// namespace

BENCHMARK(BM_runtime<4>)->Name("N=4_runtime");
BENCHMARK(BM_dispatched<4>)->Name("N=4_dispatched");
BENCHMARK(BM_runtime<8>)->Name("N=8_runtime");
BENCHMARK(BM_dispatched<8>)->Name("N=8_dispatched");
BENCHMARK(BM_runtime<16>)->Name("N=16_runtime");
BENCHMARK(BM_dispatched<16>)->Name("N=16_dispatched");
BENCHMARK(BM_runtime<32>)->Name("N=32_runtime");
BENCHMARK(BM_dispatched<32>)->Name("N=32_dispatched");

BENCHMARK_MAIN();
