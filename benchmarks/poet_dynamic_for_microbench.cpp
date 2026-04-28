/// \file poet_dynamic_for_microbench.cpp
/// \brief Self-contained microbenchmark for dynamic_for vs blocked_for on NFFT-like loops.

#include <poet/core/cpu_info.hpp>
#include <poet/core/dynamic_for.hpp>
#include <poet/core/static_for.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using clock_t = std::chrono::steady_clock;
using cplx = std::complex<double>;

#ifndef POET_MICRO_NOINLINE
#if defined(__GNUC__) || defined(__clang__)
#define POET_MICRO_NOINLINE __attribute__((noinline))
#else
#define POET_MICRO_NOINLINE
#endif
#endif

template<int Unroll, typename Index, typename Func>
inline __attribute__((always_inline)) void blocked_for(Index n, Func &&func) {
    static_assert(Unroll > 0);
    Index i = 0;
    const Index n_main = (n / static_cast<Index>(Unroll)) * static_cast<Index>(Unroll);

    for (; i < n_main; i += static_cast<Index>(Unroll)) {
        poet::static_for<Unroll>([&](auto lane) {
            constexpr Index lane_offset = static_cast<Index>(decltype(lane)::value);
            func(i + lane_offset);
        });
    }
    for (; i < n; ++i) { func(i); }
}

