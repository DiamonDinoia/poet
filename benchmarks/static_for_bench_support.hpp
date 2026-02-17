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

inline constexpr std::uint64_t seed = 42;

static inline std::uint64_t splitmix64(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

template<typename Fn>
void run_case(ankerl::nanobench::Bench &bench, std::uint64_t batch, const char *label, Fn &&fn) {
    auto fn_local = std::forward<Fn>(fn);
    bench.batch(batch).run(label, [fn_local = std::move(fn_local)]() mutable {
        const auto result = fn_local();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

void run_static_for_bench_dependent(std::uint64_t s);
void run_static_for_bench_simple(std::uint64_t s);
void run_static_for_bench_unroll(std::uint64_t s);

struct dependent_ops_functor {
    double &acc;
    std::uint64_t s;

    template<auto I>
    void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I) + s);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;

        constexpr double coeff = 1.0 + 0.1 * (I + 1);
        x *= coeff;
        acc += x;
    }
};

inline double runtime_dependent_ops(std::intmax_t n, std::uint64_t s) {
    double acc = 0.0;
    for (std::intmax_t i = 0; i < n; ++i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i) + s);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;

        const double coeff = 1.0 + 0.1 * static_cast<double>(i + 1);
        x *= coeff;
        acc += x;
    }
    return acc;
}

template<std::intmax_t N>
inline double static_for_dependent_ops(std::uint64_t s) {
    double acc = 0.0;
    poet::static_for<0, N>(dependent_ops_functor{ acc, s });
    return acc;
}

struct simple_functor {
    double &acc;
    std::uint64_t s;

    template<auto I>
    void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I) + s);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        acc += x * x;
    }
};

inline double runtime_simple(std::intmax_t n, std::uint64_t s) {
    double acc = 0.0;
    for (std::intmax_t i = 0; i < n; ++i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i) + s);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        acc += x * x;
    }
    return acc;
}

template<std::intmax_t N>
inline double static_for_simple(std::uint64_t s) {
    double acc = 0.0;
    poet::static_for<0, N>(simple_functor{ acc, s });
    return acc;
}

template<std::size_t NumAccs>
struct unrolled_functor {
    std::array<double, NumAccs> &accs;
    std::uint64_t s;

    template<auto I>
    void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I) + s);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        constexpr std::size_t idx = static_cast<std::size_t>(I) % NumAccs;
        accs[idx] += x * x;
    }
};

template<std::size_t UNROLL>
inline double runtime_unroll(std::intmax_t n, std::uint64_t s) {
    constexpr std::intmax_t U = static_cast<std::intmax_t>(UNROLL);
    std::array<double, UNROLL> accs{};
    std::intmax_t i = 0;

    for (; i + (U - 1) < n; i += U) {
        std::array<std::uint64_t, UNROLL> r{};
        for (std::intmax_t k = 0; k < U; ++k) {
            r[static_cast<std::size_t>(k)] =
                splitmix64(static_cast<std::uint64_t>(i + k) + s);
        }

        std::array<double, UNROLL> x{};
        for (std::intmax_t k = 0; k < U; ++k) {
            x[static_cast<std::size_t>(k)] =
                static_cast<double>(r[static_cast<std::size_t>(k)]) * 5.42101086242752217e-20;
        }

        for (std::intmax_t k = 0; k < U; ++k) {
            const double v = x[static_cast<std::size_t>(k)];
            accs[static_cast<std::size_t>(k)] += v * v;
        }
    }

    for (; i < n; ++i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i) + s);
        const double x = static_cast<double>(r) * 5.42101086242752217e-20;
        accs[0] += x * x;
    }

    double sum = 0.0;
    for (double v : accs) sum += v;
    return sum;
}

template<std::intmax_t N, std::intmax_t BlockSize>
inline double static_for_unrolled(std::uint64_t s) {
    std::array<double, static_cast<std::size_t>(BlockSize)> accs{};
    poet::static_for<0, N, 1, BlockSize>(unrolled_functor<static_cast<std::size_t>(BlockSize)>{ accs, s });
    double sum = 0.0;
    for (double val : accs) sum += val;
    return sum;
}

} // namespace poet_bench::static_for_support

#endif // POET_BENCHMARKS_STATIC_FOR_BENCH_SUPPORT_HPP
