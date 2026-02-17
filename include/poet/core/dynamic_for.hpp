#ifndef POET_CORE_DYNAMIC_FOR_HPP
#define POET_CORE_DYNAMIC_FOR_HPP

/// \file dynamic_for.hpp
/// \brief Provides runtime loop execution with compile-time unrolling.
///
/// This header defines `dynamic_for`, a utility that allows executing a loop
/// where the range is determined at runtime, but the body is unrolled at
/// compile-time. This is achieved by emitting unrolled blocks and a runtime
/// loop over those blocks.
///
/// ## Architecture Overview
///
/// `dynamic_for` uses a three-tier execution strategy to balance performance,
/// code size, and compile-time cost:
///
/// ### 1. Main Loop (Unrolled Blocks)
/// For ranges larger than the unroll factor, the loop executes in chunks of
/// `Unroll` iterations. Each chunk is fully unrolled at compile-time using
/// `static_for`, while the outer loop iterates at runtime. This provides:
/// - Amortized loop overhead (one branch per `Unroll` iterations)
/// - Better instruction-level parallelism (ILP) from unrolling
/// - Compiler visibility for vectorization and optimization
///
/// ### 2. Tail Dispatch (Compile-Time Dispatch Table)
/// After the main loop, 0 to `Unroll-1` iterations may remain. Instead of a
/// runtime loop, we use `poet::dispatch` over `DispatchParam<make_range<1, Unroll-1>>`
/// to route directly to a compile-time `execute_runtime_block<N>()` specialization.
/// This keeps tail handling branch-free at call sites and maps each tail size
/// to a dedicated block implementation.
///
/// ### 3. Tiny Range Fast Path
/// For ranges smaller than `Unroll`, we skip the main runtime loop and dispatch
/// directly to a specialized tail block. This keeps lane information available
/// as compile-time constants and avoids extra loop-control overhead.
///
/// ## Choosing the Unroll Factor
///
/// `Unroll` is a required template parameter. Larger values increase code size
/// and compile-time work, while potentially reducing loop overhead in hot paths.
/// Typical starting points:
/// - `Unroll=4` for balanced behavior in general code.
/// - `Unroll=2` when compile time or code size is the primary constraint.
/// - `Unroll=8` for profiled hot loops with simple bodies.

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>
#include <poet/core/static_dispatch.hpp>
#include <poet/core/static_for.hpp>

namespace poet {

namespace detail {

    template<typename...>
    inline constexpr bool always_false_v = false;

    template<typename Func, typename T, std::size_t Lane, typename = void>
    struct has_template_lane_with_index : std::false_type {};

    template<typename Func, typename T, std::size_t Lane>
    struct has_template_lane_with_index<
      Func,
      T,
      Lane,
      std::void_t<decltype(std::declval<Func &>().template operator()<Lane>(std::declval<T>()))>> : std::true_type {};

    template<typename Func, typename T, std::size_t Lane>
    inline constexpr bool has_template_lane_with_index_v = has_template_lane_with_index<Func, T, Lane>::value;

    // Invoke callable with the best available form:
    // 1) func(std::integral_constant<std::size_t, Lane>{}, index)
    // 2) func.template operator()<Lane>(index)
    // 3) func(index) (backward compatible fallback)
    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_dynamic_for_callable(Func &func, T index) {
        if constexpr (std::is_invocable_v<Func &, std::integral_constant<std::size_t, Lane>, T>) {
            func(std::integral_constant<std::size_t, Lane>{}, index);
        } else if constexpr (has_template_lane_with_index_v<Func, T, Lane>) {
            func.template operator()<Lane>(index);
        } else if constexpr (std::is_invocable_v<Func &, T>) {
            func(index);
        } else {
            static_assert(
              always_false_v<Func>,
              "dynamic_for callable must accept (lane, index), template<lane>(index), or (index)");
        }
    }

    template<typename Func, typename T, std::size_t BlockSize>
    POET_HOT_LOOP void execute_runtime_block(Func * POET_RESTRICT func, T base, T stride);

