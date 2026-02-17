#include "static_for_bench_support.hpp"

namespace poet_bench::static_for_support {

namespace {

template<std::intmax_t N, std::size_t BlockSize>
double static_for_dependent_ops_blocked(std::uint64_t s) {
    double acc = 0.0;
    poet::static_for<0, N, 1, BlockSize>(dependent_ops_functor{ acc, s });
    return acc;
}

} // namespace

void run_static_for_bench_blocksize(std::uint64_t s) {
    ankerl::nanobench::Bench bench;
    bench.title("4. BlockSize tuning: register pressure vs call overhead");
    bench.minEpochTime(std::chrono::milliseconds{10});
    bench.relative(true);

    // --- N = 32: medium loop ---
    constexpr std::intmax_t n32 = 32;
    run_case(bench, n32, "N=32 default (full)", [s]() { return static_for_dependent_ops<n32>(s); });
    run_case(bench, n32, "N=32 bs=4",  [s]() { return static_for_dependent_ops_blocked<n32, 4>(s); });
    run_case(bench, n32, "N=32 bs=8",  [s]() { return static_for_dependent_ops_blocked<n32, 8>(s); });
    run_case(bench, n32, "N=32 bs=16", [s]() { return static_for_dependent_ops_blocked<n32, 16>(s); });

    // --- N = 64: large loop ---
    constexpr std::intmax_t n64 = 64;
    run_case(bench, n64, "N=64 default (full)", [s]() { return static_for_dependent_ops<n64>(s); });
    run_case(bench, n64, "N=64 bs=4",  [s]() { return static_for_dependent_ops_blocked<n64, 4>(s); });
    run_case(bench, n64, "N=64 bs=8",  [s]() { return static_for_dependent_ops_blocked<n64, 8>(s); });
    run_case(bench, n64, "N=64 bs=16", [s]() { return static_for_dependent_ops_blocked<n64, 16>(s); });

    // --- N = 128: stress test ---
    constexpr std::intmax_t n128 = 128;
    run_case(bench, n128, "N=128 default (full)", [s]() { return static_for_dependent_ops<n128>(s); });
    run_case(bench, n128, "N=128 bs=4",  [s]() { return static_for_dependent_ops_blocked<n128, 4>(s); });
    run_case(bench, n128, "N=128 bs=8",  [s]() { return static_for_dependent_ops_blocked<n128, 8>(s); });
    run_case(bench, n128, "N=128 bs=16", [s]() { return static_for_dependent_ops_blocked<n128, 16>(s); });
}

} // namespace poet_bench::static_for_support
