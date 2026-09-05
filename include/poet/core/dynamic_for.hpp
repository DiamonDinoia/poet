#pragma once

/// \file dynamic_for.hpp
/// \brief Runtime-bounded loops with a compile-time-unrolled body.
///
/// One `run_loop` template covers every public overload: the stride, the
/// callable form, and the extra by-value arguments are template parameters, so
/// no tag object or dispatch value is passed at run time. The main loop emits
/// fully unrolled blocks of `Unroll` iterations; the tail is a binary
/// decomposition with O(log2 Unroll) branches; a range smaller than `Unroll`
/// is inlined so the lane constants stay visible.
///
/// `Unroll` is exact, not a hint: `POET_NO_UNROLL` on both runtime loops holds
/// the emitted main loop at exactly `Unroll` bodies per back-edge, `Unroll == 1`,
/// `-funroll-loops` and a compile-time-constant count included. The last case
/// needs `opaque_count` too: the pragma outranks GCC's size heuristic rather
/// than capping it, so a visible trip count invites the complete unroller to
/// peel the loop away. A single block carries no loop at all, since one block is
/// already exactly `Unroll` bodies, so a count of `Unroll` folds to straight-line
/// code. `tests/exact_unroll_check.cpp` counts the bodies.
///
/// `dynamic_for` pays off for multi-accumulator work: the lane form
/// (`func(lane_constant, index)`) gives one accumulator per lane, breaking the
/// serial dependence of a plain loop. For element-wise work or one serial
/// chain, a plain `for` loop has less overhead.

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <poet/core/for_utils.hpp>
#include <poet/core/macros.hpp>


namespace poet {

namespace detail {

    // --- Callable form: resolved once per instantiation, never per iteration ---

    /// \brief True if F accepts `(index, args...)` or `(lane_constant, index, args...)`.
    ///
    /// Guards the enable_if on every public overload so a non-callable Func
    /// slot is removed from overload resolution.
    template<typename F, typename T, typename... Args>
    inline constexpr bool is_df_callable_v =
      std::is_invocable_v<F &, T, Args...>
      || std::is_invocable_v<F &, std::integral_constant<std::size_t, 0>, T, Args...>;

    /// \brief True when the callable takes the lane as a leading `integral_constant`.
    /// Given `is_df_callable_v`, "not this" means the index-only form.
    template<typename F, typename T, typename... Args>
    inline constexpr bool wants_lane_v = std::is_invocable_v<F &, std::integral_constant<std::size_t, 0>, T, Args...>;

    template<bool WantsLane, std::size_t Lane, typename Func, typename T, typename... Args>
    POET_FORCEINLINE constexpr void invoke_lane(Func &func, T index, Args... args) {
        if constexpr (WantsLane) {
            func(std::integral_constant<std::size_t, Lane>{}, index, args...);
        } else {
            func(index, args...);
        }
    }

    // --- Stride carrier ---

    /// A stride fixed at compile time: an empty type, so it costs no register
    /// and its value reaches every expression as a literal. A runtime stride is
    /// a plain `T`; one implementation serves both forms.
    template<std::ptrdiff_t Step> using static_stride = std::integral_constant<std::ptrdiff_t, Step>;

    /// Narrows either stride flavour to `T`. The `static_stride` implicit
    /// conversion covers the compile-time case, so one cast serves both.
    template<typename T, typename Stride> POET_FORCEINLINE constexpr auto stride_of(Stride stride) noexcept -> T {
        return static_cast<T>(stride);
    }

    // --- Iteration count ---

    /// True when the stride runs backward. An unsigned `T` wraps a negative
    /// stride into the top half of its range, which also counts as backward.
    /// The `if constexpr` keeps an unsigned `T` from the always-false
    /// `stride < 0` comparison.
    template<typename T> POET_FORCEINLINE constexpr auto is_backward(T stride) noexcept -> bool {
        if constexpr (std::is_signed_v<T>) {
            return stride < 0;
        } else {
            return stride > (std::numeric_limits<T>::max() / 2);
        }
    }

    POET_FORCEINLINE constexpr auto is_power_of_two(std::size_t value) noexcept -> bool {
        return (value & (value - 1)) == 0;
    }

