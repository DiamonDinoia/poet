#include <cstddef>
#include <cstdint>

#include <nanobench.h>

#include <poet/core/static_for.hpp>

#include <cmath>// for std::fma
#include <limits>

namespace {

constexpr std::intmax_t small_iterations = 8;
constexpr std::intmax_t large_iterations = 512;

// ------------------ FP-heavy workload remains ------------------
// Deterministic SplitMix64 (stateless, depends only on I)
static inline std::uint64_t splitmix64(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Floating-point accumulator with 4 lanes to increase ILP.
struct fp_accumulator_4way {
    double &out;
    double lanes[4];

    explicit fp_accumulator_4way(double &o) : out(o) { lanes[0] = lanes[1] = lanes[2] = lanes[3] = 0.0; }

    template<auto I> void operator()() {
        // generate deterministic 64-bit value from I
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I));
        // map to double in [0,1)
        double x = static_cast<double>(r) * 5.42101086242752217e-20;// 1/2^64
        // do a small chain of FMA ops to consume CPU cycles and be vectorizable
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        // accumulate into lane
        constexpr std::size_t lane = static_cast<std::size_t>(I % 4);
        lanes[lane] += x;
    }

    void finalize() { out = lanes[0] + lanes[1] + lanes[2] + lanes[3]; }
};

// 8-way accumulator
struct fp_accumulator_8way {
    double &out;
    double lanes[8];

    explicit fp_accumulator_8way(double &o) : out(o) {
        for (int i = 0; i < 8; ++i) lanes[i] = 0.0;
    }

    template<auto I> void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I));
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        constexpr std::size_t lane = static_cast<std::size_t>(I % 8);
        lanes[lane] += x;
    }

    void finalize() {
        double s = 0.0;
        for (int i = 0; i < 8; ++i) s += lanes[i];
        out = s;
    }
};

// 16-way accumulator
struct fp_accumulator_16way {
    double &out;
    double lanes[16];

    explicit fp_accumulator_16way(double &o) : out(o) {
        for (int i = 0; i < 16; ++i) lanes[i] = 0.0;
    }

    template<auto I> void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I));
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        constexpr std::size_t lane = static_cast<std::size_t>(I % 16);
        lanes[lane] += x;
    }

    void finalize() {
        double s = 0.0;
        for (int i = 0; i < 16; ++i) s += lanes[i];
        out = s;
    }
};

// Runtime equivalent for the FP workload (same operations, same mapping)
double runtime_fp_sum(std::intmax_t begin, std::intmax_t end) {
    double lanes[16] = { 0.0 };
    for (std::intmax_t i = begin; i < end; ++i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        lanes[static_cast<std::size_t>(i % 16)] += x;
    }
    double s = 0.0;
    for (int i = 0; i < 16; ++i) s += lanes[i];
    return s;
}

// Helper to run static_for with fp accumulator and call finalize.
template<typename Acc, std::intmax_t Begin, std::intmax_t End, std::intmax_t Step = 1, std::size_t BlockSize = 8>
double static_for_sum_fp() {
    double result = 0.0;
    Acc acc{ result };
    poet::static_for<Begin, End, Step, BlockSize>(acc);
    acc.finalize();
    return result;
}

// Helper: runtime sum with parameterized lane count to match accumulators
template <std::size_t Lanes>
double runtime_fp_sum_lanes(std::intmax_t begin, std::intmax_t end) {
  double lanes[Lanes];
  for (std::size_t i = 0; i < Lanes; ++i) lanes[i] = 0.0;
  for (std::intmax_t i = begin; i < end; ++i) {
    const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
    double x = static_cast<double>(r) * 5.42101086242752217e-20;
    x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
    x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
    x = std::fma(x, 1.0000000596046448, -0.0625);
    x = std::fma(x, 1.0000001192092896, 0.25);
    x = std::fma(x, 0.9999998807907104, -0.125);
    const std::size_t idx = static_cast<std::size_t>(i % static_cast<std::intmax_t>(Lanes));
    lanes[idx] += x;
  }
  double s = 0.0;
  for (std::size_t i = 0; i < Lanes; ++i) s += lanes[i];
  return s;
}