    /// \brief Helper functor that adapts a user lambda for use with static_for.
    ///
    /// This struct wraps a runtime function pointer/reference along with base and
    /// stride values. It exposes a compile-time call operator that calculates the
    /// actual runtime index based on the compile-time offset `I` and invokes the
    /// user function.
    ///
    /// This class is invoked via `static_for` using an integral_constant index.
    /// It maps each compile-time lane to a runtime index and forwards to the user callable.
    ///
    /// **POET_RESTRICT on func pointer**:
    /// The restrict qualifier tells the compiler that `func` is the only pointer
    /// accessing the callable object, enabling better register allocation for the
    /// function pointer itself. This is safe because each invoker owns its func
    /// pointer for the duration of the block execution.
    ///
    /// \tparam Func Type of the user-provided callable.
    /// \tparam T Type of the loop counter (e.g., int, size_t).
    template<typename Func, typename T> struct dynamic_block_invoker {
        Func * POET_RESTRICT func;  ///< Pointer to user callable (restrict for better codegen)
        T base;                      ///< Base index for this block
        T stride;                    ///< Stride between iterations

        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            POET_ASSUME_NOT_NULL(func);
            constexpr auto lane = static_cast<std::size_t>(Value);
            invoke_dynamic_for_callable<lane>(*func, base + (static_cast<T>(Value) * stride));
        }

    };

    template<typename Callable, typename T>
    struct dynamic_tail_dispatch_functor {
        Callable *callable;
        T index;
        T stride;

        template<int N>
        POET_FORCEINLINE void operator()() const {
            if constexpr (N > 0) {
                execute_runtime_block<Callable, T, static_cast<std::size_t>(N)>(std::addressof(*callable), index, stride);
            }
        }
    };

    template<std::size_t Unroll, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_runtime_tail(std::size_t count, Callable &callable, T index, T stride) {
        if (count == 0) {
            return;
        }
        if constexpr (Unroll == 1) {
            execute_runtime_block<Callable, T, 1>(std::addressof(callable), index, stride);
        } else {
            dynamic_tail_dispatch_functor<Callable, T> tail_functor{ std::addressof(callable), index, stride };
            poet::dispatch(
              tail_functor,
              poet::DispatchParam<poet::make_range<1, static_cast<int>(Unroll - 1)>>{ static_cast<int>(count) });
        }
    }

    /// \brief Calculate iteration count for non-trivial strides (stride != 1).
    ///
    /// Handles three cases with different computational strategies:
    ///
    /// **Case 1: Backward iteration** (stride < 0 or wrapped negative for unsigned)
    /// - Check: stride < 0, or for unsigned: stride > max/2
    /// - Formula: ceil((begin - end) / abs(stride))
    /// - Example: begin=10, end=0, stride=-2 -> count=5 (10,8,6,4,2)
    /// - Wrapped unsigned: stride=UINT_MAX (-1 as unsigned) is backward
    ///
    /// **Case 2: Power-of-2 stride** (forward, stride is power of 2)
    /// - Check: (stride & (stride-1)) == 0
    /// - Formula: ceil((end - begin) / stride) using bit shift
    /// - Optimization: Division by power-of-2 becomes right shift
    /// - Example: stride=4 -> shift=2, division becomes >> 2
    /// - Benefit: ~2-3x faster than division on most architectures
    ///
    /// **Case 3: General forward stride**
    /// - Check: stride > 1, not power of 2
    /// - Formula: ceil((end - begin) / stride) using division
    /// - Example: begin=0, end=10, stride=3 -> count=4 (0,3,6,9)
    ///
    /// \tparam T Loop counter type (signed or unsigned integer)
    /// \param begin Inclusive start of range
    /// \param end Exclusive end of range
    /// \param stride Step value (positive or negative, never 1 or 0)
    /// \return Number of iterations needed
    template<typename T>
    POET_FORCEINLINE auto calculate_iteration_count_complex(T begin, T end, T stride) -> std::size_t {
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (POET_UNLIKELY(stride < 0 || is_wrapped_negative)) {
            // Backward iteration
            if (POET_UNLIKELY(begin <= end)) {
                return 0;
            }
            T abs_stride;
            if constexpr (std::is_signed_v<T>) {
                abs_stride = static_cast<T>(-stride);
            } else {
                abs_stride = static_cast<T>(0) - stride;
            }
            auto dist = static_cast<std::size_t>(begin - end);
            auto ustride = static_cast<std::size_t>(abs_stride);
            return (dist + ustride - 1) / ustride;
        }

        // Forward iteration with stride > 1
        if (POET_UNLIKELY(begin >= end)) {
            return 0;
        }

        auto dist = static_cast<std::size_t>(end - begin);
        auto ustride = static_cast<std::size_t>(stride);
        const bool is_power_of_2 = (ustride & (ustride - 1)) == 0;

        if (POET_LIKELY(is_power_of_2)) {
            const unsigned int shift = poet_count_trailing_zeros(ustride);
            return (dist + ustride - 1) >> shift;
        }
        return (dist + ustride - 1) / ustride;
    }

