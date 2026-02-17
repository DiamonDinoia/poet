#include "static_for_bench_support.hpp"

int main() {
    std::uint64_t s = poet_bench::static_for_support::seed;
    ankerl::nanobench::doNotOptimizeAway(s);

    poet_bench::static_for_support::run_static_for_bench_dependent(s);
    poet_bench::static_for_support::run_static_for_bench_simple(s);
    poet_bench::static_for_support::run_static_for_bench_unroll(s);

    return 0;
}
