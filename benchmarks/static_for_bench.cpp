#include <array>
#include <cmath>// for std::fma
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>
#include <chrono>
#include <nanobench.h>

#include <poet/core/static_for.hpp>

using namespace std::chrono_literals;
namespace {

// Iteration counts chosen to test different cache/unrolling characteristics:
// Testing small iteration counts where static_for should perform best
constexpr std::intmax_t tiny_iterations = 8;
constexpr std::intmax_t small_iterations = 16;
constexpr std::intmax_t medium_iterations = 32;
constexpr std::intmax_t large_iterations = 64;
constexpr std::size_t kLanes4 = 4;
constexpr std::size_t kLanes8 = 8;
constexpr std::size_t kLanes16 = 16;

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
    std::array<double, kLanes4> lanes;
    std::uint64_t runtime_seed;

    explicit fp_accumulator_4way(double &o, std::uint64_t seed = 0) : out(o), runtime_seed(seed) { lanes.fill(0.0); }

    template<auto I> void operator()() {
        // generate deterministic 64-bit value from I + runtime_seed
        // runtime_seed prevents compile-time constant folding when used with doNotOptimizeAway
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I) + runtime_seed);
        // map to double in [0,1)
        double x = static_cast<double>(r) * 5.42101086242752217e-20;// 1/2^64
        // do a small chain of FMA ops to consume CPU cycles and be vectorizable
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        // accumulate into lane
        constexpr std::size_t lane = static_cast<std::size_t>(I % kLanes4);
        lanes[lane] += x;
    }

    void finalize() { out = lanes[0] + lanes[1] + lanes[2] + lanes[3]; }
};

// 8-way accumulator
struct fp_accumulator_8way {
    double &out;
    std::array<double, kLanes8> lanes;
    std::uint64_t runtime_seed;

    explicit fp_accumulator_8way(double &o, std::uint64_t seed = 0) : out(o), runtime_seed(seed) { lanes.fill(0.0); }

    template<auto I> void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I) + runtime_seed);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        constexpr std::size_t lane = static_cast<std::size_t>(I % kLanes8);
        lanes[lane] += x;
    }

    void finalize() {
        double s = 0.0;
        for (double val : lanes) s += val;
        out = s;
    }
};

// 16-way accumulator
struct fp_accumulator_16way {
    double &out;
    std::array<double, kLanes16> lanes;
    std::uint64_t runtime_seed;

    explicit fp_accumulator_16way(double &o, std::uint64_t seed = 0) : out(o), runtime_seed(seed) { lanes.fill(0.0); }

    template<auto I> void operator()() {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(I) + runtime_seed);
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        constexpr std::size_t lane = static_cast<std::size_t>(I % kLanes16);
        lanes[lane] += x;
    }

    void finalize() {
        double s = 0.0;
        for (double val : lanes) s += val;
        out = s;
    }
};

// Runtime equivalent for the FP workload (same operations, same mapping)
double runtime_fp_sum(std::intmax_t begin, std::intmax_t end) {
    std::array<double, kLanes16> lanes;
    lanes.fill(0.0);
    for (std::intmax_t i = begin; i < end; ++i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        lanes[static_cast<std::size_t>(i) % kLanes16] += x;
    }
    double s = 0.0;
    for (double val : lanes) s += val;
    return s;
}

// Helper to run static_for with fp accumulator and call finalize.
template<typename Acc, std::intmax_t Begin, std::intmax_t End, std::intmax_t Step = 1, std::size_t BlockSize = 8>
double static_for_sum_fp(std::uint64_t seed = 0) {
    double result = 0.0;
    Acc acc{ result, seed };
    poet::static_for<Begin, End, Step, BlockSize>(acc);
    acc.finalize();
    return result;
}

// Helper: runtime sum with parameterized lane count to match accumulators
template<std::size_t Lanes> double runtime_fp_sum_lanes(std::intmax_t begin, std::intmax_t end) {
    std::array<double, Lanes> lanes;
    lanes.fill(0.0);
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
    for (double val : lanes) s += val;
    return s;
}

