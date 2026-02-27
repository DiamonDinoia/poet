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

#include <nanobench.h>

#include <poet/poet.hpp>

using namespace std::chrono_literals;

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
//
// Evaluate  c[0] + c[1]*x + c[2]*x^2 + ... + c[N-1]*x^(N-1)
// using Horner's method:  ((c[N-1]*x + c[N-2])*x + c[N-3])*x + ...
//
// The coefficients are derived from the salt to prevent constant-folding
// across calls.

/// Runtime N version: compiler sees a loop with trip count in a register.
inline double horner_runtime(const double *coeffs, int n, double x) noexcept {
    double result = coeffs[n - 1];
    for (int i = n - 2; i >= 0; --i) { result = result * x + coeffs[i]; }
    return result;
}

/// Compile-time N version: compiler unrolls completely and schedules optimally.
template<int N> inline double horner_compiletime(const double *coeffs, double x) noexcept {
    double result = coeffs[N - 1];
    // static_for unrolls at compile time — each step is visible to the optimizer
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
        // Small coefficients to avoid overflow
        c[i] = static_cast<double>(static_cast<std::int32_t>(s)) * 1e-10;
    }
    return c;
}

// ── Dispatch range covering all tested N values ─────────────────────────────

// N in {4, 8, 16, 32}
using dispatch_range = poet::make_range<4, 32>;

// ── Bench helper ────────────────────────────────────────────────────────────

template<typename Fn> void run(ankerl::nanobench::Bench &b, const char *name, Fn &&fn) {
    b.run(name, [fn = std::forward<Fn>(fn)]() mutable { ankerl::nanobench::doNotOptimizeAway(fn()); });
}

// ── Per-N benchmark pair ────────────────────────────────────────────────────

template<int N> void bench_pair(ankerl::nanobench::Bench &b, std::uint32_t salt) {
    auto coeffs = make_coeffs<N>(salt);
    double x = static_cast<double>(static_cast<std::int32_t>(xorshift32(salt))) * 1e-10;

    // Baseline: runtime N — compiler cannot unroll
    {
        constexpr const char *names[] = {
            "N=4  runtime",
            "N=5  runtime",
            "N=6  runtime",
            "N=7  runtime",
            "N=8  runtime",
            "N=9  runtime",
            "N=10 runtime",
            "N=11 runtime",
            "N=12 runtime",
            "N=13 runtime",
            "N=14 runtime",
            "N=15 runtime",
            "N=16 runtime",
            "N=17 runtime",
            "N=18 runtime",
            "N=19 runtime",
            "N=20 runtime",
            "N=21 runtime",
            "N=22 runtime",
            "N=23 runtime",
            "N=24 runtime",
            "N=25 runtime",
            "N=26 runtime",
            "N=27 runtime",
            "N=28 runtime",
            "N=29 runtime",
            "N=30 runtime",
            "N=31 runtime",
            "N=32 runtime",
        };
        static_assert(N >= 4 && N <= 32, "N out of label range");
        run(b, names[N - 4], [&coeffs, x] {
            volatile int n = N;// hide N from the optimizer
            return horner_runtime(coeffs.data(), n, x);
        });
    }

    // Dispatched: compile-time N — compiler unrolls and optimizes fully
    {
        constexpr const char *names[] = {
            "N=4  dispatched",
            "N=5  dispatched",
            "N=6  dispatched",
            "N=7  dispatched",
            "N=8  dispatched",
            "N=9  dispatched",
            "N=10 dispatched",
            "N=11 dispatched",
            "N=12 dispatched",
            "N=13 dispatched",
            "N=14 dispatched",
            "N=15 dispatched",
            "N=16 dispatched",
            "N=17 dispatched",
            "N=18 dispatched",
            "N=19 dispatched",
            "N=20 dispatched",
            "N=21 dispatched",
            "N=22 dispatched",
            "N=23 dispatched",
            "N=24 dispatched",
            "N=25 dispatched",
            "N=26 dispatched",
            "N=27 dispatched",
            "N=28 dispatched",
            "N=29 dispatched",
            "N=30 dispatched",
            "N=31 dispatched",
            "N=32 dispatched",
        };
        static_assert(N >= 4 && N <= 32, "N out of label range");
        run(b, names[N - 4], [&coeffs, x] {
            return poet::dispatch(HornerDispatch{ coeffs.data(), x }, poet::DispatchParam<dispatch_range>{ N });
        });
    }
}

}// namespace

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("Compile-time specialization: runtime N vs dispatched N");
    bench.minEpochTime(100ms).relative(true);

    const auto salt = next_salt();

    bench_pair<4>(bench, salt);
    bench_pair<8>(bench, salt);
    bench_pair<16>(bench, salt);
    bench_pair<32>(bench, salt);

    return 0;
}
