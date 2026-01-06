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
#include <type_traits>
#include <utility>

#include <poet/core/static_dispatch.hpp>
#include <poet/core/static_for.hpp>

namespace poet {

namespace detail {

    /// \brief Helper functor that adapts a user lambda for use with static_for.
    ///
    /// This struct wraps a runtime function pointer/reference along with base and
    /// step values. It exposes a compile-time call operator that calculates the
    /// actual runtime index based on the compile-time offset `I` and invokes the
    /// user function.
    ///
    /// \tparam Func Type of the user-provided callable.
    /// \tparam T Type of the loop counter (e.g., int, size_t).
    template<typename Func, typename T> struct dynamic_block_invoker {
        Func *func;
        T base;
        T step;

        template<auto I> constexpr void operator()() const {
            // Invoke the user lambda with the current runtime index.
            // The runtime index is computed as: base + (compile_time_offset * step).
            (*func)(base + (static_cast<T>(I) * step));
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
    inline void execute_runtime_block([[maybe_unused]] Func &func, [[maybe_unused]] T base, [[maybe_unused]] T step) {
        if constexpr (BlockSize > 0) {
            // Create an invoker that captures the user function and the current base index.
            dynamic_block_invoker<Func, T> invoker{ &func, base, step };

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
        T step;

        template<int Tail> void operator()(T base) const {
            // This function is instantiated by the dispatcher for a specific 'Tail' value.
            // We call execute_runtime_block with 'Tail' as the compile-time block size.
            execute_runtime_block<Callable, T, static_cast<std::size_t>(Tail)>(*callable, base, step);
        }
    };

    /// \brief Internal implementation of the dynamic loop.
    ///
    /// Calculates the total number of iterations required based on proper signed
    /// arithmetic and executes the loop in chunks of `Unroll`. Any remaining
    /// iterations are handled by `poet::dispatch`.
    template<typename T, typename Callable, std::size_t Unroll>
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    inline void dynamic_for_impl(T start_val, T end_val, T step, Callable &callable) {
        if (step == 0) { return; }

        // Calculate iteration count.
        // We determine the number of steps to go from `start_val` to `end_val` exclusively.
        std::size_t count = 0;

        if (step > 0) {
            // Forward iteration
            if (start_val >= end_val) {
                count = 0;
            } else {
                // Logic for positive step:
                // dist = end - start
                // count = ceil(dist / step) = (dist + step - 1) / step
                auto dist = static_cast<std::size_t>(end_val - start_val);
                auto ustep = static_cast<std::size_t>(step);
                count = (dist + ustep - 1) / ustep;
            }
        } else {
            // Backward iteration (step < 0)
            if constexpr (std::is_signed_v<T>) {
                if (start_val <= end_val) {
                    count = 0;
                } else {
                    // Logic for negative step:
                    // dist = start - end
                    // count = ceil(dist / abs(step))
                    auto dist = static_cast<std::size_t>(start_val - end_val);
                    auto ustep = static_cast<std::size_t>(-step);// Safe absolute value
                    count = (dist + ustep - 1) / ustep;
                }
            }
        }

        T index = start_val;
        std::size_t remaining = count;

        // Execute full blocks of size 'Unroll'.
        // We use a runtime while loop here, but the body (execute_runtime_block)
        // is fully unrolled at compile-time for 'Unroll' iterations.
        while (remaining >= Unroll) {
            detail::execute_runtime_block<Callable, T, Unroll>(callable, index, step);
            index += static_cast<T>(Unroll) * step;
            remaining -= Unroll;
        }

        // Handle remaining iterations (tail).
        if constexpr (Unroll > 1) {
            if (remaining > 0) {
                // Dispatch the runtime 'remaining' count to a compile-time template instantiation.
                // This ensures even the tail is unrolled, avoiding a runtime loop for the last few elements.
                const detail::tail_caller_for_dynamic_for<Callable, T> tail_caller{ &callable, step };
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

}// namespace poet

#endif// POET_CORE_DYNAMIC_FOR_HPP
