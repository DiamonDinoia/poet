/// \file dynamic_for_index_only_bench.cpp
/// \brief Compares current dynamic_for against blocked_for-style index-only loops.

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>
// Re-include after the umbrella's undef_macros.hpp so this TU can use the
// POET_FORCEINLINE / POET_UNLIKELY / POET_ALWAYS_INLINE_LAMBDA macros below.
#include <poet/core/macros.hpp>

namespace {

#if defined(__GNUC__) || defined(__clang__)
#define POET_BENCH_NOINLINE __attribute__((noinline))
#define POET_BENCH_FORCEINLINE_FLATTEN inline __attribute__((always_inline, flatten))
#else
#define POET_BENCH_NOINLINE
#define POET_BENCH_FORCEINLINE_FLATTEN inline
#endif

constexpr std::size_t kUnroll = 8;
constexpr std::size_t kMaxCount = 16384 + 32;

alignas(64) std::array<std::uint32_t, kMaxCount> g_in{};
alignas(64) std::array<std::uint32_t, kMaxCount> g_out{};
volatile std::uint32_t g_salt = 1;

POET_FORCEINLINE auto xorshift32(std::uint32_t x) noexcept -> std::uint32_t {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

POET_FORCEINLINE auto light_work(std::uint32_t x, std::uint32_t salt) noexcept -> std::uint32_t {
    return xorshift32(x ^ salt) + 0x9e3779b9U;
}

struct init_buffers {
    init_buffers() {
        std::uint32_t x = 1;
        for (std::size_t i = 0; i < kMaxCount; ++i) {
            x = xorshift32(x + static_cast<std::uint32_t>(i + 1));
            g_in[i] = x;
        }
    }
} g_init_buffers;

auto next_salt() noexcept -> std::uint32_t {
    auto s = xorshift32(g_salt);
    g_salt = s;
    return s;
}

template<typename Fn> void reg(const char *name, std::uint64_t batch, Fn &&fn) {
    benchmark::RegisterBenchmark(name, [fn = std::forward<Fn>(fn), batch](benchmark::State &state) mutable {
        for (auto _ : state) {
            benchmark::DoNotOptimize(fn());
            benchmark::DoNotOptimize(g_out.data());
            benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch));
    })->MinTime(0.2);
}

template<std::size_t Unroll, typename T, typename Callable>
POET_FORCEINLINE void blocked_for(T begin, T end, Callable &&func) {
    if (begin >= end) { return; }

    T i = begin;
    const T n_main = begin + ((end - begin) / static_cast<T>(Unroll)) * static_cast<T>(Unroll);

    for (; i < n_main; i += static_cast<T>(Unroll)) {
        poet::static_for<static_cast<std::ptrdiff_t>(Unroll)>([&](auto lane_c) {
            constexpr T lane = static_cast<T>(decltype(lane_c)::value);
            func(static_cast<T>(i + lane));
        });
    }

    for (; i < end; ++i) { func(i); }
}

namespace inline_only {

    template<typename...> inline constexpr bool always_false_v = false;

    struct lane_by_value_tag {};
    struct index_only_tag {};

    template<typename Func, typename T> constexpr auto detect_callable_form() {
        if constexpr (std::is_invocable_v<Func &, std::integral_constant<std::size_t, 0>, T>) {
            return lane_by_value_tag{};
        } else if constexpr (std::is_invocable_v<Func &, T>) {
            return index_only_tag{};
        } else {
            static_assert(always_false_v<Func>, "dynamic_for callable must accept (lane, index) or (index)");
            return index_only_tag{};
        }
    }

    template<typename Func, typename T> using callable_form_t = decltype(detect_callable_form<Func, T>());

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(lane_by_value_tag /*tag*/, Func &func, T index) {
        func(std::integral_constant<std::size_t, Lane>{}, index);
    }

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(index_only_tag /*tag*/, Func &func, T index) {
        func(index);
    }

    template<std::ptrdiff_t Step, typename FormTag, typename Callable, typename T, std::size_t... Lanes>
    POET_FORCEINLINE constexpr void
      emit_carried_ct(Callable &callable, T index, std::index_sequence<Lanes...> /*seq*/) {
        ((invoke_lane<Lanes>(FormTag{}, callable, index), index += static_cast<T>(Step)), ...);
    }

    template<std::ptrdiff_t Step, typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void emit_block_ct(FormTag /*tag*/, Callable &callable, T base) {
        if constexpr (Count > 0) { emit_carried_ct<Step, FormTag>(callable, base, std::make_index_sequence<Count>{}); }
    }

    template<std::ptrdiff_t Step, typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_ct(T begin, T end) -> std::size_t {
        static_assert(Step != 0, "Step must be non-zero");
        if constexpr (Step > 0) {
            if (begin >= end) { return 0; }
            const auto dist = static_cast<std::size_t>(end - begin);
            constexpr auto ustride = static_cast<std::size_t>(Step);
            return (dist + ustride - 1) / ustride;
        } else {
            if (begin <= end) { return 0; }
            const auto dist = static_cast<std::size_t>(begin - end);
            constexpr auto ustride = static_cast<std::size_t>(-Step);
            return (dist + ustride - 1) / ustride;
        }
    }

