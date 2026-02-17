#include "static_for_bench_support.hpp"

namespace poet_bench::static_for_support {

void run_static_for_bench_dependent(std::uint64_t s) {
    ankerl::nanobench::Bench bench;
    bench.title("1. Compile-time dependent operations (different constants per iteration)");
    bench.minEpochTime(std::chrono::milliseconds{ 10 });

    run_case(bench, 8, "runtime  8 iters (runtime coeff)", [s]() { return runtime_dependent_ops(8, s); });
    run_case(bench, 8, "static_for  8 iters (constexpr coeff)", [s]() { return static_for_dependent_ops<8>(s); });
    run_case(bench, 16, "runtime 16 iters (runtime coeff)", [s]() { return runtime_dependent_ops(16, s); });
    run_case(bench, 16, "static_for 16 iters (constexpr coeff)", [s]() { return static_for_dependent_ops<16>(s); });
}

}// namespace poet_bench::static_for_support
