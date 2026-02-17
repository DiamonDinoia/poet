#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>
using namespace std::chrono_literals;
#include <nanobench.h>

#include <poet/core/dynamic_for.hpp>

namespace {

constexpr std::size_t small_count = 64;
constexpr std::size_t large_count = 4'096;
constexpr std::size_t irregular_begin = 3;
constexpr std::size_t irregular_end = 515;
constexpr std::size_t default_unroll = 8;
constexpr std::size_t lanes = 8;

static inline std::uint64_t splitmix64(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

double manual_reduce_workload(std::size_t begin, std::size_t end) {
    std::array<double, lanes> acc{};
    acc.fill(0.0);
    for (std::size_t i = begin; i < end; ++i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        acc[i & (lanes - 1)] += x;
    }
    double out = 0.0;
    for (double v : acc) out += v;
    return out;
}

template<std::size_t Unroll>
double dynamic_for_workload(std::size_t begin, std::size_t end) {
    std::array<double, lanes> acc{};
    acc.fill(0.0);
    poet::dynamic_for<Unroll>(begin, end, [&](std::size_t i) {
        const std::uint64_t r = splitmix64(static_cast<std::uint64_t>(i));
        double x = static_cast<double>(r) * 5.42101086242752217e-20;
        x = std::fma(x, 1.0000001192092896, 0.3333333333333333);
        x = std::fma(x, 0.9999998807907104, 0.14285714285714285);
        x = std::fma(x, 1.0000000596046448, -0.0625);
        x = std::fma(x, 1.0000001192092896, 0.25);
        x = std::fma(x, 0.9999998807907104, -0.125);
        acc[i & (lanes - 1)] += x;
    });
    double out = 0.0;
    for (double v : acc) out += v;
    return out;
}

void benchmark_range(ankerl::nanobench::Bench &bench, std::size_t begin, std::size_t end, const char *label_suffix) {
    const std::string unroll8_label = std::string("dynamic_for ") + label_suffix + " (unroll 8)";
    bench.run(unroll8_label, [=] {
        const auto result = dynamic_for_workload<default_unroll>(begin, end);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    const std::string unroll16_label = std::string("dynamic_for ") + label_suffix + " (unroll 16)";
    bench.run(unroll16_label, [=] {
        const auto result = dynamic_for_workload<16>(begin, end);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    const std::string manual_label = std::string("for ") + label_suffix;
    bench.run(manual_label, [=] {
        const auto result = manual_reduce_workload(begin, end);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

}// namespace

void run_dynamic_for_benchmarks() {
    ankerl::nanobench::Bench bench;
    bench.title("dynamic_for vs for loop");
    bench.minEpochTime(10ms);

    benchmark_range(bench, 0, small_count, "small range");
    benchmark_range(bench, 0, large_count, "large range");
    benchmark_range(bench, irregular_begin, irregular_end, "irregular range");
}