    template<std::size_t N, std::ptrdiff_t Step, typename FormTag, typename Callable, typename T>
    POET_FORCEINLINE void tail_binary_ct(std::size_t count, Callable &callable, T index) {
        if constexpr (N <= 1) {
        } else {
            constexpr std::size_t half = []() constexpr -> std::size_t {
                std::size_t pow2 = 1;
                while (pow2 * 2 < N) { pow2 *= 2; }
                return pow2;
            }();
            const std::size_t rem = (count >= half) ? (count - half) : count;
            tail_binary_ct<half, Step, FormTag>(rem, callable, index);
            if (count >= half) {
                emit_block_ct<Step, FormTag, Callable, T, half>(
                  FormTag{}, callable, static_cast<T>(index + static_cast<T>(static_cast<std::ptrdiff_t>(rem) * Step)));
            }
        }
    }

    template<std::ptrdiff_t Step, typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_BENCH_FORCEINLINE_FLATTEN void
      dynamic_for_impl_ct_stride(const T begin, const T end, Callable &callable, const FormTag tag) {
        const std::size_t count = calculate_iteration_count_ct<Step>(begin, end);
        if (POET_UNLIKELY(count == 0)) { return; }

        if constexpr (Unroll == 1) {
            T index = begin;
            constexpr T ct_stride = static_cast<T>(Step);
            for (std::size_t i = 0; i < count; ++i) {
                invoke_lane<0>(tag, callable, index);
                index += ct_stride;
            }
        } else {
            T index = begin;
            std::size_t remaining = count;

            if (POET_UNLIKELY(count < Unroll)) {
                if (count > 0) { tail_binary_ct<Unroll, Step, FormTag>(count, callable, index); }
                return;
            }

            constexpr T stride_times_unroll = static_cast<T>(static_cast<std::ptrdiff_t>(Unroll) * Step);
            while (remaining >= Unroll) {
                emit_block_ct<Step, FormTag, Callable, T, Unroll>(tag, callable, index);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            if (remaining > 0) { tail_binary_ct<Unroll, Step, FormTag>(remaining, callable, index); }
        }
    }

    template<std::size_t Unroll, std::ptrdiff_t Step, typename T1, typename T2, typename Func>
    POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
        static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
        static_assert(Step != 0, "dynamic_for requires Step != 0");

        using T = std::common_type_t<T1, T2>;

        auto run = [&](auto &callable) POET_ALWAYS_INLINE_LAMBDA -> void {
            using callable_t = std::remove_reference_t<decltype(callable)>;
            using form_tag = callable_form_t<callable_t, T>;
            dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
        };

        if constexpr (std::is_lvalue_reference_v<Func &&>) {
            run(func);
        } else {
            std::decay_t<Func> callable(std::forward<Func>(func));
            run(callable);
        }
    }

}// namespace inline_only

template<std::size_t Unroll>
POET_BENCH_NOINLINE auto run_plain_for(std::size_t count, std::uint32_t salt) -> std::uint32_t {
    for (std::size_t i = 0; i < count; ++i) { g_out[i] = light_work(g_in[i], salt); }
    return g_out[count / 2];
}

template<std::size_t Unroll>
POET_BENCH_NOINLINE auto run_blocked_for(std::size_t count, std::uint32_t salt) -> std::uint32_t {
    blocked_for<Unroll>(std::size_t{ 0 }, count, [salt](std::size_t i) { g_out[i] = light_work(g_in[i], salt); });
    return g_out[count / 2];
}

template<std::size_t Unroll>
POET_BENCH_NOINLINE auto run_dynamic_for_inline_only(std::size_t count, std::uint32_t salt) -> std::uint32_t {
    inline_only::dynamic_for<Unroll, 1>(
      std::size_t{ 0 }, count, [salt](std::size_t i) { g_out[i] = light_work(g_in[i], salt); });
    return g_out[count / 2];
}

template<std::size_t Unroll>
POET_BENCH_NOINLINE auto run_poet_dynamic_for(std::size_t count, std::uint32_t salt) -> std::uint32_t {
    poet::dynamic_for<Unroll, 1>(
      std::size_t{ 0 }, count, [salt](std::size_t i) { g_out[i] = light_work(g_in[i], salt); });
    return g_out[count / 2];
}

void register_count(std::size_t count, std::uint32_t salt) {
    const auto suffix = "/N=" + std::to_string(count);

    reg(("IndexOnly/plain_for" + suffix).c_str(), count, [count, salt] { return run_plain_for<kUnroll>(count, salt); });
    reg(("IndexOnly/blocked_for" + suffix).c_str(), count, [count, salt] {
        return run_blocked_for<kUnroll>(count, salt);
    });
    reg(("IndexOnly/dynamic_for_inline_only" + suffix).c_str(), count, [count, salt] {
        return run_dynamic_for_inline_only<kUnroll>(count, salt);
    });
    reg(("IndexOnly/poet_dynamic_for_ct_step1" + suffix).c_str(), count, [count, salt] {
        return run_poet_dynamic_for<kUnroll>(count, salt);
    });
}

}// namespace

int main(int argc, char **argv) {
    std::cerr << "\n=== dynamic_for Index-Only Benchmark ===\n";
    std::cerr << "Unroll:           " << kUnroll << "\n";
    std::cerr << "Workload:         light index-only step=1 store\n";
    std::cerr << "Counts:           1024, 1031, 4096, 4101\n\n";

    const auto salt = next_salt();
    register_count(1024, salt);
    register_count(1031, salt);
    register_count(4096, salt);
    register_count(4101, salt);

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