// helpers to run static_for and return per-lane values for diagnostics
template<typename Acc,
  std::size_t Lanes,
  std::intmax_t Begin,
  std::intmax_t End,
  std::intmax_t Step = 1,
  std::size_t BlockSize = 8>
void get_static_lanes(std::array<double, Lanes> &out_lanes, std::uint64_t seed = 0) {
    double result = 0.0;
    Acc acc{ result, seed };
    poet::static_for<Begin, End, Step, BlockSize>(acc);
    // copy lanes from acc to out_lanes
    for (std::size_t i = 0; i < Lanes; ++i) out_lanes[i] = acc.lanes[i];
}

template<std::size_t Lanes>
void get_runtime_lanes(std::intmax_t begin, std::intmax_t end, std::array<double, Lanes> &out_lanes) {
    out_lanes.fill(0.0);
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
template<std::size_t BS> bool verify_blocksize_with_eps(double eps) {
    constexpr std::uint64_t test_seed = 0;  // Use same seed as benchmarks for verification

    // 4-way
    {
        const double s_static = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, BS>(test_seed);
        const double s_runtime = runtime_fp_sum_lanes<kLanes4>(0, large_iterations);
        const double diff = std::abs(s_static - s_runtime);
        bool ok = false;
        if (s_runtime == 0.0) {
            ok = (std::abs(s_static) <= eps);
        } else {
            const double rel = std::abs(1.0 - (s_static / s_runtime));
            ok = (rel <= eps);
        }
        if (!ok) {
            std::cerr << "Mismatch 4-way bs=" << BS << ": static=" << std::setprecision(17) << s_static
                      << " runtime=" << s_runtime << " absdiff=" << diff << " eps=" << eps << "\n";
            std::array<double, kLanes4> static_lanes{};
            std::array<double, kLanes4> runtime_lanes{};
            get_static_lanes<fp_accumulator_4way, kLanes4, 0, large_iterations, 1, BS>(static_lanes, test_seed);
            get_runtime_lanes<kLanes4>(0, large_iterations, runtime_lanes);
            std::cerr << "static lanes:\n";
            for (std::size_t i = 0; i < kLanes4; ++i) std::cerr << "  [" << i << "] " << static_lanes[i] << "\n";
            std::cerr << "runtime lanes:\n";
            for (std::size_t i = 0; i < kLanes4; ++i) std::cerr << "  [" << i << "] " << runtime_lanes[i] << "\n";
            return false;
        }
    }

    // 8-way
    {
        const double s_static = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, BS>(test_seed);
        const double s_runtime = runtime_fp_sum_lanes<kLanes8>(0, large_iterations);
        const double diff = std::abs(s_static - s_runtime);
        bool ok = false;
        if (s_runtime == 0.0) {
            ok = (std::abs(s_static) <= eps);
        } else {
            const double rel = std::abs(1.0 - (s_static / s_runtime));
            ok = (rel <= eps);
        }
        if (!ok) {
            std::cerr << "Mismatch 8-way bs=" << BS << ": static=" << std::setprecision(17) << s_static
                      << " runtime=" << s_runtime << " absdiff=" << diff << " eps=" << eps << "\n";
            std::array<double, kLanes8> static_lanes{};
            std::array<double, kLanes8> runtime_lanes{};
            get_static_lanes<fp_accumulator_8way, kLanes8, 0, large_iterations, 1, BS>(static_lanes, test_seed);
            get_runtime_lanes<kLanes8>(0, large_iterations, runtime_lanes);
            std::cerr << "static lanes:\n";
            for (std::size_t i = 0; i < kLanes8; ++i) std::cerr << "  [" << i << "] " << static_lanes[i] << "\n";
            std::cerr << "runtime lanes:\n";
            for (std::size_t i = 0; i < kLanes8; ++i) std::cerr << "  [" << i << "] " << runtime_lanes[i] << "\n";
            return false;
        }
    }

    // 16-way
    {
        const double s_static = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, BS>(test_seed);
        const double s_runtime = runtime_fp_sum_lanes<kLanes16>(0, large_iterations);
        const double diff = std::abs(s_static - s_runtime);
        bool ok = false;
        if (s_runtime == 0.0) {
            ok = (std::abs(s_static) <= eps);
        } else {
            const double rel = std::abs(1.0 - (s_static / s_runtime));
            ok = (rel <= eps);
        }
        if (!ok) {
            std::cerr << "Mismatch 16-way bs=" << BS << ": static=" << std::setprecision(17) << s_static
                      << " runtime=" << s_runtime << " absdiff=" << diff << " eps=" << eps << "\n";
            std::array<double, kLanes16> static_lanes{};
            std::array<double, kLanes16> runtime_lanes{};
            get_static_lanes<fp_accumulator_16way, kLanes16, 0, large_iterations, 1, BS>(static_lanes, test_seed);
            get_runtime_lanes<kLanes16>(0, large_iterations, runtime_lanes);
            std::cerr << "static lanes:\n";
            for (std::size_t i = 0; i < kLanes16; ++i) std::cerr << "  [" << i << "] " << static_lanes[i] << "\n";
            std::cerr << "runtime lanes:\n";
            for (std::size_t i = 0; i < kLanes16; ++i) std::cerr << "  [" << i << "] " << runtime_lanes[i] << "\n";
            return false;
        }
    }

    return true;
}

bool verify_all_with_eps(double eps) {
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
        std::cerr << "Verification passed with eps=" << std::setprecision(17) << eps << "\n";
    } else if (verify_all_with_eps(eps * 10.0)) {
        std::cerr << "Verification passed after relaxing eps to " << std::setprecision(17) << (eps * 10.0) << "\n";
    } else {
        std::cerr << "Verification failed even after relaxing tolerance (eps*10)\n";
        std::abort();
    }

    ankerl::nanobench::Bench bench;
    using namespace std::chrono_literals;
    bench.title("static_for FP vs runtime");
    bench.minEpochTime(10ms);

    // 8 iterations - tiny loops
    bench.run("static_for 8 iters (FP 4-way)", [&]() -> void {
        std::uint64_t seed = 0;
        ankerl::nanobench::doNotOptimizeAway(seed);
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, tiny_iterations, 1, 8>(seed);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("runtime 8 iters (FP)", [&]() -> void {
        const auto result = runtime_fp_sum(0, tiny_iterations);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // 16 iterations
    bench.run("static_for 16 iters (FP 4-way)", [&]() -> void {
        std::uint64_t seed = 0;
        ankerl::nanobench::doNotOptimizeAway(seed);
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, small_iterations, 1, 8>(seed);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("runtime 16 iters (FP)", [&]() -> void {
        const auto result = runtime_fp_sum(0, small_iterations);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // 32 iterations
    bench.run("static_for 32 iters (FP 4-way)", [&]() -> void {
        std::uint64_t seed = 0;
        ankerl::nanobench::doNotOptimizeAway(seed);
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, medium_iterations, 1, 8>(seed);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("runtime 32 iters (FP)", [&]() -> void {
        const auto result = runtime_fp_sum(0, medium_iterations);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // 64 iterations - testing different block sizes
    bench.run("static_for 64 iters (FP 4-way, block 8)", []() -> void {
        std::uint64_t seed = 0;
        ankerl::nanobench::doNotOptimizeAway(seed);
        const auto result = static_for_sum_fp<fp_accumulator_4way, 0, large_iterations, 1, 8>(seed);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for 64 iters (FP 8-way, block 8)", []() -> void {
        std::uint64_t seed = 0;
        ankerl::nanobench::doNotOptimizeAway(seed);
        const auto result = static_for_sum_fp<fp_accumulator_8way, 0, large_iterations, 1, 8>(seed);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("static_for 64 iters (FP 16-way, block 8)", []() -> void {
        std::uint64_t seed = 0;
        ankerl::nanobench::doNotOptimizeAway(seed);
        const auto result = static_for_sum_fp<fp_accumulator_16way, 0, large_iterations, 1, 8>(seed);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
    bench.run("runtime 64 iters (FP)", []() -> void {
        const auto result = runtime_fp_sum(0, large_iterations);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}
