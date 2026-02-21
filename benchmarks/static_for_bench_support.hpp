#ifndef POET_BENCHMARKS_STATIC_FOR_BENCH_SUPPORT_HPP
#define POET_BENCHMARKS_STATIC_FOR_BENCH_SUPPORT_HPP

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

#include <nanobench.h>

#include <poet/core/static_for.hpp>

namespace poet_bench::static_for_support {

inline constexpr std::uint32_t seed = 42;

// xorshift32: simple PRNG that both GCC and Clang vectorize well.
// Used in game engines, simulation, and production systems.
static inline std::uint32_t xorshift32(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

template<typename Fn> void run_case(ankerl::nanobench::Bench &bench, std::uint64_t batch, const char *label, Fn &&fn) {
    auto fn_local = std::forward<Fn>(fn);
    bench.batch(batch).run(label, [fn_local = std::move(fn_local)]() mutable {
        const auto result = fn_local();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

void run_static_for_bench_dependent(std::uint32_t s);
void run_static_for_bench_simple(std::uint32_t s);
void run_static_for_bench_unroll(std::uint32_t s);
void run_static_for_bench_blocksize(std::uint32_t s);

struct dependent_ops_functor {
    std::uint64_t &acc;
    std::uint32_t s;

    template<auto I> void operator()() {
        // Simple integer accumulation of hash outputs
        const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(I) + static_cast<std::uint32_t>(s));
        acc += r;
    }
};

inline double runtime_dependent_ops(std::intmax_t n, std::uint32_t s) {
    std::uint64_t acc = 0;
    for (std::intmax_t i = 0; i < n; ++i) {
        const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(i) + static_cast<std::uint32_t>(s));
        acc += r;
    }
    return static_cast<double>(acc);
}

template<std::intmax_t N> inline double static_for_dependent_ops(std::uint32_t s) {
    std::uint64_t acc = 0;
    poet::static_for<0, N>(dependent_ops_functor{ acc, s });
    return static_cast<double>(acc);
}

struct simple_functor {
    std::uint64_t &acc;
    std::uint32_t s;

    template<auto I> void operator()() {
        // Simple integer accumulation of hash outputs
        const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(I) + static_cast<std::uint32_t>(s));
        acc += r;
    }
};

inline double runtime_simple(std::intmax_t n, std::uint32_t s) {
    std::uint64_t acc = 0;
    for (std::intmax_t i = 0; i < n; ++i) {
        const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(i) + static_cast<std::uint32_t>(s));
        acc += r;
    }
    return static_cast<double>(acc);
}

template<std::intmax_t N> inline double static_for_simple(std::uint32_t s) {
    std::uint64_t acc = 0;
    poet::static_for<0, N>(simple_functor{ acc, s });
    return static_cast<double>(acc);
}

template<std::size_t NumAccs> struct unrolled_functor {
    std::array<std::uint64_t, NumAccs> &accs;
    std::uint32_t s;

    template<auto I> void operator()() {
        // Distribute hash outputs across accumulators to avoid register pressure
        const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(I) + static_cast<std::uint32_t>(s));
        constexpr std::size_t idx = static_cast<std::size_t>(I) % NumAccs;
        accs[idx] += r;
    }
};

template<std::size_t UNROLL> inline double runtime_unroll(std::intmax_t n, std::uint32_t s) {
    constexpr std::intmax_t U = static_cast<std::intmax_t>(UNROLL);
    std::array<std::uint64_t, UNROLL> accs{};
    std::intmax_t i = 0;

    for (; i + (U - 1) < n; i += U) {
        for (std::intmax_t k = 0; k < U; ++k) {
            const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(i + k) + s);
            accs[static_cast<std::size_t>(k)] += r;
        }
    }

    for (; i < n; ++i) {
        const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(i) + static_cast<std::uint32_t>(s));
        accs[0] += r;
    }

    std::uint64_t sum = 0;
    for (auto v : accs) sum += v;
    return static_cast<double>(sum);
}

template<std::intmax_t N, std::intmax_t BlockSize> inline double static_for_unrolled(std::uint32_t s) {
    std::array<std::uint64_t, static_cast<std::size_t>(BlockSize)> accs{};
    poet::static_for<0, N, 1, BlockSize>(unrolled_functor<static_cast<std::size_t>(BlockSize)>{ accs, s });
    std::uint64_t sum = 0;
    for (auto val : accs) sum += val;
    return static_cast<double>(sum);
}

}// namespace poet_bench::static_for_support

#endif// POET_BENCHMARKS_STATIC_FOR_BENCH_SUPPORT_HPP
