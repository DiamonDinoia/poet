#include <cstddef>
#include <string>
#include <chrono>
using namespace std::chrono_literals;
#include <nanobench.h>

#include <poet/core/dynamic_for.hpp>
#include <poet/core/static_dispatch.hpp>

namespace {

// ============================================================================
// Test functors
// ============================================================================

struct simple_functor {
    int* result;

    template<int V>
    void operator()() const {
        *result += V;
    }
};

struct functor_2d {
    int* result;

    template<int V1, int V2>
    void operator()() const {
        *result += V1 * 100 + V2;
    }
};

// ============================================================================
// Switch-based dispatch implementations for testing
// ============================================================================

// Recursive switch generator for small ranges
template<int... Values>
struct switch_impl;

template<int V>
struct switch_impl<V> {
    template<typename Func>
    static void dispatch(int val, Func& func) {
        if (val == V) func.template operator()<V>();
    }
};

template<int First, int... Rest>
struct switch_impl<First, Rest...> {
    template<typename Func>
    static void dispatch(int val, Func& func) {
        if (val == First) {
            func.template operator()<First>();
        } else {
            switch_impl<Rest...>::dispatch(val, func);
        }
    }
};

// Helper to generate switch from sequence
template<typename Seq, typename Func>
struct switch_dispatcher;

template<int... Vals, typename Func>
struct switch_dispatcher<std::integer_sequence<int, Vals...>, Func> {
    static void dispatch(int val, Func& func) {
        switch_impl<Vals...>::dispatch(val, func);
    }
};

// ============================================================================
// Benchmark scenarios
// ============================================================================

// Scenario 1: Small range (4 elements) - Switch sweet spot
void bench_small_range_4(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int test_val = 2;

    bench.run("1D size=4: poet::dispatch (hybrid)", [&] {
        simple_functor func{&result};
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 3>>{test_val});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("1D size=4: manual switch", [&] {
        simple_functor func{&result};
        switch_dispatcher<poet::make_range<0, 3>, simple_functor>::dispatch(test_val, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 2: Medium range (16 elements) - Testing threshold
void bench_medium_range_16(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int test_val = 8;

    bench.run("1D size=16: poet::dispatch (hybrid)", [&] {
        simple_functor func{&result};
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 15>>{test_val});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("1D size=16: manual switch", [&] {
        simple_functor func{&result};
        switch_dispatcher<poet::make_range<0, 15>, simple_functor>::dispatch(test_val, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 3: Large range (64 elements) - Switch code bloat
void bench_large_range_64(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int test_val = 32;

    bench.run("1D size=64: poet::dispatch (fptr)", [&] {
        simple_functor func{&result};
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 63>>{test_val});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("1D size=64: manual switch", [&] {
        simple_functor func{&result};
        switch_dispatcher<poet::make_range<0, 63>, simple_functor>::dispatch(test_val, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 4: Very large range (256 elements) - Extreme case
void bench_very_large_range_256(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int test_val = 128;

    bench.run("1D size=256: poet::dispatch (fptr)", [&] {
        simple_functor func{&result};
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 255>>{test_val});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // Note: We won't test manual switch here - it would create massive code
    // and likely overflow compiler limits
}

// Scenario 5: 2D dispatch small (4x4) - Nested switches needed
void bench_2d_small_4x4(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int val1 = 2;
    constexpr int val2 = 2;

    bench.run("2D 4x4: poet::dispatch", [&] {
        functor_2d func{&result};
        poet::dispatch(func,
            poet::DispatchParam<poet::make_range<0, 3>>{val1},
            poet::DispatchParam<poet::make_range<0, 3>>{val2});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // Manual 2D switch would require nested switch - complex and verbose
}

// Scenario 6: 2D dispatch medium (8x8) - 64 cases
void bench_2d_medium_8x8(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int val1 = 4;
    constexpr int val2 = 4;

    bench.run("2D 8x8: poet::dispatch", [&] {
        functor_2d func{&result};
        poet::dispatch(func,
            poet::DispatchParam<poet::make_range<0, 7>>{val1},
            poet::DispatchParam<poet::make_range<0, 7>>{val2});
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 7: Sparse sequence - Switches can't optimize
void bench_sparse_sequence(ankerl::nanobench::Bench& bench) {
    int result = 0;
    using SparseSeq = std::integer_sequence<int, 1, 5, 10, 50, 100>;
    constexpr int test_val = 50;

    bench.run("Sparse sequence: poet::dispatch", [&] {
        simple_functor func{&result};
        poet::dispatch(func, poet::DispatchParam<SparseSeq>{test_val});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("Sparse sequence: manual switch", [&] {
        simple_functor func{&result};
        switch_dispatcher<SparseSeq, simple_functor>::dispatch(test_val, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 8: Branch prediction test (varying values)
void bench_unpredictable_pattern(ankerl::nanobench::Bench& bench) {
    int result = 0;
    std::array<int, 100> values;
    for (size_t i = 0; i < 100; ++i) {
        values[i] = static_cast<int>((i * 7 + 13) % 8);
    }

    bench.run("Unpredictable (size=8): poet::dispatch", [&] {
        simple_functor func{&result};
        for (int val : values) {
            poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 7>>{val});
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("Unpredictable (size=8): manual switch", [&] {
        simple_functor func{&result};
        for (int val : values) {
            switch_dispatcher<poet::make_range<0, 7>, simple_functor>::dispatch(val, func);
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 9: Different threshold values
template<int Threshold>
void bench_threshold_sweep(ankerl::nanobench::Bench& bench, const char* label) {
    int result = 0;
    constexpr int mid = Threshold / 2;

    std::string bench_label = std::string("Threshold sweep size=") + std::to_string(Threshold) +
                               ": " + label;

    bench.run(bench_label, [&] {
        simple_functor func{&result};
        if constexpr (Threshold == 4) {
            poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 3>>{mid});
        } else if constexpr (Threshold == 8) {
            poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 7>>{mid});
        } else if constexpr (Threshold == 12) {
            poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 11>>{mid});
        } else if constexpr (Threshold == 16) {
            poet::dispatch(func, poet::DispatchParam<poet::make_range<0, 15>>{mid});
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

} // namespace

void run_switch_vs_fptr_benchmarks() {
    ankerl::nanobench::Bench bench;
    bench.title("Switch vs Function Pointer Dispatch - Comprehensive Comparison");
    bench.minEpochTime(10ms);
    bench.relative(true);
    bench.performanceCounters(true);

    bench.run("====== 1D Dispatch: Small Range (4) ======", [] {});
    bench_small_range_4(bench);

    bench.run("====== 1D Dispatch: Medium Range (16) ======", [] {});
    bench_medium_range_16(bench);

    bench.run("====== 1D Dispatch: Large Range (64) ======", [] {});
    bench_large_range_64(bench);

    bench.run("====== 1D Dispatch: Very Large Range (256) ======", [] {});
    bench_very_large_range_256(bench);

    bench.run("====== 2D Dispatch: Small (4x4) ======", [] {});
    bench_2d_small_4x4(bench);

    bench.run("====== 2D Dispatch: Medium (8x8) ======", [] {});
    bench_2d_medium_8x8(bench);

    bench.run("====== Sparse Sequence ======", [] {});
    bench_sparse_sequence(bench);

    bench.run("====== Unpredictable Pattern ======", [] {});
    bench_unpredictable_pattern(bench);

    bench.run("====== Threshold Sensitivity Analysis ======", [] {});
    bench_threshold_sweep<4>(bench, "size=4");
    bench_threshold_sweep<8>(bench, "size=8");
    bench_threshold_sweep<12>(bench, "size=12");
    bench_threshold_sweep<16>(bench, "size=16");
}