    /// \brief Emits a block of unrolled iterations.
    ///
    /// Generates `BlockSize` calls to the user function with consecutive indices.
    /// This helper is used for both the main unrolled loop body and the tail
    /// handling. Each call uses a compile-time index offset, allowing the compiler
    /// to optimize the entire block as a single unit.
    ///
    /// **Pointer parameter design**:
    /// Takes `func` as a pointer (instead of reference) to enable POET_RESTRICT,
    /// which tells the compiler this pointer is the only way to access the
    /// callable. This enables:
    /// - Better register allocation for the function pointer itself
    /// - More aggressive inlining of the callable
    /// - Elimination of redundant loads in the unrolled loop
    ///
    /// **Register allocation benefits**:
    /// The restrict qualifier combined with POET_HOT_LOOP allows the compiler to:
    /// 1. Keep `func` pointer in a register across all `BlockSize` calls
    /// 2. Hoist callable object member accesses out of the unroll
    /// 3. Optimize away parameter passing overhead within the block
    ///
    /// **Compiler optimization note**:
    /// With POET_HOT_LOOP + POET_RESTRICT, GCC/Clang typically inline this
    /// function completely, then inline all `BlockSize` calls to `(*func)(...)`,
    /// resulting in a single straight-line sequence of user code with no function
    /// call overhead.
    ///
    /// \tparam Func User callable type.
    /// \tparam T Loop counter type.
    /// \tparam BlockSize Number of iterations to unroll in this block.
    /// \param func Pointer to user callable (restrict-qualified for better codegen)
    /// \param base Starting index for this block
    /// \param stride Increment between consecutive iterations
    template<typename Func, typename T, std::size_t BlockSize>
    POET_HOT_LOOP void execute_runtime_block([[maybe_unused]] Func * POET_RESTRICT func, [[maybe_unused]] T base, [[maybe_unused]] T stride) {
        POET_ASSUME_NOT_NULL(func);
        if constexpr (BlockSize > 0) {
            dynamic_block_invoker<Func, T> invoker{ func, base, stride };
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        }
    }

    /// \brief Internal implementation of the dynamic loop.
    ///
    /// Calculates the total number of iterations required based on proper signed
    /// arithmetic and executes the loop in chunks of `Unroll`. Any remaining
    /// iterations are handled by `poet::dispatch`.
    ///
    /// \param begin Inclusive start of the iteration range.
    /// \param end Exclusive end of the iteration range.
    /// \param stride Step/increment value (can be negative for backward iteration).
    /// \param callable User-provided function to invoke for each index.
    POET_PUSH_OPTIMIZE
    template<typename T, typename Callable, std::size_t Unroll>
    // POET_FLATTEN: Inline all helper functions into this impl for unified register
    // allocation across calculate_iteration_count_complex, execute_runtime_block,
    // and match_and_execute_tail. This enables the compiler to see the full picture
    // and optimize register allocation across the entire loop structure.
    POET_HOT_LOOP POET_FLATTEN void dynamic_for_impl(T begin, T end, T stride, Callable &callable) {
        if (POET_UNLIKELY(stride == 0)) { return; }

        // Calculate iteration count.
        // We determine the number of steps to go from `begin` to `end` exclusively.
        std::size_t count = 0;

        // Super-fast path: stride == 1 (most common case ~90%)
        if (stride == 1 && begin < end) {
            count = static_cast<std::size_t>(end - begin);
        } else if (POET_UNLIKELY(stride == 1)) {
            // Empty range with stride 1
            return;
        } else {
            // Complex path for stride != 1 - delegate to helper function
            count = calculate_iteration_count_complex(begin, end, stride);
        }

        T index = begin;
        std::size_t remaining = count;

        // Tiny ranges: dispatch directly to a compile-time block so lane is
        // available as an integral_constant in user callables.
        if (POET_UNLIKELY(count < Unroll)) {
            dispatch_runtime_tail<Unroll>(count, callable, index, stride);
            return;
        }

        // Execute full blocks of size 'Unroll'.
        // We use a runtime while loop here, but the body (execute_runtime_block)
        // is fully unrolled at compile-time for 'Unroll' iterations.
        // Optimization: Hoist loop-invariant multiplication out of the loop.
        const T stride_times_unroll = static_cast<T>(Unroll) * stride;
        while (remaining >= Unroll) {
            detail::execute_runtime_block<Callable, T, Unroll>(std::addressof(callable), index, stride);
            index += stride_times_unroll;
            remaining -= Unroll;
        }

        // Handle remaining iterations (tail) via poet::dispatch.
        dispatch_runtime_tail<Unroll>(remaining, callable, index, stride);
    }

