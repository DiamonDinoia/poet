#ifndef POET_CORE_DYNAMIC_FOR_HPP
#define POET_CORE_DYNAMIC_FOR_HPP

/// \file dynamic_for.hpp
/// \brief Provides runtime loop execution with compile-time unrolling.
///
/// This header defines `dynamic_for`, a utility that allows executing a loop
/// where the range is determined at runtime, but the body is unrolled at
/// compile-time. This is achieved by combining `static_for` for the bulk of
/// iterations and a runtime loop (or dispatch) strategy.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <poet/core/static_dispatch.hpp>
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
        Func *func;
        T base;
        T stride;

        template<auto I> constexpr void operator()() const {
            // Invoke the user lambda with the current runtime index.
            // The runtime index is computed as: base + (compile_time_offset * stride).
            (*func)(base + (static_cast<T>(I) * stride));
        }
    };

    /// \brief Emits a block of unrolled iterations.
    ///
    /// Invokes `static_for` to generate `BlockSize` calls to the user function.
    /// This helper is used for both the main unrolled loop body and the
    /// tail handling (where the block size is determined at runtime via dispatch).
    ///
    /// \tparam Func User callable type.
    /// \tparam T Loop counter type.
    /// \tparam BlockSize Number of iterations to unroll in this block.
    template<typename Func, typename T, std::size_t BlockSize>
    inline void execute_runtime_block([[maybe_unused]] Func &func, [[maybe_unused]] T base, [[maybe_unused]] T stride) {
        if constexpr (BlockSize > 0) {
            // Create an invoker that captures the user function and the current base index.
            dynamic_block_invoker<Func, T> invoker{ &func, base, stride };

            // Use static_for to generate exactly 'BlockSize' compile-time calls.
            // We pass BlockSize as the unrolling factor to ensure a single unrolled block is emitted.
            // The range [0, BlockSize) will be iterated.
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        } else {
            // Nothing to do when the block size is zero.
        }
    }

    /// \brief Functor for dispatching the tail of a dynamic loop.
    ///
    /// When the stored `Tail` template parameter matches the runtime remainder,
    /// this operator invokes `execute_runtime_block` with the correct
    /// compile-time block size.
    template<typename Callable, typename T> struct tail_caller_for_dynamic_for {
        Callable *callable;
        T stride;

        template<int Tail> void operator()(T base) const {
            // This function is instantiated by the dispatcher for a specific 'Tail' value.
            // We call execute_runtime_block with 'Tail' as the compile-time block size.
            execute_runtime_block<Callable, T, static_cast<std::size_t>(Tail)>(*callable, base, stride);
        }
    };

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
    template<typename T, typename Callable, std::size_t Unroll>
    inline void dynamic_for_impl(T begin, T end, T stride, Callable &callable) {
        if (stride == 0) { return; }

        // Calculate iteration count.
        // We determine the number of steps to go from `begin` to `end` exclusively.
        std::size_t count = 0;

        // For unsigned types, detect wrapped negative values caused by implicit conversion.
        // When users pass negative literals (e.g., -1, -5) to unsigned parameters, C++ performs
        // implicit conversion via unsigned wrapping, resulting in very large positive values:
        //   - For uint32_t: -1 → 4294967295 (UINT_MAX), -2 → 4294967294, etc.
        //   - For uint64_t: -1 → 18446744073709551615 (ULLONG_MAX), -2 → 18446744073709551614, etc.
        //
        // Detection heuristic: Values > (max/2) are likely wrapped negatives.
        // This works because:
        //   1. Negative values -1 to -N map to [max, max-N+1] (all > max/2)
        //   2. Normal positive strides are typically small (< max/2)
        //   3. Edge case: Very large positive strides (> max/2) would be misidentified,
        //      but such strides are impractical (would cause integer overflow in loops)
        //
        // Examples for uint32_t (max = 4294967295, half_max = 2147483647):
        //   - stride = static_cast<uint32_t>(-1)  → 4294967295 > 2147483647 → detected as wrapped negative ✓
        //   - stride = static_cast<uint32_t>(-5)  → 4294967291 > 2147483647 → detected as wrapped negative ✓
        //   - stride = 100                        → 100 < 2147483647          → normal positive stride ✓
        //   - stride = 1000000                    → 1000000 < 2147483647      → normal positive stride ✓
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (stride < 0 || is_wrapped_negative) {
            // Backward iteration (negative stride or wrapped unsigned)
            if (begin <= end) {
                count = 0;
            } else {
                // Compute absolute stride value
                T abs_stride;
                if constexpr (std::is_signed_v<T>) {
                    abs_stride = static_cast<T>(-stride);  // Safe for signed types
                } else {
                    // For unsigned wrapped values: unsigned(-1) → abs is 1
                    abs_stride = static_cast<T>(0) - stride;  // Wrapping subtraction
                }
                auto dist = static_cast<std::size_t>(begin - end);
                auto ustride = static_cast<std::size_t>(abs_stride);
                count = (dist + ustride - 1) / ustride;
            }
        } else {
            // Forward iteration (stride > 0 and not wrapped)
            if (begin >= end) {
                count = 0;
            } else {
                // Logic for positive stride:
                // dist = end - begin
                // count = ceil(dist / stride) = (dist + stride - 1) / stride
                auto dist = static_cast<std::size_t>(end - begin);
                auto ustride = static_cast<std::size_t>(stride);
                count = (dist + ustride - 1) / ustride;
            }
        }

        T index = begin;
        std::size_t remaining = count;

        // Execute full blocks of size 'Unroll'.
        // We use a runtime while loop here, but the body (execute_runtime_block)
        // is fully unrolled at compile-time for 'Unroll' iterations.
        while (remaining >= Unroll) {
            detail::execute_runtime_block<Callable, T, Unroll>(callable, index, stride);
            index += static_cast<T>(Unroll) * stride;
            remaining -= Unroll;
        }

        // Handle remaining iterations (tail).
        if constexpr (Unroll > 1) {
            if (remaining > 0) {
                // Dispatch the runtime 'remaining' count to a compile-time template instantiation.
                // This ensures even the tail is unrolled, avoiding a runtime loop for the last few elements.
                const detail::tail_caller_for_dynamic_for<Callable, T> tail_caller{ &callable, stride };
                // Define the allowed range of tail sizes: [0, Unroll - 1].
                using TailRange = poet::make_range<0, (static_cast<int>(Unroll) - 1)>;
                auto params = std::make_tuple(poet::DispatchParam<TailRange>{ static_cast<int>(remaining) });
                // Invoke dispatch. This will find the matching CompileTimeTail in TailRange
                // and call tail_caller.operator()<CompileTimeTail>(index).
                poet::dispatch(tail_caller, params, index);
            }
        }
    }

}// namespace detail

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// The helper iterates over the half-open range `[begin, end)` with a given
/// `step`. Blocks of `Unroll` iterations are dispatched through `static_for`.
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \param begin Inclusive lower/start bound.
/// \param end Exclusive upper/end bound.
/// \param step Increment per iteration. Can be negative.
/// \param func Callable invoked for each iteration.
constexpr std::size_t kDefaultUnroll = 8;

template<std::size_t Unroll = kDefaultUnroll, typename T1, typename T2, typename T3, typename Func>
inline void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2, T3>;
    using Callable = std::remove_reference_t<Func>;

    [[maybe_unused]] Callable callable(std::forward<Func>(func));

    // Delegate to implementation
    detail::dynamic_for_impl<T, Callable, Unroll>(
      static_cast<T>(begin), static_cast<T>(end), static_cast<T>(step), callable);
}

/// \brief Executes a runtime-sized loop using compile-time unrolling with
/// automatic step detection.
///
/// If `begin <= end`, step is +1.
/// If `begin > end`, step is -1.
template<std::size_t Unroll = kDefaultUnroll, typename T1, typename T2, typename Func>
inline void dynamic_for(T1 begin, T2 end, Func &&func) {
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
