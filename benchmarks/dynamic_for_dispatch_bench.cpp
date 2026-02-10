#include <cstddef>
#include <string>
#include <chrono>
using namespace std::chrono_literals;
#include <nanobench.h>

#include <poet/core/dynamic_for.hpp>
#include <poet/core/static_dispatch.hpp>
#include <poet/core/static_for.hpp>

namespace {

// Test different unroll factors
constexpr std::size_t Unroll = 4;

// ============================================================================
// Version 1: Current implementation (if constexpr chain)
// ============================================================================

template<std::size_t N, typename Callable, typename T>
POET_FORCEINLINE void match_and_execute_tail_ifconstexpr(std::size_t remaining, Callable &callable, T index, T stride) {
    if constexpr (N == 0) {
        return;
    } else {
        if (remaining == N) {
            poet::detail::execute_runtime_block<Callable, T, N>(std::addressof(callable), index, stride);
            return;
        }
        if constexpr (N > 1) {
            match_and_execute_tail_ifconstexpr<N - 1>(remaining, callable, index, stride);
        }
    }
}

template<typename T, typename Callable>
POET_FLATTEN void dynamic_for_ifconstexpr(T begin, T end, T stride, Callable &callable) {
    if (POET_UNLIKELY(stride == 0)) { return; }

    std::size_t count = 0;
    if (stride == 1 && begin < end) {
        count = static_cast<std::size_t>(end - begin);
    } else if (POET_UNLIKELY(stride == 1)) {
        return;
    } else {
        count = poet::detail::calculate_iteration_count_complex(begin, end, stride);
    }

    T index = begin;
    std::size_t remaining = count;

    if (POET_UNLIKELY(count < Unroll)) {
        for (std::size_t i = 0; i < count; ++i) {
            callable(index);
            index += stride;
        }
        return;
    }

    const T stride_times_unroll = static_cast<T>(Unroll) * stride;
    while (remaining >= Unroll) {
        poet::detail::execute_runtime_block<Callable, T, Unroll>(std::addressof(callable), index, stride);
        index += stride_times_unroll;
        remaining -= Unroll;
    }

    if constexpr (Unroll > 1) {
        if (POET_UNLIKELY(remaining > 0)) {
            match_and_execute_tail_ifconstexpr<Unroll - 1>(remaining, callable, index, stride);
        }
    }
}

// ============================================================================
// Version 2: poet::dispatch version (function pointer array)
// ============================================================================

template<typename Callable, typename T>
struct tail_dispatch_functor {
    Callable* callable;
    T index;
    T stride;

    template<int N>
    POET_FORCEINLINE void operator()() const {
        if constexpr (N > 0) {
            poet::detail::execute_runtime_block<Callable, T, N>(callable, index, stride);
        }
    }
};

template<typename T, typename Callable>
POET_FLATTEN void dynamic_for_dispatch(T begin, T end, T stride, Callable &callable) {
    if (POET_UNLIKELY(stride == 0)) { return; }

    std::size_t count = 0;
    if (stride == 1 && begin < end) {
        count = static_cast<std::size_t>(end - begin);
    } else if (POET_UNLIKELY(stride == 1)) {
        return;
    } else {
        count = poet::detail::calculate_iteration_count_complex(begin, end, stride);
    }

    T index = begin;
    std::size_t remaining = count;

    if (POET_UNLIKELY(count < Unroll)) {
        for (std::size_t i = 0; i < count; ++i) {
            callable(index);
            index += stride;
        }
        return;
    }

    const T stride_times_unroll = static_cast<T>(Unroll) * stride;
    while (remaining >= Unroll) {
        poet::detail::execute_runtime_block<Callable, T, Unroll>(std::addressof(callable), index, stride);
        index += stride_times_unroll;
        remaining -= Unroll;
    }

    if constexpr (Unroll > 1) {
        if (POET_UNLIKELY(remaining > 0)) {
            // Use poet::dispatch for tail handling
            tail_dispatch_functor<Callable, T> functor{ std::addressof(callable), index, stride };
            poet::dispatch(
                functor,
                poet::DispatchParam<poet::make_range<1, Unroll-1>>{ static_cast<int>(remaining) }
            );
        }
    }
}

// ============================================================================
// Version 3: Normal for loop (baseline)
// ============================================================================

template<typename T, typename Callable>
void normal_for_loop(T begin, T end, T stride, Callable &callable) {
    for (T i = begin; i < end; i += stride) {
        callable(i);
    }
}

// ============================================================================
// Benchmark helpers
// ============================================================================

// Benchmark scenario 1: Full blocks only (no tail) - Best case for all
auto sum_full_blocks(std::size_t count) -> int {
    int total = 0;

    // Current implementation
    poet::dynamic_for<Unroll>(std::size_t{0}, count, [&](std::size_t i) {
        total += static_cast<int>(i);
    });

    return total;
}

auto sum_full_blocks_dispatch(std::size_t count) -> int {
    int total = 0;
    auto callable = [&](std::size_t i) { total += static_cast<int>(i); };
    dynamic_for_dispatch<std::size_t>(std::size_t{0}, count, std::size_t{1}, callable);
    return total;
}

auto sum_full_blocks_normal(std::size_t count) -> int {
    int total = 0;
    auto callable = [&](std::size_t i) { total += static_cast<int>(i); };
    normal_for_loop<std::size_t>(std::size_t{0}, count, std::size_t{1}, callable);
    return total;
}

// Benchmark scenario 2: Small tail (1-3 iterations) - Stress test for tail dispatch
auto sum_with_tail(std::size_t count) -> int {
    int total = 0;

    // Current implementation
    poet::dynamic_for<Unroll>(std::size_t{0}, count, [&](std::size_t i) {
        total += static_cast<int>(i);
    });

    return total;
}

auto sum_with_tail_dispatch(std::size_t count) -> int {
    int total = 0;
    auto callable = [&](std::size_t i) { total += static_cast<int>(i); };
    dynamic_for_dispatch<std::size_t>(std::size_t{0}, count, std::size_t{1}, callable);
    return total;
}

auto sum_with_tail_normal(std::size_t count) -> int {
    int total = 0;
    auto callable = [&](std::size_t i) { total += static_cast<int>(i); };
    normal_for_loop<std::size_t>(std::size_t{0}, count, std::size_t{1}, callable);
    return total;
}

// Benchmark scenario 3: Only tail (count < Unroll) - Tiny loops
auto sum_tiny(std::size_t count) -> int {
    int total = 0;

    poet::dynamic_for<Unroll>(std::size_t{0}, count, [&](std::size_t i) {
        total += static_cast<int>(i);
    });

    return total;
}

auto sum_tiny_dispatch(std::size_t count) -> int {
    int total = 0;
    auto callable = [&](std::size_t i) { total += static_cast<int>(i); };
    dynamic_for_dispatch<std::size_t>(std::size_t{0}, count, std::size_t{1}, callable);
    return total;
}

auto sum_tiny_normal(std::size_t count) -> int {
    int total = 0;
    auto callable = [&](std::size_t i) { total += static_cast<int>(i); };
    normal_for_loop<std::size_t>(std::size_t{0}, count, std::size_t{1}, callable);
    return total;
}

}// namespace

