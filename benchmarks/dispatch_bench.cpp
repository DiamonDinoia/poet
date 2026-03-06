/// \file dispatch_bench.cpp
/// \brief Dispatch benchmark: dimensionality, hit/miss, contiguous/sparse.

#include <array>
#include <tuple>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>

namespace {

struct simple_kernel {
    template<int... Vs> int operator()(int scale) const {
        int s = 0;
        ((s += Vs), ...);
        return scale * s;
    }
};

volatile int runtime_noise = 1;

// 1D ranges
using range_1d_contig = poet::make_range<1, 8>;
using range_1d_noncontig = std::integer_sequence<int, 1, 10, 20, 30, 40, 50, 60, 70>;

// 2D ranges
using range_2d_contig = poet::make_range<1, 8>;
using range_2d_noncontig = std::integer_sequence<int, 1, 10, 20, 30, 40, 50, 60, 70>;

// 5D ranges (4 options each = table size 4^5 = 1024)
using range_5d_contig = poet::make_range<0, 3>;
using range_5d_noncontig = std::integer_sequence<int, 0, 10, 20, 30>;

int next_noise() {
    const int v = runtime_noise;
    runtime_noise = v + 1;
    return v;
}

}// namespace

// ── 1D Dispatch ─────────────────────────────────────────────────────
static void BM_1D_contiguous_hit(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 3 + (next_noise() & 3);// values in [3,6] ⊂ [1,8]
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_contig>{ v }, 2));
    }
}
BENCHMARK(BM_1D_contiguous_hit);

static void BM_1D_contiguous_miss(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 100 + next_noise();
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_contig>{ v }, 2));
    }
}
BENCHMARK(BM_1D_contiguous_miss);

static void BM_1D_noncontiguous_hit(benchmark::State &state) {
    for (auto _ : state) {
        constexpr std::array<int, 4> vals{ 1, 20, 50, 70 };
        auto v = vals[next_noise() & 3];
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_noncontig>{ v }, 2));
    }
}
BENCHMARK(BM_1D_noncontiguous_hit);

static void BM_1D_noncontiguous_miss(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 5 + next_noise();
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_noncontig>{ v }, 2));
    }
}
BENCHMARK(BM_1D_noncontiguous_miss);

// ── 2D Dispatch ─────────────────────────────────────────────────────
static void BM_2D_contiguous_hit(benchmark::State &state) {
    for (auto _ : state) {
        auto w = 2 + (next_noise() & 3);
        auto h = 3 + (next_noise() & 3);
        benchmark::DoNotOptimize(w);
        benchmark::DoNotOptimize(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_contig>{ w }, poet::DispatchParam<range_2d_contig>{ h });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 2));
    }
}
BENCHMARK(BM_2D_contiguous_hit);

static void BM_2D_contiguous_miss(benchmark::State &state) {
    for (auto _ : state) {
        auto w = 9 + next_noise();
        auto h = 1;
        benchmark::DoNotOptimize(w);
        benchmark::DoNotOptimize(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_contig>{ w }, poet::DispatchParam<range_2d_contig>{ h });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 2));
    }
}
BENCHMARK(BM_2D_contiguous_miss);

static void BM_2D_noncontiguous_hit(benchmark::State &state) {
    for (auto _ : state) {
        constexpr std::array<int, 4> vals{ 10, 20, 50, 70 };
        auto w = vals[next_noise() & 3];
        auto h = vals[next_noise() & 3];
        benchmark::DoNotOptimize(w);
        benchmark::DoNotOptimize(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_noncontig>{ w }, poet::DispatchParam<range_2d_noncontig>{ h });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 2));
    }
}
BENCHMARK(BM_2D_noncontiguous_hit);

static void BM_2D_noncontiguous_miss(benchmark::State &state) {
    for (auto _ : state) {
        auto w = 5 + next_noise();
        auto h = 15;
        benchmark::DoNotOptimize(w);
        benchmark::DoNotOptimize(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_noncontig>{ w }, poet::DispatchParam<range_2d_noncontig>{ h });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 2));
    }
}
BENCHMARK(BM_2D_noncontiguous_miss);

// ── 5D Dispatch (table size = 4^5 = 1024) ──────────────────────────
static void BM_5D_contiguous_hit(benchmark::State &state) {
    for (auto _ : state) {
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_contig>{ (n + 0) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 1) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 2) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 3) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 4) & 3 });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 3));
    }
}
BENCHMARK(BM_5D_contiguous_hit);

static void BM_5D_contiguous_miss(benchmark::State &state) {
    for (auto _ : state) {
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_contig>{ 5 },// out of [0,3]
          poet::DispatchParam<range_5d_contig>{ (n + 1) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 2) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 3) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 4) & 3 });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 3));
    }
}
BENCHMARK(BM_5D_contiguous_miss);

static void BM_5D_noncontiguous_hit(benchmark::State &state) {
    for (auto _ : state) {
        constexpr std::array<int, 4> vals{ 0, 10, 20, 30 };
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_noncontig>{ vals[(n + 0) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 1) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 2) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 3) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 4) & 3] });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 3));
    }
}
BENCHMARK(BM_5D_noncontiguous_hit);

static void BM_5D_noncontiguous_miss(benchmark::State &state) {
    for (auto _ : state) {
        constexpr std::array<int, 4> vals{ 0, 10, 20, 30 };
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_noncontig>{ 5 },// not in {0,10,20,30}
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 1) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 2) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 3) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 4) & 3] });
        benchmark::DoNotOptimize(poet::dispatch(simple_kernel{}, params, 3));
    }
}
BENCHMARK(BM_5D_noncontiguous_miss);

BENCHMARK_MAIN();