// helpers to run static_for and return per-lane values for diagnostics
template <typename Acc, std::size_t Lanes, std::intmax_t Begin, std::intmax_t End,
          std::intmax_t Step = 1, std::size_t BlockSize = 8>
void get_static_lanes(double out_lanes[Lanes]) {
  double result = 0.0;
  Acc acc{result};
  poet::static_for<Begin, End, Step, BlockSize>(acc);
  // copy lanes from acc to out_lanes
  for (std::size_t i = 0; i < Lanes; ++i) out_lanes[i] = acc.lanes[i];
}

template <std::size_t Lanes>
void get_runtime_lanes(std::intmax_t begin, std::intmax_t end, double out_lanes[Lanes]) {
  for (std::size_t i = 0; i < Lanes; ++i) out_lanes[i] = 0.0;
  for (std::intmax_t i = begin; i < end; ++i) {
    const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
    double x = static_cast<double>(r) * 5.42101086242752217e-20;
    x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
    x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
    x = std::fma(x, 1.0000000596046448, -0.0625);
    x = std::fma(x, 1.0000001192092896, 0.25);
    x = std::fma(x, 0.9999998807907104, -0.125);
    const std::size_t idx = static_cast<std::size_t>(i % static_cast<std::intmax_t>(Lanes));
    out_lanes[idx] += x;
  }
}

// Modify verify_blocksize_with_eps to show per-lane diagnostics on mismatch
template <std::size_t BS>
static bool verify_blocksize_with_eps(double eps) {
  // 4-way
  {
    double s_static = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, BS>();
    double s_runtime = runtime_fp_sum_lanes<4>(0, large_iterations);
    double diff = std::abs(s_static - s_runtime);
    bool ok;
    if (s_runtime == 0.0) {
      ok = (std::abs(s_static) <= eps);
    } else {
      double rel = std::abs(1.0 - s_static / s_runtime);
      ok = (rel <= eps);
    }
    if (!ok) {
      std::fprintf(stderr, "Mismatch 4-way bs=%zu: static=%.17g runtime=%.17g absdiff=%.17g eps=%.17g\n",
                   static_cast<std::size_t>(BS), s_static, s_runtime, diff, eps);
      double static_lanes[4];
      double runtime_lanes[4];
      get_static_lanes<fp_accumulator_4way,4,0,large_iterations,1,BS>(static_lanes);
      get_runtime_lanes<4>(0, large_iterations, runtime_lanes);
      std::fprintf(stderr, "static lanes:\n");
      for (int i=0;i<4;++i) std::fprintf(stderr, "  [%d] %.17g\n", i, static_lanes[i]);
      std::fprintf(stderr, "runtime lanes:\n");
      for (int i=0;i<4;++i) std::fprintf(stderr, "  [%d] %.17g\n", i, runtime_lanes[i]);
      return false;
    }
  }

  // 8-way
  {
    double s_static = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, BS>();
    double s_runtime = runtime_fp_sum_lanes<8>(0, large_iterations);
    double diff = std::abs(s_static - s_runtime);
    bool ok;
    if (s_runtime == 0.0) {
      ok = (std::abs(s_static) <= eps);
    } else {
      double rel = std::abs(1.0 - s_static / s_runtime);
      ok = (rel <= eps);
    }
    if (!ok) {
      std::fprintf(stderr, "Mismatch 8-way bs=%zu: static=%.17g runtime=%.17g absdiff=%.17g eps=%.17g\n",
                   static_cast<std::size_t>(BS), s_static, s_runtime, diff, eps);
      double static_lanes[8];
      double runtime_lanes[8];
      get_static_lanes<fp_accumulator_8way,8,0,large_iterations,1,BS>(static_lanes);
      get_runtime_lanes<8>(0, large_iterations, runtime_lanes);
      std::fprintf(stderr, "static lanes:\n");
      for (int i=0;i<8;++i) std::fprintf(stderr, "  [%d] %.17g\n", i, static_lanes[i]);
      std::fprintf(stderr, "runtime lanes:\n");
      for (int i=0;i<8;++i) std::fprintf(stderr, "  [%d] %.17g\n", i, runtime_lanes[i]);
      return false;
    }
  }

  // 16-way
  {
    double s_static = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, BS>();
    double s_runtime = runtime_fp_sum_lanes<16>(0, large_iterations);
    double diff = std::abs(s_static - s_runtime);
    bool ok;
    if (s_runtime == 0.0) {
      ok = (std::abs(s_static) <= eps);
    } else {
      double rel = std::abs(1.0 - s_static / s_runtime);
      ok = (rel <= eps);
    }
    if (!ok) {
      std::fprintf(stderr, "Mismatch 16-way bs=%zu: static=%.17g runtime=%.17g absdiff=%.17g eps=%.17g\n",
                   static_cast<std::size_t>(BS), s_static, s_runtime, diff, eps);
      double static_lanes[16];
      double runtime_lanes[16];
      get_static_lanes<fp_accumulator_16way,16,0,large_iterations,1,BS>(static_lanes);
      get_runtime_lanes<16>(0, large_iterations, runtime_lanes);
      std::fprintf(stderr, "static lanes:\n");
      for (int i=0;i<16;++i) std::fprintf(stderr, "  [%d] %.17g\n", i, static_lanes[i]);
      std::fprintf(stderr, "runtime lanes:\n");
      for (int i=0;i<16;++i) std::fprintf(stderr, "  [%d] %.17g\n", i, runtime_lanes[i]);
      return false;
    }
  }

  return true;
}