void run_dynamic_for_dispatch_benchmarks() {
    ankerl::nanobench::Bench bench;
    bench.title("dynamic_for tail dispatch: if constexpr vs poet::dispatch vs for loop");
    bench.minEpochTime(10ms);
    bench.relative(true);

    // Scenario 1: Full blocks only (multiple of Unroll) - amortizes tail dispatch cost
    {
        constexpr std::size_t count = Unroll * 25;  // 100 iterations

        bench.run("Full blocks (if constexpr)", [=] {
            const auto result = sum_full_blocks(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Full blocks (poet::dispatch)", [=] {
            const auto result = sum_full_blocks_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Full blocks (normal for)", [=] {
            const auto result = sum_full_blocks_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 2a: With tail=1 - stress test tail dispatch
    {
        constexpr std::size_t count = Unroll * 10 + 1;  // 41 iterations

        bench.run("With tail=1 (if constexpr)", [=] {
            const auto result = sum_with_tail(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("With tail=1 (poet::dispatch)", [=] {
            const auto result = sum_with_tail_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("With tail=1 (normal for)", [=] {
            const auto result = sum_with_tail_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 2b: With tail=2
    {
        constexpr std::size_t count = Unroll * 10 + 2;  // 42 iterations

        bench.run("With tail=2 (if constexpr)", [=] {
            const auto result = sum_with_tail(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("With tail=2 (poet::dispatch)", [=] {
            const auto result = sum_with_tail_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("With tail=2 (normal for)", [=] {
            const auto result = sum_with_tail_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 2c: With tail=3 (Unroll-1)
    {
        constexpr std::size_t count = Unroll * 10 + 3;  // 43 iterations

        bench.run("With tail=3 (if constexpr)", [=] {
            const auto result = sum_with_tail(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("With tail=3 (poet::dispatch)", [=] {
            const auto result = sum_with_tail_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("With tail=3 (normal for)", [=] {
            const auto result = sum_with_tail_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 3a: Tiny loops (count=1) - fast path
    {
        constexpr std::size_t count = 1;

        bench.run("Tiny loop (count=1, if constexpr)", [=] {
            const auto result = sum_tiny(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Tiny loop (count=1, poet::dispatch)", [=] {
            const auto result = sum_tiny_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Tiny loop (count=1, normal for)", [=] {
            const auto result = sum_tiny_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 3b: Tiny loops (count=3)
    {
        constexpr std::size_t count = 3;

        bench.run("Tiny loop (count=3, if constexpr)", [=] {
            const auto result = sum_tiny(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Tiny loop (count=3, poet::dispatch)", [=] {
            const auto result = sum_tiny_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Tiny loop (count=3, normal for)", [=] {
            const auto result = sum_tiny_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 4: Medium loop with tail - realistic mixed case
    {
        constexpr std::size_t count = 127;  // 31*4 + 3

        bench.run("Medium (127 iter, if constexpr)", [=] {
            const auto result = sum_with_tail(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Medium (127 iter, poet::dispatch)", [=] {
            const auto result = sum_with_tail_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Medium (127 iter, normal for)", [=] {
            const auto result = sum_with_tail_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Scenario 5: Large loop - tail cost amortized
    {
        constexpr std::size_t count = 4095;  // 1023*4 + 3

        bench.run("Large (4095 iter, if constexpr)", [=] {
            const auto result = sum_with_tail(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Large (4095 iter, poet::dispatch)", [=] {
            const auto result = sum_with_tail_dispatch(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("Large (4095 iter, normal for)", [=] {
            const auto result = sum_with_tail_normal(count);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }
}
