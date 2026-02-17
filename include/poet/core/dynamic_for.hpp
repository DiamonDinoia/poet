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
/// `Unroll` iterations. Each chunk is fully unrolled at compile-time via
/// `static_for`, while the outer loop iterates at runtime. This provides:
/// - Amortized loop overhead (one branch per `Unroll` iterations)
/// - Better instruction-level parallelism (ILP) from unrolling
/// - Compiler visibility for vectorization and optimization
///
/// ### 2. Tail Dispatch (Compile-Time Dispatch Table)
/// After the main loop, 0 to `Unroll-1` iterations may remain. Instead of a
/// runtime loop, we use `poet::dispatch` over `DispatchParam<make_range<1, Unroll-1>>`
/// to route directly to a compile-time block specialization.
/// This keeps tail handling branch-free at call sites and maps each tail size
/// to a dedicated block implementation.
///
/// ### 3. Tiny Range Fast Path
/// For ranges smaller than `Unroll`, we skip the main runtime loop and dispatch
/// directly to a specialized tail block. This keeps lane information available
/// as compile-time constants and avoids extra loop-control overhead.
///
/// ## Optimization Design
///
/// The implementation minimizes abstraction layers between user code and the
/// actual loop body:
///
/// - **Callable form introspection**: The callable's signature (lane-by-value,
///   lane-by-template, or index-only) is detected once at template instantiation
///   via `detect_callable_form`, not per-iteration via `if constexpr`.
///
/// - **Fused block emission**: Unrolled blocks delegate to `static_for`
///   with lightweight invoker structs, reusing its register-pressure-aware
///   block isolation for large unroll factors.
///
/// - **Stride=1 specialization**: The ~90% common case (stride=1) gets a
///   dedicated implementation that eliminates stride multiplication.
///
/// - **Inline callable materialization**: Rvalue callables are materialized
///   directly in the public API without lambda indirection.
///
/// ## Choosing the Unroll Factor
///
/// `Unroll` is a required template parameter. Larger values increase code size
/// and compile-time work, while potentially reducing loop overhead in hot paths.
/// Typical starting points:
/// - `Unroll=4` for balanced behavior in general code.
/// - `Unroll=2` when compile time or code size is the primary constraint.
/// - `Unroll=8` for profiled hot loops with simple bodies.
/// - `Unroll=1` compiles to a plain loop with no dispatch overhead.

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

    // ========================================================================
    // Callable form detection — detect signature once, not per iteration
    // ========================================================================

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

    /// \brief Tag types for callable form dispatch.
    ///
    /// Each tag represents one of the three callable signatures. The tag is
    /// selected once at template instantiation via `detect_callable_form` and
    /// threaded through as a type parameter, enabling the compiler to resolve
    /// the correct invocation path via overloading — no enum or switch needed.
    struct lane_by_value_tag {};    ///< func(integral_constant<size_t, Lane>{}, index)
    struct lane_by_template_tag {}; ///< func.template operator()<Lane>(index)
    struct index_only_tag {};       ///< func(index)

    /// \brief Detects the callable form at template instantiation time.
    ///
    /// Returns the appropriate tag type. Uses Lane=0 as representative —
    /// the form is the same for all lanes.
    template<typename Func, typename T>
    constexpr auto detect_callable_form() {
        if constexpr (std::is_invocable_v<Func &, std::integral_constant<std::size_t, 0>, T>) {
            return lane_by_value_tag{};
        } else if constexpr (has_template_lane_with_index_v<Func, T, 0>) {
            return lane_by_template_tag{};
        } else if constexpr (std::is_invocable_v<Func &, T>) {
            return index_only_tag{};
        } else {
            static_assert(
                always_false_v<Func>,
                "dynamic_for callable must accept (lane, index), template<lane>(index), or (index)");
            return index_only_tag{};
        }
    }

    /// \brief Type alias for the detected callable form tag.
    template<typename Func, typename T>
    using callable_form_t = decltype(detect_callable_form<Func, T>());

    // ========================================================================
    // Specialized invokers — overloaded on tag, zero per-call branching
    // ========================================================================

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(lane_by_value_tag /*tag*/, Func &func, T index) {
        func(std::integral_constant<std::size_t, Lane>{}, index);
    }

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(lane_by_template_tag /*tag*/, Func &func, T index) {
        func.template operator()<Lane>(index);
    }

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(index_only_tag /*tag*/, Func &func, T index) {
        func(index);
    }

    // ========================================================================
    // Block invokers — adapter structs for static_for unrolling
    // ========================================================================

    /// \brief Invoker for unrolled blocks with arbitrary stride.
    ///
    /// Adapts the tag-dispatched invoke_lane interface for use with static_for.
    /// Each lane computes index = base + Lane * stride.
    template<typename FormTag, typename Callable, typename T>
    struct block_invoker {
        Callable &callable;
        T base;
        T stride;

        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            invoke_lane<static_cast<std::size_t>(Value)>(FormTag{}, callable,
                base + static_cast<T>(Value) * stride);
        }
    };

    /// \brief Stride=1 invoker — eliminates per-lane multiplication.
    template<typename FormTag, typename Callable, typename T>
    struct block_invoker_stride1 {
        Callable &callable;
        T base;

        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            invoke_lane<static_cast<std::size_t>(Value)>(FormTag{}, callable,
                base + static_cast<T>(Value));
        }
    };

    /// \brief Compile-time stride invoker — stride baked into template parameter.
    template<std::intmax_t Step, typename FormTag, typename Callable, typename T>
    struct block_invoker_ct_stride {
        Callable &callable;
        T base;

        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            invoke_lane<static_cast<std::size_t>(Value)>(FormTag{}, callable,
                base + static_cast<T>(Value * Step));
        }
    };

    // ========================================================================
    // Block execution — delegates to static_for for register-aware unrolling
    // ========================================================================

    /// \brief Executes an unrolled block via static_for with arbitrary stride.
    template<typename FormTag, typename Callable, typename T, std::size_t BlockSize>
    POET_FORCEINLINE constexpr void execute_block([[maybe_unused]] FormTag /*tag*/,
                                                  [[maybe_unused]] Callable &callable,
                                                  [[maybe_unused]] T base,
                                                  [[maybe_unused]] T stride) {
        if constexpr (BlockSize > 0) {
            block_invoker<FormTag, Callable, T> invoker{callable, base, stride};
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        }
    }

    /// \brief Stride=1 variant — eliminates stride multiplication.
    template<typename FormTag, typename Callable, typename T, std::size_t BlockSize>
    POET_FORCEINLINE constexpr void execute_block_stride1([[maybe_unused]] FormTag /*tag*/,
                                                          [[maybe_unused]] Callable &callable,
                                                          [[maybe_unused]] T base) {
        if constexpr (BlockSize > 0) {
            block_invoker_stride1<FormTag, Callable, T> invoker{callable, base};
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        }
    }

    /// \brief Compile-time stride variant.
    template<std::intmax_t Step, typename FormTag, typename Callable, typename T, std::size_t BlockSize>
    POET_FORCEINLINE constexpr void execute_block_ct_stride([[maybe_unused]] FormTag /*tag*/,
                                                            [[maybe_unused]] Callable &callable,
                                                            [[maybe_unused]] T base) {
        if constexpr (BlockSize > 0) {
            block_invoker_ct_stride<Step, FormTag, Callable, T> invoker{callable, base};
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        }
    }

    // ========================================================================
    // Tail dispatch — reuses poet::dispatch with fused block emission
    // ========================================================================

    /// \brief Stateless functor for tail dispatch — args passed through dispatch.
    ///
    /// Empty struct (is_stateless_v = true) so dispatch eliminates the functor
    /// pointer from the function pointer table. The callable pointer, index, and
    /// stride are passed as dispatch arguments and stay in registers.
    template<typename FormTag, typename Callable, typename T>
    struct tail_dispatch_functor {
        template<int N>
        POET_FORCEINLINE void operator()(Callable *callable, T index, T stride) const {
            if constexpr (N > 0) {
                execute_block<FormTag, Callable, T, static_cast<std::size_t>(N)>(
                    FormTag{}, *callable, index, stride);
            }
        }
    };

    /// \brief Stride=1 variant — eliminates stride argument and per-lane multiplication.
    template<typename FormTag, typename Callable, typename T>
    struct tail_dispatch_functor_stride1 {
        template<int N>
        POET_FORCEINLINE void operator()(Callable *callable, T index) const {
            if constexpr (N > 0) {
                execute_block_stride1<FormTag, Callable, T, static_cast<std::size_t>(N)>(
                    FormTag{}, *callable, index);
            }
        }
    };

    template<typename FormTag, std::size_t Unroll, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_tail(std::size_t count, Callable &callable, T index, T stride) {
        static_assert(Unroll > 1, "dispatch_tail requires Unroll > 1");
        if (count == 0) { return; }
        const tail_dispatch_functor<FormTag, Callable, T> functor{};
        const T c_index = index;
        const T c_stride = stride;
        poet::dispatch(
            functor,
            poet::DispatchParam<poet::make_range<1, static_cast<int>(Unroll - 1)>>{
                static_cast<int>(count)},
            std::addressof(callable), c_index, c_stride);
    }

    template<typename FormTag, std::size_t Unroll, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_tail_stride1(std::size_t count, Callable &callable, T index) {
        static_assert(Unroll > 1, "dispatch_tail_stride1 requires Unroll > 1");
        if (count == 0) { return; }
        const tail_dispatch_functor_stride1<FormTag, Callable, T> functor{};
        const T c_index = index;
        poet::dispatch(
            functor,
            poet::DispatchParam<poet::make_range<1, static_cast<int>(Unroll - 1)>>{
                static_cast<int>(count)},
            std::addressof(callable), c_index);
    }

    /// \brief Compile-time stride variant — stride baked into the functor type.
    template<std::intmax_t Step, typename FormTag, typename Callable, typename T>
    struct tail_dispatch_functor_ct_stride {
        template<int N>
        POET_FORCEINLINE void operator()(Callable *callable, T index) const {
            if constexpr (N > 0) {
                execute_block_ct_stride<Step, FormTag, Callable, T, static_cast<std::size_t>(N)>(
                    FormTag{}, *callable, index);
            }
        }
    };

    template<std::intmax_t Step, typename FormTag, std::size_t Unroll, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_tail_ct_stride(std::size_t count, Callable &callable, T index) {
        static_assert(Unroll > 1, "dispatch_tail_ct_stride requires Unroll > 1");
        if (count == 0) { return; }
        const tail_dispatch_functor_ct_stride<Step, FormTag, Callable, T> functor{};
        const T c_index = index;
        poet::dispatch(
            functor,
            poet::DispatchParam<poet::make_range<1, static_cast<int>(Unroll - 1)>>{
                static_cast<int>(count)},
            std::addressof(callable), c_index);
    }

    // ========================================================================
    // Iteration count calculation (unchanged from previous implementation)
    // ========================================================================

    /// \brief Calculate iteration count for non-trivial strides (stride != 1).
    ///
    /// Handles three cases:
    /// 1. Backward iteration (stride < 0 or wrapped negative for unsigned)
    /// 2. Power-of-2 forward stride (uses bit shift instead of division)
    /// 3. General forward stride (uses division)
    template<typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_complex(T begin, T end, T stride) -> std::size_t {
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (POET_UNLIKELY(stride < 0 || is_wrapped_negative)) {
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

    // ========================================================================
    // Fused implementation: stride=1 specialization (~90% of calls)
    // ========================================================================

    POET_PUSH_OPTIMIZE

    /// \brief Fused dynamic_for implementation for stride=1.
    ///
    /// Eliminates stride multiplication in the hot loop. The callable form
    /// is baked into the template parameter, so there's no per-iteration
    /// signature detection.
    template<typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP POET_FLATTEN
    void dynamic_for_impl_stride1(T begin, T end, Callable &callable, FormTag tag) {
        if (POET_UNLIKELY(begin >= end)) { return; }

        if constexpr (Unroll == 1) {
            // Plain loop — no dispatch, no tail, no blocks.
            for (T i = begin; i < end; ++i) {
                invoke_lane<0>(tag, callable, i);
            }
        } else {
            std::size_t count = static_cast<std::size_t>(end - begin);
            T index = begin;
            std::size_t remaining = count;

            // Tiny ranges: dispatch directly to a compile-time block.
            if (POET_UNLIKELY(count < Unroll)) {
                dispatch_tail_stride1<FormTag, Unroll>(count, callable, index);
                return;
            }

            // Main unrolled loop — no stride multiplication.
            while (remaining >= Unroll) {
                execute_block_stride1<FormTag, Callable, T, Unroll>(tag, callable, index);
                index += static_cast<T>(Unroll);
                remaining -= Unroll;
            }

            // Tail dispatch for remaining iterations.
            dispatch_tail_stride1<FormTag, Unroll>(remaining, callable, index);
        }
    }

    // ========================================================================
    // Fused implementation: general stride (handles stride != 1, including
    // negative strides and stride > 1)
    // ========================================================================

    /// \brief Fused dynamic_for implementation for arbitrary stride.
    ///
    /// Handles all non-unit strides including negative, power-of-2, and general.
    /// The callable form is baked into the template parameter.
    template<typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP POET_FLATTEN
    void dynamic_for_impl_general(T begin, T end, T stride, Callable &callable, FormTag tag) {
        if (POET_UNLIKELY(stride == 0)) { return; }

        std::size_t count = calculate_iteration_count_complex(begin, end, stride);
        if (POET_UNLIKELY(count == 0)) { return; }

        if constexpr (Unroll == 1) {
            // Plain loop — no dispatch, no tail, no blocks.
            T index = begin;
            for (std::size_t i = 0; i < count; ++i) {
                invoke_lane<0>(tag, callable, index);
                index += stride;
            }
        } else {
            T index = begin;
            std::size_t remaining = count;

            // Tiny ranges: dispatch directly to a compile-time block.
            if (POET_UNLIKELY(count < Unroll)) {
                dispatch_tail<FormTag, Unroll>(count, callable, index, stride);
                return;
            }

            // Main unrolled loop with stride arithmetic.
            const T stride_times_unroll = static_cast<T>(Unroll) * stride;
            while (remaining >= Unroll) {
                execute_block<FormTag, Callable, T, Unroll>(tag, callable, index, stride);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            // Tail dispatch for remaining iterations.
            dispatch_tail<FormTag, Unroll>(remaining, callable, index, stride);
        }
    }

    // ========================================================================
    // Fused implementation: compile-time stride
    // ========================================================================

    /// \brief Calculate iteration count when stride is known at compile time.
    ///
    /// Uses `if constexpr` on the sign of Step to eliminate runtime branches
    /// for stride direction. The compiler can also constant-fold power-of-2
    /// divisions since the stride is a template parameter.
    template<std::intmax_t Step, typename T>
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

    /// \brief Fused dynamic_for implementation for compile-time stride.
    ///
    /// The stride is a template parameter, so:
    /// - Per-lane multiplication uses compile-time constants
    /// - Tail dispatch passes no stride argument (baked into functor type)
    /// - Iteration count eliminates stride-sign branches at compile time
    template<std::intmax_t Step, typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP POET_FLATTEN
    void dynamic_for_impl_ct_stride(T begin, T end, Callable &callable, FormTag tag) {
        std::size_t count = calculate_iteration_count_ct<Step>(begin, end);
        if (POET_UNLIKELY(count == 0)) { return; }

        if constexpr (Unroll == 1) {
            // Plain loop — no dispatch, no tail, no blocks.
            T index = begin;
            constexpr T ct_stride = static_cast<T>(Step);
            for (std::size_t i = 0; i < count; ++i) {
                invoke_lane<0>(tag, callable, index);
                index += ct_stride;
            }
        } else {
            T index = begin;
            std::size_t remaining = count;

            // Tiny ranges: dispatch directly to a compile-time block.
            if (POET_UNLIKELY(count < Unroll)) {
                dispatch_tail_ct_stride<Step, FormTag, Unroll>(count, callable, index);
                return;
            }

            // Main unrolled loop — stride multiplication is compile-time.
            constexpr T stride_times_unroll = static_cast<T>(static_cast<std::intmax_t>(Unroll) * Step);
            while (remaining >= Unroll) {
                execute_block_ct_stride<Step, FormTag, Callable, T, Unroll>(tag, callable, index);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            // Tail dispatch for remaining iterations.
            dispatch_tail_ct_stride<Step, FormTag, Unroll>(remaining, callable, index);
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// ============================================================================
// Public API
// ============================================================================

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// The helper iterates over the half-open range `[begin, end)` with a given
/// `step`. Blocks of `Unroll` iterations are emitted via compile-time unrolling.
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \param begin Inclusive lower/start bound.
/// \param end Exclusive upper/end bound.
/// \param step Increment per iteration. Can be negative.
/// \param func Callable invoked for each iteration. Supports three forms:
///   - `func(std::integral_constant<std::size_t, lane>{}, index)` — lane as type
///   - `func.template operator()<lane>(index)` — template method form
///   - `func(index)` — fallback without lane info

/// \brief Choose `Unroll` explicitly per call site.
///
/// `dynamic_for` does not provide a default unroll factor. Callers choose
/// `Unroll` based on their own code-size, compile-time, and runtime trade-offs.
/// Typical starting points are:
/// - `Unroll=4` for balanced behavior in general code.
/// - `Unroll=2` when compile time/code size is the priority.
/// - `Unroll=8` for profiled hot loops with simple bodies.
template<std::size_t Unroll, typename T1, typename T2, typename T3, typename Func>
// POET_FORCEINLINE: Ensures the public API is always inlined at the call site,
// giving the compiler full visibility of the loop structure for optimization.
// POET_FLATTEN: When enabled, additionally inlines all callees for unified
// register allocation across the entire call chain.
POET_FORCEINLINE POET_FLATTEN constexpr void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2, T3>;
    const T s = static_cast<T>(step);

    // Inline callable materialization — no lambda indirection.
    // Detect callable form once at instantiation via tag type, not per iteration.
    if constexpr (std::is_lvalue_reference_v<Func>) {
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;

        if (s == static_cast<T>(1)) {
            detail::dynamic_for_impl_stride1<T, callable_t, Unroll>(
                static_cast<T>(begin), static_cast<T>(end), func, form_tag{});
        } else {
            detail::dynamic_for_impl_general<T, callable_t, Unroll>(
                static_cast<T>(begin), static_cast<T>(end), s, func, form_tag{});
        }
    } else {
        std::remove_reference_t<Func> callable(std::forward<Func>(func));
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;

        if (s == static_cast<T>(1)) {
            detail::dynamic_for_impl_stride1<T, callable_t, Unroll>(
                static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
        } else {
            detail::dynamic_for_impl_general<T, callable_t, Unroll>(
                static_cast<T>(begin), static_cast<T>(end), s, callable, form_tag{});
        }
    }
}

/// \brief Executes a runtime-sized loop with compile-time stride.
///
/// The stride is a template parameter, enabling the compiler to:
/// - Replace per-lane stride multiplication with compile-time constants
/// - Eliminate stride from tail dispatch arguments (one fewer register)
/// - Constant-fold stride-direction branches in iteration count
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \tparam Step Compile-time stride (must be non-zero).
/// \param begin Inclusive start bound.
/// \param end Exclusive end bound.
/// \param func Callable invoked for each iteration.
template<std::size_t Unroll, std::intmax_t Step, typename T1, typename T2, typename Func>
POET_FORCEINLINE POET_FLATTEN constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2>;

    if constexpr (std::is_lvalue_reference_v<Func>) {
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        detail::dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
            static_cast<T>(begin), static_cast<T>(end), func, form_tag{});
    } else {
        std::remove_reference_t<Func> callable(std::forward<Func>(func));
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        detail::dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
            static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
    }
}

/// \brief Executes a runtime-sized loop using compile-time unrolling with
/// automatic step detection.
///
/// If `begin <= end`, step is +1.
/// If `begin > end`, step is -1.
template<std::size_t Unroll, typename T1, typename T2, typename Func>
POET_FORCEINLINE POET_FLATTEN constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
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
template<std::size_t Unroll, typename Func> POET_FORCEINLINE constexpr void dynamic_for(std::size_t count, Func &&func) {
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
