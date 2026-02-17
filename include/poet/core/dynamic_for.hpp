#ifndef POET_CORE_DYNAMIC_FOR_HPP
#define POET_CORE_DYNAMIC_FOR_HPP

/// \file dynamic_for.hpp
/// \brief Provides runtime loop execution with compile-time unrolling.
///
/// This header defines `dynamic_for`, a utility that allows executing a loop
/// where the range is determined at runtime, but the body is unrolled at
/// compile-time. This is achieved by emitting unrolled blocks and a runtime
/// loop over those blocks.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>
#include <poet/core/static_for.hpp>

namespace poet {

namespace detail {

    /// \brief Helper functor that adapts a user lambda for use with static_for.
    ///
    /// This struct wraps a runtime function pointer/reference along with base and
    /// stride values. It exposes a compile-time call operator that calculates the
    /// actual runtime index based on the compile-time offset `I` and invokes the
    /// user function.
    ///
    /// \tparam Func Type of the user-provided callable.
    /// \tparam T Type of the loop counter (e.g., int, size_t).
    template<typename Func, typename T> struct dynamic_block_invoker {
        Func * POET_RESTRICT func;
        T base;
        T stride;

        // Direct compile-time integral_constant invocation used by `static_for`'s
        // direct-invocation path. This avoids the extra `template_static_loop_
        // invoker` adapter and lets the compiler call this functor directly
        // with an `std::integral_constant` index.
        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            (*func)(base + (static_cast<T>(Value) * stride));
        }

        // Preserve the template<auto I> operator<> overload to support other
        // call sites that expect the template operator form.
        template<auto I>
        POET_FORCEINLINE constexpr void operator()() const {
            (*func)(base + (static_cast<T>(I) * stride));
        }
    };

    /// \brief Calculate iteration count for non-trivial strides (helper to reduce cognitive complexity).
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
    /// Emits `BlockSize` calls to the user function.
    /// This helper is used for both the main unrolled loop body and the
    /// tail handling.
    ///
    /// \tparam Func User callable type.
    /// \tparam T Loop counter type.
    /// \tparam BlockSize Number of iterations to unroll in this block.
    template<typename Func, typename T, std::size_t BlockSize>
    POET_HOT_LOOP void execute_runtime_block([[maybe_unused]] Func * POET_RESTRICT func, [[maybe_unused]] T base, [[maybe_unused]] T stride) {
        if constexpr (BlockSize > 0) {
            dynamic_block_invoker<Func, T> invoker{ func, base, stride };
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        }
    }

    template<std::size_t N, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_tail_impl(std::size_t remaining, Callable &callable, T index, T stride) {
        if constexpr (N == 0) {
            return;
        } else {
            if (remaining == N) {
                execute_runtime_block<Callable, T, N>(std::addressof(callable), index, stride);
                return;
            }
            if constexpr (N > 1) {
                dispatch_tail_impl<N - 1>(remaining, callable, index, stride);
            }
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

        // Fast path for tiny ranges: avoid any tail dispatch overhead.
        if (POET_UNLIKELY(count < Unroll)) {
            for (std::size_t i = 0; i < count; ++i) {
                callable(index);
                index += stride;
            }
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

        // Handle remaining iterations (tail) using a fully inlined dispatch chain.
        if constexpr (Unroll > 1) {
            if (POET_UNLIKELY(remaining > 0)) {
                detail::dispatch_tail_impl<Unroll - 1>(remaining, callable, index, stride);
            }
        }
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
constexpr std::size_t kDefaultUnroll = 4;

template<std::size_t Unroll = kDefaultUnroll, typename T1, typename T2, typename T3, typename Func>
inline POET_FLATTEN void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2, T3>;
    using Callable = std::remove_reference_t<Func>;

    if constexpr (std::is_lvalue_reference_v<Func>) {
        // Avoid copying lvalue callables in hot paths.
        detail::dynamic_for_impl<T, std::remove_reference_t<Func>, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), static_cast<T>(step), func);
    } else {
        [[maybe_unused]] Callable callable(std::forward<Func>(func));
        detail::dynamic_for_impl<T, Callable, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), static_cast<T>(step), callable);
    }
}

/// \brief Executes a runtime-sized loop using compile-time unrolling with
/// automatic step detection.
///
/// If `begin <= end`, step is +1.
/// If `begin > end`, step is -1.
template<std::size_t Unroll = kDefaultUnroll, typename T1, typename T2, typename Func>
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
template<std::size_t Unroll = kDefaultUnroll, typename Func> inline void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll>(static_cast<std::size_t>(0), count, std::forward<Func>(func));
}

} // namespace poet


#if __cplusplus >= 202002L
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cstddef>

namespace poet {

// Adaptor holds the user callable.
// Template ordering: Func first (deduced), Unroll second (optional).
template<typename Func, std::size_t Unroll = poet::kDefaultUnroll>
struct dynamic_for_adaptor {
  Func func;
  dynamic_for_adaptor(Func f) : func(std::move(f)) {}
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
template<std::size_t U = poet::kDefaultUnroll, typename F>
dynamic_for_adaptor<std::decay_t<F>, U> make_dynamic_for(F &&f) {
  return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

} // namespace poet
#endif // __cplusplus >= 202002L

#endif// POET_CORE_DYNAMIC_FOR_HPP
