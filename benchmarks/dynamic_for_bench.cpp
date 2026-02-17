#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <nanobench.h>

#include <poet/core/dynamic_for.hpp>

using namespace std::chrono_literals;

namespace {

constexpr std::size_t small_count = 1000;
constexpr std::size_t large_count = 5000;
constexpr std::size_t irregular_begin = 3;
constexpr std::size_t irregular_end = 515;
constexpr std::size_t default_unroll = 8;
constexpr std::uint64_t salt_increment = 0x9e3779b97f4a7c15ULL;
volatile std::uint64_t benchmark_salt = 1;

// Hash function to generate pseudo-random workload per iteration
static inline std::uint64_t splitmix64(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Compute a workload with latency (FMA chain) - demonstrates ILP benefit when unrolled
static inline double compute_work(std::size_t i) noexcept {
    const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
    double x = static_cast<double>(r) * 5.42101086242752217e-20;
    // Chain of FMAs creates latency bottleneck in serial execution
    x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
    x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
    x = std::fma(x, 1.0000000596046448, -0.0625);
    x = std::fma(x, 1.0000001192092896, 0.25);
    x = std::fma(x, 0.9999998807907104, -0.125);
    return x;
}

// Baseline: naive for loop with loop-carried dependency
// Each iteration's accumulation depends on the previous sum -> limits ILP
double serial_loop(std::size_t begin, std::size_t end, std::size_t salt) {
    double sum = 0.0;
    for (std::size_t i = begin; i < end; ++i) {
        sum += compute_work(i + salt);  // Dependency chain: sum_i = sum_{i-1} + work(i)
    }
    return sum;
}

// Optimized: dynamic_for automatically unrolls to boost ILP
// The unrolling breaks the dependency chain by exposing multiple independent operations
// Compiler can interleave FMA chains from different iterations -> higher throughput
template<std::size_t Unroll>
double dynamic_for_loop(std::size_t begin, std::size_t end, std::size_t salt) {
    double sum = 0.0;
    poet::dynamic_for<Unroll>(begin, end, [&sum, salt](std::size_t i) {
        sum += compute_work(i + salt);  // Same code, but unrolling enables parallel execution
    });
    return sum;
}

template<std::size_t NumAccs>
double serial_loop_multi_acc(std::size_t begin, std::size_t end, std::size_t salt) {
    std::array<double, NumAccs> sums{};
    for (std::size_t i = begin; i < end; ++i) {
        sums[i % NumAccs] += compute_work(i + salt);
    }
    double total = 0.0;
    for (double v : sums) {
        total += v;
    }
    return total;
}

template<std::size_t Unroll, std::size_t NumAccs>
double dynamic_for_loop_multi_acc(std::size_t begin, std::size_t end, std::size_t salt) {
    std::array<double, NumAccs> sums{};
    poet::dynamic_for<Unroll>(begin, end, [&sums, salt](auto lane_c, std::size_t i) {
        constexpr std::size_t lane = decltype(lane_c)::value;
        constexpr std::size_t acc_idx = lane % NumAccs;
        sums[acc_idx] += compute_work(i + salt);
    });
    double total = 0.0;
    for (double v : sums) {
        total += v;
    }
    return total;
}

inline std::uint64_t next_salt() noexcept {
    const auto salt = benchmark_salt;
    benchmark_salt += salt_increment;
    return salt;
}

template<typename ComputeFn>
void run_benchmark_case(
    ankerl::nanobench::Bench &bench,
    std::size_t begin,
    std::size_t end,
    const std::string &label,
    ComputeFn &&compute) {
    auto compute_fn = std::forward<ComputeFn>(compute);
    bench.batch(end - begin).run(label, [begin, end, compute_fn = std::move(compute_fn)]() mutable {
        auto b = begin;
        auto e = end;
        const auto salt = next_salt();
        ankerl::nanobench::doNotOptimizeAway(b);
        ankerl::nanobench::doNotOptimizeAway(e);
        ankerl::nanobench::doNotOptimizeAway(salt);
        const auto result = compute_fn(b, e, salt);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

void benchmark_range(ankerl::nanobench::Bench &bench, std::size_t begin, std::size_t end, const std::string &label) {
    run_benchmark_case(bench, begin, end, std::string("for loop ") + label, [](auto b, auto e, auto salt) {
        return serial_loop(b, e, salt);
    });
    run_benchmark_case(bench, begin, end, std::string("dynamic_for ") + label, [](auto b, auto e, auto salt) {
        return dynamic_for_loop<default_unroll>(b, e, salt);
    });
    run_benchmark_case(bench, begin, end, std::string("for loop multi-acc4 ") + label, [](auto b, auto e, auto salt) {
        return serial_loop_multi_acc<4>(b, e, salt);
    });
    run_benchmark_case(bench, begin, end, std::string("dynamic_for lane multi-acc4 ") + label, [](auto b, auto e, auto salt) {
        return dynamic_for_loop_multi_acc<default_unroll, 4>(b, e, salt);
    });
}

}// namespace

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("dynamic_for: ILP boost via automatic unrolling");
    bench.minEpochTime(10ms);

    benchmark_range(bench, 0, small_count, std::string("small ") + std::to_string(small_count));
    benchmark_range(bench, 0, large_count, std::string("large ") + std::to_string(large_count));
    benchmark_range(bench, irregular_begin, irregular_end, std::string("irregular (") + std::to_string(irregular_begin) + "-" + std::to_string(irregular_end) + ")");

    return 0;
}
