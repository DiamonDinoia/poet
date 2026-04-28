#pragma once

/// \file dynamic_for.hpp
/// \brief Runtime loops emitted as compile-time unrolled blocks.

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>


namespace poet {

namespace detail {

    template<typename...> inline constexpr bool always_false_v = false;

    struct lane_by_value_tag {};///< func(integral_constant<size_t, Lane>{}, index)
    struct index_only_tag {};///< func(index)

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

    // Emits `Count` calls as a single expanded pack; the comma-fold carries `index` forward
    // so each lane sees a distinct compile-time `Lane` and the running runtime `index`.
    template<typename FormTag, typename Callable, typename T, std::size_t... Lanes>
    POET_FORCEINLINE constexpr void
      emit_carried(Callable &callable, T index, T stride, std::index_sequence<Lanes...> /*seq*/) {
        ((invoke_lane<Lanes>(FormTag{}, callable, index), index += stride), ...);
    }

    template<typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void emit_block(FormTag /*tag*/,
      [[maybe_unused]] Callable &callable,
      [[maybe_unused]] T base,
      [[maybe_unused]] T stride) {
        if constexpr (Count > 0) { emit_carried<FormTag>(callable, base, stride, std::make_index_sequence<Count>{}); }
    }

    template<std::ptrdiff_t Step, typename FormTag, typename Callable, typename T, std::size_t... Lanes>
    POET_FORCEINLINE constexpr void
      emit_carried_ct(Callable &callable, T index, std::index_sequence<Lanes...> /*seq*/) {
        ((invoke_lane<Lanes>(FormTag{}, callable, index), index += static_cast<T>(Step)), ...);
    }

    template<std::ptrdiff_t Step, typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void
      emit_block_ct(FormTag /*tag*/, [[maybe_unused]] Callable &callable, [[maybe_unused]] T base) {
        if constexpr (Count > 0) { emit_carried_ct<Step, FormTag>(callable, base, std::make_index_sequence<Count>{}); }
    }

    // Handles a leftover count in [0, N) by emitting at most log2(N) fixed-size unrolled
    // blocks — picks the largest power of two <= N/2, optionally emits it, then recurses on
    // the remainder. Each level has a compile-time `half`, so codegen stays fully unrolled.
    template<std::size_t N, typename FormTag, typename Callable, typename T>
    POET_FORCEINLINE void tail_binary(std::size_t count, Callable &callable, T index, T stride) {
        if constexpr (N <= 1) {
        } else {
            // Largest power of two strictly less than N — the block size we might emit here.
            constexpr std::size_t half = []() constexpr -> std::size_t {
                std::size_t pow2 = 1;
                while (pow2 * 2 < N) { pow2 *= 2; }
                return pow2;
            }();
            // If this level fires, it consumes exactly `half` iterations; otherwise all
            // `count` pass through to the smaller level.
            const std::size_t rem = (count >= half) ? (count - half) : count;
            tail_binary<half, FormTag>(rem, callable, index, stride);
            if (count >= half) {
                // Smaller blocks run first over the low indices; this block picks up at
                // `index + rem*stride` so iteration order is preserved.
                emit_block<FormTag, Callable, T, half>(
                  FormTag{}, callable, static_cast<T>(index + (static_cast<T>(rem) * stride)), stride);
            }
        }
    }

