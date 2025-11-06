#include <array>
#include <tuple>
#include <utility>

#include <nanobench.h>

#include <poet/core/static_dispatch.hpp>

namespace {

using width_range = poet::make_range<1, 8>;
using height_range = poet::make_range<1, 8>;

constexpr std::array<std::pair<int, int>, 6> dispatch_hits{{
    {1, 1},
    {2, 3},
    {4, 5},
    {6, 2},
    {7, 8},
    {8, 4},
}};

constexpr std::array<std::pair<int, int>, 4> dispatch_misses{{
    {0, 0},
    {9, 1},
    {1, 9},
    {12, 12},
}};

struct matrix_kernel {
  template <int Width, int Height>
  [[nodiscard]] int operator()(int scale) const {
    // Pretend to perform work that benefits from compile-time specialization.
    return scale * (Width * Height + Width + Height);
  }
};

int run_manual(int width, int height, int scale) {
  if (width < 1 || width > 8 || height < 1 || height > 8) {
    return 0;
  }
  return scale * (width * height + width + height);
}

int run_dispatch(int width, int height, int scale) {
  auto params = std::make_tuple(
      poet::DispatchParam<width_range>{width},
      poet::DispatchParam<height_range>{height});
  return poet::dispatch(matrix_kernel{}, params, scale);
}

} // namespace

void run_dispatch_benchmarks() {
  ankerl::nanobench::Bench bench;
  bench.title("static dispatch parameter search");
  bench.minEpochIterations(10'000);

  bench.run("dispatch hits", [] {
    int total = 0;
    for (auto [width, height] : dispatch_hits) {
      total += run_dispatch(width, height, 2);
    }
    ankerl::nanobench::doNotOptimizeAway(total);
  });

  bench.run("manual hits", [] {
    int total = 0;
    for (auto [width, height] : dispatch_hits) {
      total += run_manual(width, height, 2);
    }
    ankerl::nanobench::doNotOptimizeAway(total);
  });

  bench.run("dispatch misses", [] {
    int total = 0;
    for (auto [width, height] : dispatch_misses) {
      total += run_dispatch(width, height, 2);
    }
    ankerl::nanobench::doNotOptimizeAway(total);
  });

  bench.run("manual misses", [] {
    int total = 0;
    for (auto [width, height] : dispatch_misses) {
      total += run_manual(width, height, 2);
    }
    ankerl::nanobench::doNotOptimizeAway(total);
  });
}