    /// \brief Number of iterations in `[begin, end)` at the given stride.
    ///
    /// With a `static_stride`, the direction and power-of-two tests
    /// constant-fold to the arithmetic of a compile-time-stride loop.
    template<typename T, typename Stride>
    POET_FORCEINLINE constexpr auto iteration_count(T begin, T end, Stride stride_in) -> std::size_t {
        using unsigned_t = std::make_unsigned_t<T>;
        const T stride = stride_of<T>(stride_in);

        // Every public overload asserts `Step != 0`, so only a runtime stride
        // can still be zero here, and dividing by zero below would be UB.
        if constexpr (std::is_integral_v<Stride>) {
            if (POET_UNLIKELY(stride == 0)) { return 0; }
        }

        if (POET_UNLIKELY(is_backward(stride))) {
            if (POET_UNLIKELY(begin <= end)) { return 0; }
            // Negate at T's width, where wrapping is defined: this recovers
            // `2` from both signed `-2` and unsigned `T(-2)`. Written as
            // `0 - x` because MSVC's C4146 flags the deliberate wrap.
            const auto negated = static_cast<unsigned_t>(unsigned_t{ 0 } - static_cast<unsigned_t>(stride));
            const auto magnitude = static_cast<std::size_t>(negated);
            return ((static_cast<std::size_t>(begin - end) + magnitude) - 1) / magnitude;
        }

        if (POET_UNLIKELY(begin >= end)) { return 0; }

        const auto magnitude = static_cast<std::size_t>(stride);
        const std::size_t span = (static_cast<std::size_t>(end - begin) + magnitude) - 1;
        // A power-of-two stride, including the dominant stride==1 case,
        // shifts instead of dividing.
        if (POET_LIKELY(is_power_of_two(magnitude))) { return span >> count_trailing_zeros(magnitude); }
        return span / magnitude;
    }

    // --- Block emission ---

    /// Carried index (`index += stride`) rather than `base + Lane * stride`:
    /// the lane-to-lane dependence keeps GCC's SLP vectorizer from packing the
    /// index computations into a vector; per-lane accumulators stay independent.
    template<bool WantsLane, typename Func, typename T, typename Stride, std::size_t... Lanes, typename... Args>
    POET_FORCEINLINE constexpr void
      emit_lanes(Func &func, T index, Stride stride, std::index_sequence<Lanes...> /*lanes*/, Args... args) {
        ((invoke_lane<WantsLane, Lanes>(func, index, args...), index += stride_of<T>(stride)), ...);
    }

    /// One fully unrolled block of `Count` iterations starting at `index`.
    template<std::size_t Count, bool WantsLane, typename Func, typename T, typename Stride, typename... Args>
    POET_FORCEINLINE constexpr void emit_block(Func &func, T index, Stride stride, Args... args) {
        emit_lanes<WantsLane>(func, index, stride, std::make_index_sequence<Count>{}, args...);
    }

    // --- Binary decomposition tail ---

    /// Largest power of two strictly below `bound` (`bound >= 2`).
    constexpr auto half_below(std::size_t bound) noexcept -> std::size_t {
        std::size_t pow2 = 1;
        while (pow2 * 2 < bound) { pow2 *= 2; }
        return pow2;
    }

    /// \brief Runs the final 0..N-1 iterations by halving the envelope.
    ///
    /// Each level spends one branch on its upper half and emits it as a fully
    /// unrolled block: O(log2 N) branches instead of the O(N) of a cascade.
    /// Lanes restart at 0 in each emitted block, so a tail iteration's lane is
    /// not `index % Unroll`. Per-lane accumulators stay correct; code that
    /// assumes a fixed lane-to-iteration mapping does not.
    template<std::size_t N, bool WantsLane, typename Func, typename T, typename Stride, typename... Args>
    POET_FORCEINLINE void tail_binary(std::size_t count, Func &func, T index, Stride stride, Args... args) {
        if constexpr (N > 1) {
            constexpr std::size_t half = half_below(N);
            const std::size_t rem = (count >= half) ? (count - half) : count;
            tail_binary<half, WantsLane>(rem, func, index, stride, args...);
            if (count >= half) {
                const T offset = static_cast<T>(rem) * stride_of<T>(stride);
                emit_block<half, WantsLane>(func, static_cast<T>(index + offset), stride, args...);
            }
        }
    }

    /// The same tail, kept out of line so its register allocation cannot perturb
    /// the hot loop's. `flatten` stops GCC's ISRA pass from re-outlining each
    /// functor body inside the tail, which would reload loop constants per call.
    template<std::size_t N, bool WantsLane, typename Func, typename T, typename Stride, typename... Args>
    POET_NOINLINE_FLATTEN void
      tail_binary_outlined(std::size_t count, Func &func, T index, Stride stride, Args... args) {
        tail_binary<N, WantsLane>(count, func, index, stride, args...);
    }

    /// \brief Returns `count` in a form the optimizer cannot constant-fold.
    ///
    /// Hides the trip count, where `POET_NO_UNROLL` holds the loop shape. The
    /// two cover disjoint compilers: `POET_NO_UNROLL` is empty on MSVC, so this
    /// is the whole of the `Unroll == 1` contract there. On GNU and clang the
    /// pragma alone already holds every cell the check measures, and an empty
    /// asm barrier costs zero instructions, so this stays. MSVC has no x64
    /// inline asm and pays one stack store and load for it.
    template<typename T> POET_FORCEINLINE auto opaque_count(T count) -> T {
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : "+r"(count));// NOLINT(hicpp-no-assembler)
        return count;
#elif defined(_MSC_VER)
        volatile T laundered = count;
        return laundered;
#else
        return count;
#endif
    }