    template<std::size_t N, typename FormTag, typename Callable, typename T>
    POET_NOINLINE_FLATTEN void tail_binary_noinline(std::size_t count, Callable &callable, T index, T stride) {
        tail_binary<N, FormTag>(count, callable, index, stride);
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

    template<std::size_t N, std::ptrdiff_t Step, typename FormTag, typename Callable, typename T>
    POET_NOINLINE_FLATTEN void tail_binary_ct_noinline(std::size_t count, Callable &callable, T index) {
        tail_binary_ct<N, Step, FormTag>(count, callable, index);
    }

    // Handles signed and unsigned-wrapped-negative strides uniformly. For unsigned T, a
    // "negative" stride arrives as a large positive value (> half_max); we detect and flip
    // it so both directions share the same (dist + |stride| - 1) / |stride| ceiling formula.
    template<typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_complex(T begin, T end, T stride) -> std::size_t {
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (POET_UNLIKELY(stride < 0 || is_wrapped_negative)) {
            // Descending: empty unless begin > end.
            if (POET_UNLIKELY(begin <= end)) { return 0; }
            T abs_stride;
            if constexpr (std::is_signed_v<T>) {
                abs_stride = static_cast<T>(-stride);
            } else {
                // Unsigned two's-complement negation recovers the original magnitude.
                abs_stride = static_cast<T>(0) - stride;
            }
            auto dist = static_cast<std::size_t>(begin - end);
            auto ustride = static_cast<std::size_t>(abs_stride);
            return (dist + ustride - 1) / ustride;
        }

        if (begin >= end) { return 0; }

        auto dist = static_cast<std::size_t>(end - begin);
        auto ustride = static_cast<std::size_t>(stride);
        // Classic `x & (x-1) == 0` power-of-two test; replaces the divide with a shift.
        // Worth ~18x cycles on znver4 (`tzcntq+shrxq` ≈ 1c block-RT vs `divq` ≈ 18c) —
        // see /tmp/poet-asm/SUMMARY.md (T5) for the simdref+llvm-mca cross-check.
        const bool is_power_of_2 = (ustride & (ustride - 1)) == 0;

        if (is_power_of_2) {
            const unsigned int shift = poet_count_trailing_zeros(ustride);
            return (dist + ustride - 1) >> shift;
        }
        return (dist + ustride - 1) / ustride;
    }

    template<std::ptrdiff_t Step, typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_ct(T begin, T end) -> std::size_t {
        static_assert(Step != 0, "Step must be non-zero");
        if constexpr (Step > 0) {
            if (begin >= end) { return 0; }
            auto dist = static_cast<std::size_t>(end - begin);
            constexpr auto ustride = static_cast<std::size_t>(Step);
            return (dist + ustride - 1) / ustride;
        } else {
            if (begin <= end) { return 0; }
            auto dist = static_cast<std::size_t>(begin - end);
            constexpr auto ustride = static_cast<std::size_t>(-Step);
            return (dist + ustride - 1) / ustride;
        }
    }

    template<typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP void
      dynamic_for_impl_general(const T begin, const T end, const T stride, Callable &callable, const FormTag tag) {
        if (POET_UNLIKELY(stride == 0)) { return; }

        std::size_t count = calculate_iteration_count_complex(begin, end, stride);
        if (POET_UNLIKELY(count == 0)) { return; }

        if constexpr (Unroll == 1) {
            T index = begin;
            for (std::size_t i = 0; i < count; ++i) {
                invoke_lane<0>(tag, callable, index);
                index += stride;
            }
        } else {
            T index = begin;
            std::size_t remaining = count;

            if (POET_UNLIKELY(count < Unroll)) {
                if (count > 0) { tail_binary<Unroll, FormTag>(count, callable, index, stride); }
                return;
            }

            const T stride_times_unroll = static_cast<T>(Unroll) * stride;
            while (remaining >= Unroll) {
                emit_block<FormTag, Callable, T, Unroll>(tag, callable, index, stride);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            if (remaining > 0) { tail_binary_noinline<Unroll, FormTag>(remaining, callable, index, stride); }
        }
    }

    template<std::ptrdiff_t Step, typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP void dynamic_for_impl_ct_stride(const T begin, const T end, Callable &callable, const FormTag tag) {
        std::size_t count = calculate_iteration_count_ct<Step>(begin, end);
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

            if (remaining > 0) { tail_binary_ct_noinline<Unroll, Step, FormTag>(remaining, callable, index); }
        }
    }

}// namespace detail

// ============================================================================
// Public API
// ============================================================================

/// \brief Runs `[begin, end)` with compile-time unrolled blocks.
///
/// `func` may take `(index)` or `(lane, index)`, where `lane` is
/// `std::integral_constant<std::size_t, L>`. Use the lane form for
/// multi-accumulator kernels; prefer a plain `for` loop for trivial index-only work.
template<std::size_t Unroll, typename T1, typename T2, typename T3, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");

    using T = std::common_type_t<T1, T2, T3>;
    const T stride = static_cast<T>(step);

    auto run = [&](auto &callable) POET_ALWAYS_INLINE_LAMBDA -> void {
        using callable_t = std::remove_reference_t<decltype(callable)>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        if (stride == static_cast<T>(1)) {
            detail::dynamic_for_impl_ct_stride<1, T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
        } else {
            detail::dynamic_for_impl_general<T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), stride, callable, form_tag{});
        }
    };

    if constexpr (std::is_lvalue_reference_v<Func>) {
        run(func);
    } else {
        std::remove_reference_t<Func> local(std::forward<Func>(func));
        run(local);
    }
}

/// \brief Runs `[begin, end)` with a compile-time stride.
template<std::size_t Unroll, std::ptrdiff_t Step, typename T1, typename T2, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");

    using T = std::common_type_t<T1, T2>;

    auto run = [&](auto &callable) POET_ALWAYS_INLINE_LAMBDA -> void {
        using callable_t = std::remove_reference_t<decltype(callable)>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        detail::dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
    };

    if constexpr (std::is_lvalue_reference_v<Func>) {
        run(func);
    } else {
        std::remove_reference_t<Func> local(std::forward<Func>(func));
        run(local);
    }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/// \brief Runs `[begin, end)` with an inferred step of `+1` or `-1`.
template<std::size_t Unroll, typename T1, typename T2, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    T s_begin = static_cast<T>(begin);
    T s_end = static_cast<T>(end);
    T step = (s_begin <= s_end) ? static_cast<T>(1) : static_cast<T>(-1);

    dynamic_for<Unroll>(s_begin, s_end, step, std::forward<Func>(func));
}

/// \brief Convenience overload for `[0, count)`.
template<std::size_t Unroll, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll>(static_cast<std::size_t>(0), count, std::size_t{ 1 }, std::forward<Func>(func));
}
#endif

}// namespace poet


#if __cplusplus >= 202002L
#include <ranges>
#include <tuple>

namespace poet {

template<typename Func, std::size_t Unroll> struct dynamic_for_adaptor {
    Func func;
    constexpr explicit dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

template<typename Func, std::size_t Unroll, typename Range>
requires std::ranges::range<Range> void operator|(Range const &r, dynamic_for_adaptor<Func, Unroll> const &ad) {
    auto it = std::ranges::begin(r);
    auto it_end = std::ranges::end(r);

    if (it == it_end) return;// empty range

    using ValT = std::remove_reference_t<decltype(*it)>;
    ValT start = *it;

    std::size_t count = 0;
    for (auto jt = it; jt != it_end; ++jt) ++count;

    // Treat the range as a consecutive [start, start + count) sequence.
    poet::dynamic_for<Unroll>(start, static_cast<ValT>(start + static_cast<ValT>(count)), ad.func);
}

template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
    auto [b, e, s] = t;
    poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

template<std::size_t U, typename F> constexpr auto make_dynamic_for(F &&f) -> dynamic_for_adaptor<std::decay_t<F>, U> {
    return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

}// namespace poet
#endif// __cplusplus >= 202002L
