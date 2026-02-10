#include <cstddef>
#include <string>
#include <chrono>
using namespace std::chrono_literals;
#include <nanobench.h>

#include <poet/core/static_dispatch.hpp>

namespace {

// ============================================================================
// Test functors
// ============================================================================

struct simple_functor {
    int* result;

    template<int N>
    POET_FORCEINLINE void operator()() const {
        *result += N;
    }

    POET_FORCEINLINE void operator()(std::integral_constant<int, 0>) const { *result += 0; }
    POET_FORCEINLINE void operator()(std::integral_constant<int, 1>) const { *result += 1; }
    POET_FORCEINLINE void operator()(std::integral_constant<int, 2>) const { *result += 2; }
    POET_FORCEINLINE void operator()(std::integral_constant<int, 3>) const { *result += 3; }
};

struct functor_with_arg {
    int* result;

    template<int N>
    POET_FORCEINLINE void operator()(int arg) const {
        *result += N + arg;
    }

    POET_FORCEINLINE void operator()(std::integral_constant<int, 0>, int arg) const { *result += 0 + arg; }
    POET_FORCEINLINE void operator()(std::integral_constant<int, 1>, int arg) const { *result += 1 + arg; }
    POET_FORCEINLINE void operator()(std::integral_constant<int, 2>, int arg) const { *result += 2 + arg; }
    POET_FORCEINLINE void operator()(std::integral_constant<int, 3>, int arg) const { *result += 3 + arg; }
};

// ============================================================================
// Baseline: if constexpr chain (what dynamic_for uses)
// ============================================================================

template<int N>
POET_FORCEINLINE void if_constexpr_dispatch_impl(int runtime_val, int* result) {
    if constexpr (N == 0) {
        return;
    } else {
        if (runtime_val == N) {
            *result += N;
            return;
        }
        if constexpr (N > 1) {
            if_constexpr_dispatch_impl<N - 1>(runtime_val, result);
        }
    }
}

template<int Max>
POET_FORCEINLINE void if_constexpr_dispatch(int runtime_val, int* result) {
    if_constexpr_dispatch_impl<Max>(runtime_val, result);
}

// ============================================================================
// Alternative 1: Switch statement
// ============================================================================

template<int... Values>
struct switch_dispatcher;

template<int First, int... Rest>
struct switch_dispatcher<First, Rest...> {
    static POET_FORCEINLINE void dispatch(int runtime_val, int* result) {
        if (runtime_val == First) {
            *result += First;
            return;
        }
        if constexpr (sizeof...(Rest) > 0) {
            switch_dispatcher<Rest...>::dispatch(runtime_val, result);
        }
    }
};

template<>
struct switch_dispatcher<> {
    static POET_FORCEINLINE void dispatch(int /*runtime_val*/, int* /*result*/) {}
};

template<typename Seq> struct switch_dispatch_helper;

template<int... Is>
struct switch_dispatch_helper<std::integer_sequence<int, Is...>> {
    static POET_FORCEINLINE void dispatch(int runtime_val, int* result) {
        switch_dispatcher<(Is + 1)...>::dispatch(runtime_val, result);
    }
};

template<int Max>
POET_FORCEINLINE void switch_dispatch(int runtime_val, int* result) {
    switch_dispatch_helper<std::make_integer_sequence<int, Max>>::dispatch(runtime_val, result);
}

// ============================================================================
// Alternative 2: Direct function pointer array (no poet::dispatch wrapper)
// ============================================================================

template<int... Values>
POET_CPP20_CONSTEVAL auto make_simple_table(std::integer_sequence<int, Values...>) {
    return std::array<void(*)(int*), sizeof...(Values)>{
        +[](int* result) { *result += Values; }...
    };
}

template<int Max>
POET_FORCEINLINE void direct_fptr_dispatch(int runtime_val, int* result) {
    if (runtime_val >= 1 && runtime_val <= Max) {
        static constexpr auto table = make_simple_table(poet::make_range<1, Max>{});
        table[static_cast<std::size_t>(runtime_val - 1)](result);
    }
}

// Alternative 3: Jump table (real switch statement - compiler may optimize to jump table)
template<int Max>
POET_FORCEINLINE void real_switch_dispatch(int runtime_val, int* result) {
    // Use explicit switch statement - compiler will generate jump table for dense cases
    if constexpr (Max == 3) {
        switch(runtime_val) {
            case 1: *result += 1; return;
            case 2: *result += 2; return;
            case 3: *result += 3; return;
        }
    } else if constexpr (Max == 7) {
        switch(runtime_val) {
            case 1: *result += 1; return;
            case 2: *result += 2; return;
            case 3: *result += 3; return;
            case 4: *result += 4; return;
            case 5: *result += 5; return;
            case 6: *result += 6; return;
            case 7: *result += 7; return;
        }
    }
}

// ============================================================================
// Benchmark scenarios
// ============================================================================

constexpr int kSmallRange = 4;   // Small dispatch range (0-3)
constexpr int kMediumRange = 8;  // Medium dispatch range (0-7)

// Scenario 1: Small range, no arguments
template<int TestVal>
void bench_small_no_args(ankerl::nanobench::Bench& bench) {
    int result = 0;

    bench.run("poet::dispatch (1D, size=4)", [&] {
        simple_functor func{ &result };
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, kSmallRange-1>>{ TestVal });
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // if constexpr chain
    bench.run("if constexpr chain (size=4)", [&] {
        if_constexpr_dispatch<kSmallRange-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // Direct function pointer array
    bench.run("direct fptr array (size=4)", [&] {
        direct_fptr_dispatch<kSmallRange-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // Real switch statement
    bench.run("switch statement (size=4)", [&] {
        real_switch_dispatch<kSmallRange-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 2: Small range, with arguments
template<int TestVal>
void bench_small_with_args(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int arg = 42;

    bench.run("poet::dispatch (1D, size=4, +arg)", [&] {
        functor_with_arg func{ &result };
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, kSmallRange-1>>{ TestVal }, arg);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    // Manual dispatch with lambda
    bench.run("manual dispatch (size=4, +arg)", [&] {
        switch(TestVal) {
            case 0: result += 0 + arg; break;
            case 1: result += 1 + arg; break;
            case 2: result += 2 + arg; break;
            case 3: result += 3 + arg; break;
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 3: Medium range, test different values
template<int TestVal>
void bench_medium_range(ankerl::nanobench::Bench& bench, const char* label) {
    int result = 0;

    std::string poet_label = std::string("poet::dispatch (1D, size=8, val=") + label + ")";
    bench.run(poet_label, [&] {
        simple_functor func{ &result };
        poet::dispatch(func, poet::DispatchParam<poet::make_range<0, kMediumRange-1>>{ TestVal });
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    std::string ifconst_label = std::string("if constexpr (size=8, val=") + label + ")";
    bench.run(ifconst_label, [&] {
        if_constexpr_dispatch<kMediumRange-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    std::string fptr_label = std::string("direct fptr (size=8, val=") + label + ")";
    bench.run(fptr_label, [&] {
        direct_fptr_dispatch<kMediumRange-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Functor for 2D dispatch
struct func_2d {
    int* result;

    template<int V1, int V2>
    void operator()() const {
        *result += V1 * 10 + V2;
    }
};

// Scenario 4: 2D dispatch
template<int Val1, int Val2>
void bench_2d_dispatch(ankerl::nanobench::Bench& bench) {
    int result = 0;

    bench.run("poet::dispatch (2D, 4x4)", [&] {
        func_2d func{ &result };
        poet::dispatch(func,
            poet::DispatchParam<poet::make_range<0, 3>>{ Val1 },
            poet::DispatchParam<poet::make_range<0, 3>>{ Val2 });
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("manual nested switch (2D, 4x4)", [&] {
        switch(Val1) {
            case 0: switch(Val2) {
                case 0: result += 0; break;
                case 1: result += 1; break;
                case 2: result += 2; break;
                case 3: result += 3; break;
            } break;
            case 1: switch(Val2) {
                case 0: result += 10; break;
                case 1: result += 11; break;
                case 2: result += 12; break;
                case 3: result += 13; break;
            } break;
            case 2: switch(Val2) {
                case 0: result += 20; break;
                case 1: result += 21; break;
                case 2: result += 22; break;
                case 3: result += 23; break;
            } break;
            case 3: switch(Val2) {
                case 0: result += 30; break;
                case 1: result += 31; break;
                case 2: result += 32; break;
                case 3: result += 33; break;
            } break;
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Scenario 5: Unpredictable dispatch (worst case for branch prediction)
void bench_unpredictable(ankerl::nanobench::Bench& bench) {
    int result = 0;
    std::array<int, 100> random_vals = []() {
        std::array<int, 100> arr;
        for (std::size_t i = 0; i < 100; ++i) {
            arr[i] = static_cast<int>((i * 7 + 13) % kSmallRange);  // Pseudo-random pattern
        }
        return arr;
    }();

    bench.run("poet::dispatch (unpredictable)", [&] {
        simple_functor func{ &result };
        for (int val : random_vals) {
            poet::dispatch(func, poet::DispatchParam<poet::make_range<0, kSmallRange-1>>{ val });
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("if constexpr (unpredictable)", [&] {
        for (int val : random_vals) {
            if_constexpr_dispatch<kSmallRange-1>(val, &result);
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("direct fptr (unpredictable)", [&] {
        for (int val : random_vals) {
            direct_fptr_dispatch<kSmallRange-1>(val, &result);
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("manual switch (unpredictable)", [&] {
        for (int val : random_vals) {
            switch(val) {
                case 0: result += 0; break;
                case 1: result += 1; break;
                case 2: result += 2; break;
                case 3: result += 3; break;
            }
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// Helper for varying size dispatch
template<typename Seq> struct dispatch_by_seq;

template<int... Is>
struct dispatch_by_seq<std::integer_sequence<int, Is...>> {
    static void dispatch_poet(int test_val, int* result) {
        simple_functor func{ result };
        poet::dispatch(func, poet::DispatchParam<std::integer_sequence<int, Is...>>{ test_val });
    }
};

// Scenario 6: Vary dispatch table size
template<int Size, int TestVal>
void bench_varying_size(ankerl::nanobench::Bench& bench) {
    int result = 0;

    std::string label = std::string("size=") + std::to_string(Size);

    bench.run("poet::dispatch (" + label + ")", [&] {
        dispatch_by_seq<std::make_integer_sequence<int, Size>>::dispatch_poet(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("if constexpr (" + label + ")", [&] {
        if_constexpr_dispatch<Size-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("direct fptr (" + label + ")", [&] {
        direct_fptr_dispatch<Size-1>(TestVal, &result);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

}// namespace

void run_dispatch_optimization_benchmarks() {
    ankerl::nanobench::Bench bench;
    bench.title("poet::dispatch optimization analysis");
    bench.minEpochTime(10ms);
    bench.relative(true);

    // Test 1: Small range (4 values), first value
    bench.run("=== Small range (size=4), dispatch to value=1 ===", [] {});
    bench_small_no_args<1>(bench);

    // Test 2: Small range, last value
    bench.run("=== Small range (size=4), dispatch to value=3 ===", [] {});
    bench_small_no_args<3>(bench);

    // Test 3: Small range with arguments
    bench.run("=== Small range (size=4) with runtime arguments ===", [] {});
    bench_small_with_args<2>(bench);

    // Test 4: Medium range, first value
    bench.run("=== Medium range (size=8), dispatch to different values ===", [] {});
    bench_medium_range<0>(bench, "0");
    bench_medium_range<3>(bench, "3");
    bench_medium_range<7>(bench, "7");

    // Test 5: 2D dispatch
    bench.run("=== 2D dispatch (4x4 grid) ===", [] {});
    bench_2d_dispatch<2, 2>(bench);

    // Test 6: Unpredictable pattern
    bench.run("=== Unpredictable dispatch pattern (100 iterations) ===", [] {});
    bench_unpredictable(bench);

    // Test 7: Varying table sizes
    bench.run("=== Impact of dispatch table size ===", [] {});
    bench_varying_size<2, 1>(bench);
    bench_varying_size<4, 1>(bench);
    bench_varying_size<8, 1>(bench);
    bench_varying_size<16, 1>(bench);
}