    POET_PUSH_OPTIMIZE

    // --- Fused implementation ---

    /// \brief The whole of dynamic_for: main unrolled loop plus binary tail.
    ///
    /// `Args...` are loop-invariant "hot" values threaded by value; see the
    /// public `(count, func, args...)` overload for the rationale.
    template<std::size_t Unroll, bool WantsLane, typename T, typename Func, typename Stride, typename... Args>
    POET_HOT_LOOP void run_loop(const T begin, const T end, Stride stride, Func &func, Args... args) {
        const std::size_t count = iteration_count(begin, end, stride);
        if (POET_UNLIKELY(count == 0)) { return; }

        T index = begin;

        if constexpr (Unroll == 1) {
            const std::size_t trips = opaque_count(count);
            POET_NO_UNROLL
            for (std::size_t i = 0; i < trips; ++i) {
                invoke_lane<WantsLane, 0>(func, index, args...);
                index += stride_of<T>(stride);
            }
        } else if (POET_UNLIKELY(count < Unroll)) {
            // Tiny range: there is no main loop to run, so inline the tail and
            // keep the lane constants visible.
            tail_binary<Unroll, WantsLane>(count, func, index, stride, args...);
        } else {
            const T block_step = static_cast<T>(Unroll) * stride_of<T>(stride);
            const std::size_t blocks = count / Unroll;
            const std::size_t remaining = count % Unroll;
            if (blocks == 1) {
                // One block is already exactly `Unroll` bodies, so there is
                // nothing to unroll and nothing to hide. Hiding it here would
                // cost a constant count its straight-line form.
                emit_block<Unroll, WantsLane>(func, index, stride, args...);
                index += block_step;
            } else {
                // Only the BLOCK count is hidden, and only where a loop runs.
                // `POET_NO_UNROLL` outranks GCC's size heuristic instead of
                // merely capping it, so a visible trip count lets the complete
                // unroller peel the loop away and the contract with it.
                // `remaining` keeps reading the original `count`, so a constant
                // count still folds the tail to its exact blocks.
                // A `do` loop would drop the entry test, since `blocks >= 2`
                // here, but GCC 14 ignores a loop annotation on `do` and says
                // so, which leaves the shape to `opaque_count` alone.
                const std::size_t trips = opaque_count(blocks);
                POET_NO_UNROLL
                for (std::size_t block = 0; block < trips; ++block) {
                    emit_block<Unroll, WantsLane>(func, index, stride, args...);
                    index += block_step;
                }
            }
            if (remaining > 0) { tail_binary_outlined<Unroll, WantsLane>(remaining, func, index, stride, args...); }
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// --- Public API ---

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// Iterates over `[begin, end)` with the given `step`, emitting blocks of
/// `Unroll` iterations. `step == 1` selects the compile-time-stride path.
///
/// \tparam Unroll Iterations per unrolled block, exactly: the main loop carries
///   `Unroll` bodies per back-edge and the compiler does not unroll it further.
///   A range of exactly `Unroll` is one block and no loop.
///   No default: choose per call site. `2` small codegen, `4` balanced, `8`
///   profiled hot loops, `1` a loop that stays rolled.
/// \param begin Inclusive start bound.
/// \param end Exclusive end bound.
/// \param step Increment per iteration. May be negative.
/// \param func Callable invoked per iteration, in either form:
///   - `func(std::integral_constant<std::size_t, lane>{}, index)`: lane as type
///   - `func(index)`: index only
template<std::size_t Unroll,
  typename T1,
  typename T2,
  typename T3,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::common_type_t<T1, T2, T3>>, int> = 0>
POET_FORCEINLINE void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");

    using T = std::common_type_t<T1, T2, T3>;
    constexpr bool lane = detail::wants_lane_v<std::remove_reference_t<Func>, T>;

    detail::callable_storage_t<Func> callable(std::forward<Func>(func));
    const T stride = static_cast<T>(step);

    if (stride == static_cast<T>(1)) {
        detail::run_loop<Unroll, lane>(
          static_cast<T>(begin), static_cast<T>(end), detail::static_stride<1>{}, callable);
    } else {
        detail::run_loop<Unroll, lane>(static_cast<T>(begin), static_cast<T>(end), stride, callable);
    }
}

/// \brief Executes a runtime-sized loop with a compile-time stride.
///
/// Per-lane stride multiplications become compile-time constants and the
/// direction test in the iteration count folds away.
///
/// \tparam Unroll Iterations emitted per unrolled block.
/// \tparam Step Compile-time stride (must be non-zero).
template<std::size_t Unroll,
  std::ptrdiff_t Step,
  typename T1,
  typename T2,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::common_type_t<T1, T2>>, int> = 0>
POET_FORCEINLINE void dynamic_for(T1 begin, T2 end, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");

    using T = std::common_type_t<T1, T2>;
    detail::callable_storage_t<Func> callable(std::forward<Func>(func));

    detail::run_loop<Unroll, detail::wants_lane_v<std::remove_reference_t<Func>, T>>(
      static_cast<T>(begin), static_cast<T>(end), detail::static_stride<Step>{}, callable);
}

/// \brief Executes a runtime-sized loop, inferring the step direction (+1 when
/// `begin <= end`, -1 otherwise).
template<std::size_t Unroll,
  typename T1,
  typename T2,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::common_type_t<T1, T2>>, int> = 0>
POET_FORCEINLINE void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    const auto first = static_cast<T>(begin);
    const auto last = static_cast<T>(end);
    dynamic_for<Unroll>(first, last, first <= last ? static_cast<T>(1) : static_cast<T>(-1), std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop over `[0, count)`.
template<std::size_t Unroll,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::size_t>, int> = 0>
POET_FORCEINLINE void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll, 1>(std::size_t{ 0 }, count, std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop over `[0, count)`, passing loop-invariant
/// "hot" values to the callable by value instead of through a closure.
///
/// GCC does not scalar-replace a capturing lambda's closure that holds large
/// types (AVX-512 zmm values, say): the closure spills to the stack and
/// reloads once per iteration even with full inlining. Named by-value
/// parameters stay in registers.
///
/// This form requires at least one hot argument, so a zero-arg call still
/// selects the `(count, func)` overload.
///
/// \tparam Unroll Iterations per unrolled block.
/// \tparam Step Compile-time stride (must be non-zero).
/// \param count Iteration count, i.e. the range `[0, count)`.
/// \param func Callable `void(T index, HotArgs...)`. Do not also capture the
///   hot values; a capture would reintroduce the closure this form avoids.
/// \param args Loop-invariant values forwarded by value at each level.
template<std::size_t Unroll,
  std::ptrdiff_t Step = 1,
  typename Func,
  typename... Args,
  std::enable_if_t<(sizeof...(Args) >= 1)
                     && detail::is_df_callable_v<std::remove_reference_t<Func>, std::size_t, Args...>,
    int> = 0>
POET_FORCEINLINE void dynamic_for(std::size_t count, Func &&func, Args... args) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");

    detail::callable_storage_t<Func> callable(std::forward<Func>(func));

    detail::run_loop<Unroll, detail::wants_lane_v<std::remove_reference_t<Func>, std::size_t, Args...>>(
      std::size_t{ 0 }, count, detail::static_stride<Step>{}, callable, args...);
}

}// namespace poet


#if POET_CPLUSPLUS >= 202002L
#include <ranges>
#include <tuple>

namespace poet {

/// Holds the user callable for the `range | make_dynamic_for<N>(f)` form.
/// `Func` is deduced, `Unroll` is not, hence the ordering.
template<typename Func, std::size_t Unroll> struct dynamic_for_adaptor {
    Func func;
    constexpr explicit dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

/// Runs the callable over the range's elements.
///
/// Random access is required because the unrolled body indexes off `begin`
/// rather than advancing an iterator, which is also what keeps the lane
/// constants compile-time.
template<typename Func, std::size_t Unroll, std::ranges::random_access_range Range>
void operator|(Range &&r, dynamic_for_adaptor<Func, Unroll> const &ad) {
    const auto first = std::ranges::begin(r);
    const auto at = [first](std::size_t pos) -> decltype(auto) {
        return first[static_cast<std::ranges::range_difference_t<Range>>(pos)];
    };
    // O(1) when the sentinel can be subtracted, which random access usually
    // implies; `ranges::size` would reject views like `iota(0) | take(n)`.
    const auto count = static_cast<std::size_t>(std::ranges::distance(r));

    if constexpr (detail::wants_lane_v<Func, std::ranges::range_reference_t<Range>>) {
        dynamic_for<Unroll>(count, [&](auto lane, std::size_t pos) { ad.func(lane, at(pos)); });
    } else {
        dynamic_for<Unroll>(count, [&](std::size_t pos) { ad.func(at(pos)); });
    }
}

/// Tuple-like `(begin, end, step)` source.
template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
    const auto [b, e, s] = t;
    poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

/// Deduces `Func` so only `Unroll` has to be spelled out.
template<std::size_t U, typename F> constexpr auto make_dynamic_for(F &&f) -> dynamic_for_adaptor<std::decay_t<F>, U> {
    return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

}// namespace poet
#endif// POET_CPLUSPLUS >= 202002L
