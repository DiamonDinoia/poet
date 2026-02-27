#pragma once

/// \file static_for.hpp
/// \brief Provides a compile-time loop unrolling utility.
///
/// This header defines `static_for`, which unrolls loops at compile-time.
/// It is useful for iterating over template parameter packs, integer sequences,
/// or performing other meta-programming tasks where the iteration count is known
/// at compile-time.

#include <cstddef>
#include <type_traits>
#include <utility>

#include <poet/core/for_utils.hpp>

namespace poet {

namespace detail {

    template<typename Callable,
      std::ptrdiff_t Begin,
      std::ptrdiff_t Step,
      std::size_t BlockSize,
      std::size_t FullBlocks,
      std::size_t Remainder>
    POET_FORCEINLINE constexpr void run_blocks(Callable &callable) {
        if constexpr (FullBlocks > 0) {
            // Use register-isolated (noinline) blocks for multi-block loops to
            // prevent the compiler from interleaving computations across blocks,
            // which would cause excessive register spills on x86-64.
            // Single-block loops are always inlined.
            if constexpr (FullBlocks > 1) {
                emit_blocks_iso<Callable, Begin, Step, BlockSize>(callable, std::make_index_sequence<FullBlocks>{});
            } else {
                emit_blocks<Callable, Begin, Step, BlockSize>(callable, std::make_index_sequence<FullBlocks>{});
            }
        }

        if constexpr (Remainder > 0) {
            run_block<Callable, Begin, Step, FullBlocks * BlockSize>(callable, std::make_index_sequence<Remainder>{});
        }
    }

    /// \brief Default block size: total iteration count, or 1 for empty ranges.
    template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::ptrdiff_t Step>
    POET_CPP20_CONSTEVAL auto default_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        return count == 0 ? 1 : count;
    }

}// namespace detail

/// \brief Compile-time unrolled loop over the half-open range `[Begin, End)`.
///
/// Executes a compile-time unrolled loop using the specified `Step`, where
/// `Step` can be positive or negative. The iteration space is partitioned
/// into blocks of `BlockSize` elements to manage template instantiation depth.
///
/// Callables are supported in two forms:
/// - A callable that accepts a `std::integral_constant<std::ptrdiff_t, I>`
///   argument. This form is constexpr-friendly and lets the implementation
///   forward the index as a compile-time value.
/// - A functor exposing `template <auto I>` call operators. In this case,
///   `static_for` internally adapts the functor to the integral-constant based
///   machinery./cle
///
/// The default `BlockSize` spans the entire range, so with the default block
/// size **all iterations are fully inlined** as a single block and the compiler
/// can optimise freely across the entire loop.
/// Lvalue callables are preserved by reference, while rvalues are copied into
/// a local instance for the duration of the loop.
///
/// ## Tuning `BlockSize` for register pressure
///
/// By default every iteration is fully inlined into the caller.  When the
/// per-iteration body is heavy (hashing, FP chains, etc.) this can cause
/// the compiler to interleave many iterations and spill registers.  Passing
/// an explicit `BlockSize` partitions the loop into noinline blocks, each
/// with its own register-allocation scope:
///
/// \code
///   // Default: all 64 iterations fully inlined (compiler optimises freely)
///   poet::static_for<0, 64>(func);
///
///   // Tuned: 8 noinline blocks of 8 (reduces register spills)
///   poet::static_for<0, 64, 1, 8>(func);
/// \endcode
///
///
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
///                   total iteration count, or `1` for empty ranges).
/// \tparam Func Callable type.
/// \param func Callable instance invoked once per iteration.
template<std::ptrdiff_t Begin,
  std::ptrdiff_t End,
  std::ptrdiff_t Step = 1,
  std::size_t BlockSize = detail::default_block_size<Begin, End, Step>(),
  typename Func>
POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_assert(BlockSize > 0, "static_for requires BlockSize > 0");

    constexpr auto count = detail::compute_range_count<Begin, End, Step>();
    if constexpr (count == 0) { return; }

    constexpr auto full_blocks = count / BlockSize;
    constexpr auto remainder = count % BlockSize;

    // Callable storage is handled inline (rather than via a helper) to
    // avoid introducing an indirection that compilers may refuse to
    // inline across the block emission pipeline.
    using callable_t = std::remove_reference_t<Func>;

    if constexpr (std::is_invocable_v<callable_t &, std::integral_constant<std::ptrdiff_t, Begin>>) {
        if constexpr (std::is_lvalue_reference_v<Func>) {
            detail::run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder>(func);
        } else {
            callable_t callable(std::forward<Func>(func));
            detail::run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder>(callable);
        }
    } else {
        using invoker_t = detail::template_invoker<callable_t>;
        if constexpr (std::is_lvalue_reference_v<Func>) {
            invoker_t invoker{ func };
            detail::run_blocks<invoker_t, Begin, Step, BlockSize, full_blocks, remainder>(invoker);
        } else {
            callable_t callable(std::forward<Func>(func));
            invoker_t invoker{ callable };
            detail::run_blocks<invoker_t, Begin, Step, BlockSize, full_blocks, remainder>(invoker);
        }
    }
}

/// \brief Convenience overload for `static_for` iterating from 0 to `End`.
///
/// This is equivalent to `static_for<0, End>(func)`.
///
/// \tparam End Exclusive upper bound of the range.
/// \param func Callable instance invoked once per iteration.
template<std::ptrdiff_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}

}// namespace poet