template<class F> auto bench_ns(F &&fn, int repeats = 9) -> double {
    std::vector<double> samples;
    samples.reserve(repeats);

    for (int r = 0; r < repeats; ++r) {
        const auto t0 = clock_t::now();
        fn();
        const auto t1 = clock_t::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }

    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

struct naf_like_data_t {
    std::vector<cplx> sums;
    std::vector<cplx> uq0;
    std::vector<std::uint32_t> map;
};

struct chain_like_data_t {
    std::vector<cplx> fiw;
    std::vector<cplx> tbl;
    std::vector<std::uint32_t> row;
    std::vector<std::uint8_t> conj_flag;
};

auto make_naf_like(std::size_t n_targets, std::size_t n_unique) -> naf_like_data_t {
    std::mt19937_64 gen(0x12345678ULL);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::uniform_int_distribution<std::uint32_t> pick(0, static_cast<std::uint32_t>(n_unique - 1));

    naf_like_data_t data;
    data.sums.resize(n_targets);
    data.uq0.resize(n_unique);
    data.map.resize(n_targets);

    for (auto &z : data.sums) z = { dist(gen), dist(gen) };
    for (auto &z : data.uq0) z = { dist(gen), dist(gen) };
    for (auto &m : data.map) m = pick(gen);

    return data;
}

auto make_chain_like(std::size_t n_targets, std::size_t n_rows) -> chain_like_data_t {
    std::mt19937_64 gen(0xabcdefULL);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::uniform_int_distribution<std::uint32_t> pick(0, static_cast<std::uint32_t>(n_rows - 1));
    std::bernoulli_distribution coin(0.5);

    chain_like_data_t data;
    data.fiw.resize(n_targets);
    data.tbl.resize(n_rows);
    data.row.resize(n_targets);
    data.conj_flag.resize(n_targets);

    for (auto &z : data.fiw) z = { dist(gen), dist(gen) };
    for (auto &z : data.tbl) z = { dist(gen), dist(gen) };
    for (auto &m : data.row) m = pick(gen);
    for (auto &b : data.conj_flag) b = static_cast<std::uint8_t>(coin(gen));

    return data;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_naf_blocked(naf_like_data_t data, int iters) -> double {
    const cplx fj{ 0.37, -0.19 };
    const auto run = [&] {
        for (int iter = 0; iter < iters; ++iter) {
            blocked_for<Unroll>(static_cast<std::int64_t>(data.sums.size()),
              [&](std::int64_t d) { data.sums[d] += fj * data.uq0[data.map[d]]; });
        }
    };
    const auto ns = bench_ns(run);
    volatile double sink = std::accumulate(
      data.sums.begin(), data.sums.end(), 0.0, [](double acc, cplx z) { return acc + z.real() + z.imag(); });
    (void)sink;
    return ns / iters;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_naf_dynamic_ct1(naf_like_data_t data, int iters) -> double {
    const cplx fj{ 0.37, -0.19 };
    const auto run = [&] {
        for (int iter = 0; iter < iters; ++iter) {
            poet::dynamic_for<Unroll, 1>(std::int64_t{ 0 },
              static_cast<std::int64_t>(data.sums.size()),
              [&](std::int64_t d) { data.sums[d] += fj * data.uq0[data.map[d]]; });
        }
    };
    const auto ns = bench_ns(run);
    volatile double sink = std::accumulate(
      data.sums.begin(), data.sums.end(), 0.0, [](double acc, cplx z) { return acc + z.real() + z.imag(); });
    (void)sink;
    return ns / iters;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_naf_dynamic_auto(naf_like_data_t data, int iters) -> double {
    const cplx fj{ 0.37, -0.19 };
    const auto run = [&] {
        for (int iter = 0; iter < iters; ++iter) {
            poet::dynamic_for<Unroll>(std::int64_t{ 0 },
              static_cast<std::int64_t>(data.sums.size()),
              [&](std::int64_t d) { data.sums[d] += fj * data.uq0[data.map[d]]; });
        }
    };
    const auto ns = bench_ns(run);
    volatile double sink = std::accumulate(
      data.sums.begin(), data.sums.end(), 0.0, [](double acc, cplx z) { return acc + z.real() + z.imag(); });
    (void)sink;
    return ns / iters;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_chain_blocked(chain_like_data_t data, int iters) -> double {
    const cplx fj{ 0.41, 0.27 };
    const auto run = [&] {
        for (int iter = 0; iter < iters; ++iter) {
            blocked_for<Unroll>(static_cast<std::int64_t>(data.fiw.size()), [&](std::int64_t d) {
                const cplx pow = data.tbl[data.row[d]];
                data.fiw[d] += fj * (data.conj_flag[d] ? std::conj(pow) : pow);
            });
        }
    };
    const auto ns = bench_ns(run);
    volatile double sink = std::accumulate(
      data.fiw.begin(), data.fiw.end(), 0.0, [](double acc, cplx z) { return acc + z.real() + z.imag(); });
    (void)sink;
    return ns / iters;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_chain_dynamic_ct1(chain_like_data_t data, int iters) -> double {
    const cplx fj{ 0.41, 0.27 };
    const auto run = [&] {
        for (int iter = 0; iter < iters; ++iter) {
            poet::dynamic_for<Unroll, 1>(
              std::int64_t{ 0 }, static_cast<std::int64_t>(data.fiw.size()), [&](std::int64_t d) {
                  const cplx pow = data.tbl[data.row[d]];
                  data.fiw[d] += fj * (data.conj_flag[d] ? std::conj(pow) : pow);
              });
        }
    };
    const auto ns = bench_ns(run);
    volatile double sink = std::accumulate(
      data.fiw.begin(), data.fiw.end(), 0.0, [](double acc, cplx z) { return acc + z.real() + z.imag(); });
    (void)sink;
    return ns / iters;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_chain_dynamic_auto(chain_like_data_t data, int iters) -> double {
    const cplx fj{ 0.41, 0.27 };
    const auto run = [&] {
        for (int iter = 0; iter < iters; ++iter) {
            poet::dynamic_for<Unroll>(
              std::int64_t{ 0 }, static_cast<std::int64_t>(data.fiw.size()), [&](std::int64_t d) {
                  const cplx pow = data.tbl[data.row[d]];
                  data.fiw[d] += fj * (data.conj_flag[d] ? std::conj(pow) : pow);
              });
        }
    };
    const auto ns = bench_ns(run);
    volatile double sink = std::accumulate(
      data.fiw.begin(), data.fiw.end(), 0.0, [](double acc, cplx z) { return acc + z.real() + z.imag(); });
    (void)sink;
    return ns / iters;
}

template<int Unroll> POET_MICRO_NOINLINE auto run_lane_dynamic(std::size_t n, int iters) -> double {
    std::vector<double> x(n);
    std::vector<double> y(n);
    std::iota(x.begin(), x.end(), 1.0);
    std::iota(y.begin(), y.end(), 2.0);

    const auto run = [&] {
        std::array<double, Unroll> accs{};
        for (int iter = 0; iter < iters; ++iter) {
            poet::dynamic_for<Unroll, 1>(
              std::size_t{ 0 }, n, [&](auto lane, std::size_t i) { accs[lane] += x[i] * y[i] + 0.25; });
        }
        volatile double sink = std::accumulate(accs.begin(), accs.end(), 0.0);
        (void)sink;
    };

    return bench_ns(run) / iters;
}

void print_result(std::string_view name, double ns_per_iter) {
    std::cout << std::left << std::setw(28) << name << " " << std::right << std::setw(10) << std::fixed
              << std::setprecision(2) << ns_per_iter << " ns/outer\n";
}

}// namespace

int main(int argc, char **argv) {
    constexpr int unroll = 4;
    const std::size_t n_targets = 512;
    const std::size_t n_unique = 384;
    const std::size_t n_rows = 256;
    const int outer_iters = 4000;
    const int lane_outer_iters = 2000;
    const std::string_view mode = (argc > 1) ? std::string_view(argv[1]) : std::string_view("all");

    std::cout << "POET registers: " << poet::vector_register_count() << "\n";
    std::cout << "POET 64-bit lanes: " << poet::vector_lanes_64bit() << "\n";
    std::cout << "Targets: " << n_targets << ", unroll: " << unroll << "\n\n";

    const auto naf_data = make_naf_like(n_targets, n_unique);
    const auto chain_data = make_chain_like(n_targets, n_rows);

    if (mode == "naf-blocked") {
        print_result("blocked_for", run_naf_blocked<unroll>(naf_data, outer_iters));
        return 0;
    }
    if (mode == "naf-dynamic-ct1") {
        print_result("dynamic_for<4,1>", run_naf_dynamic_ct1<unroll>(naf_data, outer_iters));
        return 0;
    }
    if (mode == "naf-dynamic-auto") {
        print_result("dynamic_for<4>", run_naf_dynamic_auto<unroll>(naf_data, outer_iters));
        return 0;
    }
    if (mode == "chain-blocked") {
        print_result("blocked_for", run_chain_blocked<unroll>(chain_data, outer_iters));
        return 0;
    }
    if (mode == "chain-dynamic-ct1") {
        print_result("dynamic_for<4,1>", run_chain_dynamic_ct1<unroll>(chain_data, outer_iters));
        return 0;
    }
    if (mode == "chain-dynamic-auto") {
        print_result("dynamic_for<4>", run_chain_dynamic_auto<unroll>(chain_data, outer_iters));
        return 0;
    }
    if (mode == "lane-dynamic") {
        print_result("dynamic_for<4,1> lane", run_lane_dynamic<unroll>(1U << 20, lane_outer_iters));
        return 0;
    }

    std::cout << "[NAF-like index-only loop]\n";
    print_result("blocked_for", run_naf_blocked<unroll>(naf_data, outer_iters));
    print_result("dynamic_for<4,1>", run_naf_dynamic_ct1<unroll>(naf_data, outer_iters));
    print_result("dynamic_for<4>", run_naf_dynamic_auto<unroll>(naf_data, outer_iters));

    std::cout << "\n[Chain-like index-only loop]\n";
    print_result("blocked_for", run_chain_blocked<unroll>(chain_data, outer_iters));
    print_result("dynamic_for<4,1>", run_chain_dynamic_ct1<unroll>(chain_data, outer_iters));
    print_result("dynamic_for<4>", run_chain_dynamic_auto<unroll>(chain_data, outer_iters));

    std::cout << "\n[Lane-aware positive control]\n";
    print_result("dynamic_for<4,1> lane", run_lane_dynamic<unroll>(1U << 20, lane_outer_iters));

    return 0;
}
