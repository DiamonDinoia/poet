#include <cstddef>
#include <cstdint>

#include <nanobench.h>

#include <poet/core/static_for.hpp>

namespace {

constexpr std::intmax_t small_iterations = 8;
constexpr std::intmax_t large_iterations = 512;

struct runtime_accumulator {
  static int sum(std::intmax_t begin, std::intmax_t end) {
    int result = 0;
    for (std::intmax_t i = begin; i < end; ++i) {
      result += static_cast<int>(i);
    }
    return result;
  }
};

struct static_accumulator {
  int &result;

  template <auto I>
  void operator()() const {
    result += static_cast<int>(I);
  }
};

template <std::intmax_t Begin, std::intmax_t End, std::intmax_t Step = 1,
          std::size_t BlockSize = 8>
int static_for_sum() {
  int result = 0;
  static_accumulator accumulator{result};
  poet::static_for<Begin, End, Step, BlockSize>(accumulator);
  return result;
}

} // namespace

void run_static_for_benchmarks() {
  ankerl::nanobench::Bench bench;
  bench.title("static_for vs runtime");
  bench.minEpochIterations(10'000);

  bench.run("static_for small (default block)", [] {
    const auto result = static_for_sum<0, small_iterations>();
    ankerl::nanobench::doNotOptimizeAway(result);
  });

  bench.run("static_for small (block size 4)", [] {
    const auto result = static_for_sum<0, small_iterations, 1, 4>();
    ankerl::nanobench::doNotOptimizeAway(result);
  });

  bench.run("runtime for small", [] {
    const auto result = runtime_accumulator::sum(0, small_iterations);
    ankerl::nanobench::doNotOptimizeAway(result);
  });

  bench.run("static_for large (default block)", [] {
    const auto result = static_for_sum<0, large_iterations>();
    ankerl::nanobench::doNotOptimizeAway(result);
  });

  bench.run("static_for large (block size 32)", [] {
    const auto result = static_for_sum<0, large_iterations, 1, 32>();
    ankerl::nanobench::doNotOptimizeAway(result);
  });

  bench.run("runtime for large", [] {
    const auto result = runtime_accumulator::sum(0, large_iterations);
    ankerl::nanobench::doNotOptimizeAway(result);
  });
}