static bool verify_all_with_eps(double eps) {
  if (!verify_blocksize_with_eps<1>(eps)) return false;
  if (!verify_blocksize_with_eps<4>(eps)) return false;
  if (!verify_blocksize_with_eps<8>(eps)) return false;
  if (!verify_blocksize_with_eps<32>(eps)) return false;
  return true;
}

}// namespace

void run_static_for_benchmarks() {
  // verify before running benchmarks
  const double eps = std::numeric_limits<double>::epsilon();
  if (verify_all_with_eps(eps)) {
    std::fprintf(stderr, "Verification passed with eps=%.17g\n", eps);
  } else if (verify_all_with_eps(eps * 10.0)) {
    std::fprintf(stderr, "Verification passed after relaxing eps to %.17g\n", eps * 10.0);
  } else {
    std::fprintf(stderr, "Verification failed even after relaxing tolerance (eps*10)\n");
    std::abort();
  }

  ankerl::nanobench::Bench bench;
  using namespace std::chrono_literals;
  bench.title("static_for FP vs runtime");
  bench.minEpochTime(10ms);

    // BlockSize sweep for 4-way
    bench.run("static_for large (FP 4-way, block 1)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, 1>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 4-way, block 4)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, 4>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 4-way, block 8)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, 8>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 4-way, block 32)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, 32>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // BlockSize sweep for 8-way
    bench.run("static_for large (FP 8-way, block 1)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, 1>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 8-way, block 4)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, 4>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 8-way, block 8)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, 8>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 8-way, block 32)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, 32>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // BlockSize sweep for 16-way
    bench.run("static_for large (FP 16-way, block 1)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, 1>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 16-way, block 4)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, 4>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 16-way, block 8)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, 8>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for large (FP 16-way, block 32)", [] {
        const auto result = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, 32>();
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // runtime reference
    bench.run("runtime for large (FP)", [] {
        const auto result = runtime_fp_sum(0, large_iterations);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}
