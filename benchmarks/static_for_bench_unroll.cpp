#include "static_for_bench_support.hpp"

namespace poet_bench::static_for_support {

void run_static_for_bench_unroll(std::uint32_t s) {
    constexpr std::intmax_t num_iters = 64;
    ankerl::nanobench::Bench bench;
    bench.title("3. Effect of unrolling on ILP (64 iterations)");
    bench.minEpochTime(std::chrono::milliseconds{ 10 });

    run_case(bench, static_cast<std::uint64_t>(num_iters), "no unroll (bs=1)", [s]() {
        return runtime_unroll<1>(num_iters, s);
    });
    run_case(bench, static_cast<std::uint64_t>(num_iters), "manual unroll 4x", [s]() {
        return runtime_unroll<4>(num_iters, s);
    });
    run_case(bench, static_cast<std::uint64_t>(num_iters), "static_for bs=1", [s]() {
        return static_for_unrolled<num_iters, 1>(s);
    });
    run_case(bench, static_cast<std::uint64_t>(num_iters), "static_for bs=4", [s]() {
        return static_for_unrolled<num_iters, 4>(s);
    });
    run_case(bench, static_cast<std::uint64_t>(num_iters), "static_for bs=8", [s]() {
        return static_for_unrolled<num_iters, 8>(s);
    });
    run_case(bench, static_cast<std::uint64_t>(num_iters), "static_for bs=16", [s]() {
        return static_for_unrolled<num_iters, 16>(s);
    });
}

}// namespace poet_bench::static_for_support