    POET_POP_OPTIMIZE

}// namespace detail

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// The helper iterates over the half-open range `[begin, end)` with a given
/// `step`. Blocks of `Unroll` iterations are emitted via compile-time unrolling.
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \param begin Inclusive lower/start bound.
/// \param end Exclusive upper/end bound.
/// \param step Increment per iteration. Can be negative.
/// \param func Callable invoked for each iteration.

/// \brief Choose `Unroll` explicitly per call site.
///
/// `dynamic_for` does not provide a default unroll factor. Callers choose
/// `Unroll` based on their own code-size, compile-time, and runtime trade-offs.
/// Typical starting points are:
/// - `Unroll=4` for balanced behavior in general code.
/// - `Unroll=2` when compile time/code size is the priority.
/// - `Unroll=8` for profiled hot loops with simple bodies.
template<std::size_t Unroll, typename T1, typename T2, typename T3, typename Func>
// POET_FLATTEN on public API: Enables cross-translation-unit inlining of the
// entire dynamic_for call chain into the caller. Without this, callers from
// other TUs would see only a function call, losing optimization opportunities.
// With POET_FLATTEN, the compiler can inline dynamic_for_impl and all helpers,
// enabling full optimization context at the call site.
inline POET_FLATTEN void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2, T3>;
    detail::with_stored_callable(std::forward<Func>(func), [&](auto &callable) -> void {
        using callable_t = std::remove_reference_t<decltype(callable)>;
        detail::dynamic_for_impl<T, callable_t, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), static_cast<T>(step), callable);
    });
}

/// \brief Executes a runtime-sized loop using compile-time unrolling with
/// automatic step detection.
///
/// If `begin <= end`, step is +1.
/// If `begin > end`, step is -1.
template<std::size_t Unroll, typename T1, typename T2, typename Func>
// POET_FLATTEN: Same reasoning as above - enable cross-TU inlining for this
// convenience overload that auto-detects step direction.
inline POET_FLATTEN void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    T s_begin = static_cast<T>(begin);
    T s_end = static_cast<T>(end);
    T step = (s_begin <= s_end) ? static_cast<T>(1) : static_cast<T>(-1);

    // Call general overload
    dynamic_for<Unroll>(s_begin, s_end, step, std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop from zero using compile-time unrolling.
///
/// This overload iterates over the range `[0, count)`.
template<std::size_t Unroll, typename Func> inline void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll>(static_cast<std::size_t>(0), count, std::forward<Func>(func));
}

} // namespace poet


#if __cplusplus >= 202002L
#include <ranges>
#include <tuple>
#include <cstddef>

namespace poet {

// Adaptor holds the user callable.
// Template ordering: Func first (deduced), Unroll second (required).
template<typename Func, std::size_t Unroll>
struct dynamic_for_adaptor {
  Func func;
  constexpr explicit dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

// Range overload: accept any std::ranges::range.
// Interprets the range as a sequence of consecutive indices starting at *begin(range).
// This implementation computes the distance by iterating the range (works even when not sized).
template<typename Func, std::size_t Unroll, typename Range>
  requires std::ranges::range<Range>
void operator|(Range &&r, dynamic_for_adaptor<Func, Unroll> const &ad) {
  auto it = std::ranges::begin(r);
  auto it_end = std::ranges::end(r);

  if (it == it_end) return; // empty range

  using ValT = std::remove_reference_t<decltype(*it)>;
  ValT start = *it;

  std::size_t count = 0;
  for (auto jt = it; jt != it_end; ++jt) ++count;

  // Call dynamic_for with [start, start+count) using step = +1
  poet::dynamic_for<Unroll>(start, static_cast<ValT>(start + static_cast<ValT>(count)), ad.func);
}

// Tuple overload: accept tuple-like (begin, end, step)
template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
  auto [b, e, s] = t;
  poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

// Helper to construct adaptor with type deduction
template<std::size_t U, typename F>
constexpr auto make_dynamic_for(F &&f) -> dynamic_for_adaptor<std::decay_t<F>, U> {
  return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

} // namespace poet
#endif // __cplusplus >= 202002L

#endif// POET_CORE_DYNAMIC_FOR_HPP
