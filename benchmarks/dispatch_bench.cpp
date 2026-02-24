/// \file dispatch_bench.cpp
/// \brief Dispatch benchmark: dimensionality, hit/miss, contiguous/sparse.

#include <array>
#include <chrono>
#include <tuple>

#include <nanobench.h>

#include <poet/poet.hpp>

using namespace std::chrono_literals;
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

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("dispatch: dimensionality, hit/miss, contiguous/sparse");
    bench.minEpochTime(10ms);

    // ── 1D Dispatch ─────────────────────────────────────────────────────
    bench.run("1D contiguous hit", [&] {
        auto v = 3 + (next_noise() & 3);// values in [3,6] ⊂ [1,8]
        ankerl::nanobench::doNotOptimizeAway(v);
        ankerl::nanobench::doNotOptimizeAway(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_contig>{ v }, 2));
    });
    bench.run("1D contiguous miss", [&] {
        auto v = 100 + next_noise();
        ankerl::nanobench::doNotOptimizeAway(v);
        ankerl::nanobench::doNotOptimizeAway(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_contig>{ v }, 2));
    });
    bench.run("1D non-contiguous hit", [&] {
        constexpr std::array<int, 4> vals{ 1, 20, 50, 70 };
        auto v = vals[next_noise() & 3];
        ankerl::nanobench::doNotOptimizeAway(v);
        ankerl::nanobench::doNotOptimizeAway(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_noncontig>{ v }, 2));
    });
    bench.run("1D non-contiguous miss", [&] {
        auto v = 5 + next_noise();
        ankerl::nanobench::doNotOptimizeAway(v);
        ankerl::nanobench::doNotOptimizeAway(
          poet::dispatch(simple_kernel{}, poet::DispatchParam<range_1d_noncontig>{ v }, 2));
    });

    // ── 2D Dispatch ─────────────────────────────────────────────────────
    bench.run("2D contiguous hit", [&] {
        auto w = 2 + (next_noise() & 3);
        auto h = 3 + (next_noise() & 3);
        ankerl::nanobench::doNotOptimizeAway(w);
        ankerl::nanobench::doNotOptimizeAway(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_contig>{ w }, poet::DispatchParam<range_2d_contig>{ h });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 2));
    });
    bench.run("2D contiguous miss", [&] {
        auto w = 9 + next_noise();
        auto h = 1;
        ankerl::nanobench::doNotOptimizeAway(w);
        ankerl::nanobench::doNotOptimizeAway(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_contig>{ w }, poet::DispatchParam<range_2d_contig>{ h });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 2));
    });
    bench.run("2D non-contiguous hit", [&] {
        constexpr std::array<int, 4> vals{ 10, 20, 50, 70 };
        auto w = vals[next_noise() & 3];
        auto h = vals[next_noise() & 3];
        ankerl::nanobench::doNotOptimizeAway(w);
        ankerl::nanobench::doNotOptimizeAway(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_noncontig>{ w }, poet::DispatchParam<range_2d_noncontig>{ h });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 2));
    });
    bench.run("2D non-contiguous miss", [&] {
        auto w = 5 + next_noise();
        auto h = 15;
        ankerl::nanobench::doNotOptimizeAway(w);
        ankerl::nanobench::doNotOptimizeAway(h);
        auto params =
          std::make_tuple(poet::DispatchParam<range_2d_noncontig>{ w }, poet::DispatchParam<range_2d_noncontig>{ h });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 2));
    });

    // ── 5D Dispatch (table size = 4^5 = 1024) ──────────────────────────
    bench.run("5D contiguous hit", [&] {
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_contig>{ (n + 0) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 1) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 2) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 3) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 4) & 3 });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 3));
    });
    bench.run("5D contiguous miss", [&] {
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_contig>{ 5 },// out of [0,3]
          poet::DispatchParam<range_5d_contig>{ (n + 1) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 2) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 3) & 3 },
          poet::DispatchParam<range_5d_contig>{ (n + 4) & 3 });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 3));
    });
    bench.run("5D non-contiguous hit", [&] {
        constexpr std::array<int, 4> vals{ 0, 10, 20, 30 };
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_noncontig>{ vals[(n + 0) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 1) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 2) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 3) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 4) & 3] });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 3));
    });
    bench.run("5D non-contiguous miss", [&] {
        constexpr std::array<int, 4> vals{ 0, 10, 20, 30 };
        const int n = next_noise();
        auto params = std::make_tuple(poet::DispatchParam<range_5d_noncontig>{ 5 },// not in {0,10,20,30}
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 1) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 2) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 3) & 3] },
          poet::DispatchParam<range_5d_noncontig>{ vals[(n + 4) & 3] });
        ankerl::nanobench::doNotOptimizeAway(poet::dispatch(simple_kernel{}, params, 3));
    });

    return 0;
}
