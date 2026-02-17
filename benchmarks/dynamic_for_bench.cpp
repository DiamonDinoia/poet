#include <cstddef>
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
constexpr std::size_t min_epoch_iterations = 5'000;

auto manual_sum(std::size_t begin, std::size_t end) -> int {
    int total = 0;
    for (std::size_t i = begin; i < end; ++i) { total += static_cast<int>(i); }
    return total;
}

template<std::size_t Unroll = default_unroll> auto dynamic_for_sum(std::size_t begin, std::size_t end) -> int {
    int total = 0;
    poet::dynamic_for<Unroll>(begin, end, [&](std::size_t index) { total += static_cast<int>(index); });
    return total;
}

void benchmark_range(ankerl::nanobench::Bench &bench, std::size_t begin, std::size_t end, const char *label_suffix) {
    const std::string unroll8_label = std::string("dynamic_for ") + label_suffix + " (unroll 8)";
    bench.run(unroll8_label, [=] {
        const auto result = dynamic_for_sum(begin, end);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    const std::string unroll16_label = std::string("dynamic_for ") + label_suffix + " (unroll 16)";
    bench.run(unroll16_label, [=] {
        const auto result = dynamic_for_sum<16>(begin, end);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    const std::string manual_label = std::string("for ") + label_suffix;
    bench.run(manual_label, [=] {
        const auto result = manual_sum(begin, end);
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
