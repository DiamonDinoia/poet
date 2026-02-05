#include <array>
#include <tuple>
#include <utility>
#include <chrono>
#include <nanobench.h>

#include <poet/core/static_dispatch.hpp>

using namespace std::chrono_literals;
namespace {

using width_range = poet::make_range<1, 8>;
using height_range = poet::make_range<1, 8>;

constexpr std::array<std::pair<int, int>, 6> dispatch_hits{ {
  { 1, 1 },
  { 2, 3 },
  { 4, 5 },
  { 6, 2 },
  { 7, 8 },
  { 8, 4 },
} };

constexpr std::array<std::pair<int, int>, 4> dispatch_misses{ {
  { 0, 0 },
  { 9, 1 },
  { 1, 9 },
  { 12, 12 },
} };

struct matrix_kernel {
    template<int Width, int Height> [[nodiscard]] int operator()(int scale) const {
        // Pretend to perform work that benefits from compile-time specialization.
        return scale * (Width * Height + Width + Height);
    }
};

struct big_kernel {
    template<int... Vs> int operator()(int scale) const {
        int s = 0;
        ((s += Vs), ...);
        return scale * s;
    }
};

int run_manual(int width, int height, int scale) {
    if (width < 1 || width > 8 || height < 1 || height > 8) { return 0; }
    return scale * (width * height + width + height);
}

int run_dispatch(int width, int height, int scale) {
    auto params =
      std::make_tuple(poet::DispatchParam<width_range>{ width }, poet::DispatchParam<height_range>{ height });
    return poet::dispatch(matrix_kernel{}, params, scale);
}

}// namespace

void run_dispatch_benchmarks() {
    ankerl::nanobench::Bench bench;
    bench.title("static dispatch parameter search");
    bench.minEpochTime(10ms);

    bench.run("dispatch hits", [] {
        int total = 0;
        for (auto [width, height] : dispatch_hits) { total += run_dispatch(width, height, 2); }
        ankerl::nanobench::doNotOptimizeAway(total);
    });

    bench.run("manual hits", [] {
        int total = 0;
        for (auto [width, height] : dispatch_hits) { total += run_manual(width, height, 2); }
        ankerl::nanobench::doNotOptimizeAway(total);
    });

    bench.run("dispatch misses", [] {
        int total = 0;
        for (auto [width, height] : dispatch_misses) { total += run_dispatch(width, height, 2); }
        ankerl::nanobench::doNotOptimizeAway(total);
    });

    bench.run("manual misses", [] {
        int total = 0;
        for (auto [width, height] : dispatch_misses) { total += run_manual(width, height, 2); }
        ankerl::nanobench::doNotOptimizeAway(total);
    });

    // Large multi-dimensional dispatch benchmark
    // We'll test a 5-dimensional dispatch with 4 options per-dimension
    // (total table size 4^5 = 1024). We compare a contiguous range
    // per-dimension vs a non-contiguous integer sequence with the same
    // cardinality to observe dispatch differences.
    {
        using dim_seq_contig = poet::make_range<0, 3>;// 0,1,2,3
        using dim_seq_noncontig = std::integer_sequence<int, 0, 10, 20, 30>;

        // build parameter tuple for 5 dimensions
        auto params_contig = std::make_tuple(poet::DispatchParam<dim_seq_contig>{ 0 },
          poet::DispatchParam<dim_seq_contig>{ 1 },
          poet::DispatchParam<dim_seq_contig>{ 2 },
          poet::DispatchParam<dim_seq_contig>{ 3 },
          poet::DispatchParam<dim_seq_contig>{ 0 });

        auto params_noncontig = std::make_tuple(poet::DispatchParam<dim_seq_noncontig>{ 0 },
          poet::DispatchParam<dim_seq_noncontig>{ 10 },
          poet::DispatchParam<dim_seq_noncontig>{ 20 },
          poet::DispatchParam<dim_seq_noncontig>{ 30 },
          poet::DispatchParam<dim_seq_noncontig>{ 0 });


        bench.run("big dispatch contiguous (5D x4)", [&] {
            int total = 0;
            for (int i = 0; i < 1000; ++i) { total += poet::dispatch(big_kernel{}, params_contig, 3); }
            ankerl::nanobench::doNotOptimizeAway(total);
        });

        bench.run("big dispatch non-contiguous (5D x4)", [&] {
            int total = 0;
            for (int i = 0; i < 1000; ++i) { total += poet::dispatch(big_kernel{}, params_noncontig, 3); }
            ankerl::nanobench::doNotOptimizeAway(total);
        });
    }
}
