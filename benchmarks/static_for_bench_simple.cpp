#include "static_for_bench_support.hpp"

namespace poet_bench::static_for_support {

void run_static_for_bench_simple(std::uint64_t s) {
    ankerl::nanobench::Bench bench;
    bench.title("2. Simple accumulation (baseline - compiler can auto-unroll both)");
    bench.minEpochTime(std::chrono::milliseconds{ 10 });

    run_case(bench, 8, "runtime  8 iters", [s]() { return runtime_simple(8, s); });
    run_case(bench, 8, "static_for  8 iters", [s]() { return static_for_simple<8>(s); });
    run_case(bench, 16, "runtime 16 iters", [s]() { return runtime_simple(16, s); });
    run_case(bench, 16, "static_for 16 iters", [s]() { return static_for_simple<16>(s); });
}

}// namespace poet_bench::static_for_support
