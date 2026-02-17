#ifndef POET_CORE_STATIC_FOR_HPP
#define POET_CORE_STATIC_FOR_HPP

/// \file static_for.hpp
/// \brief Provides a compile-time loop unrolling utility.
///
/// This header defines `static_for`, which unrolls loops at compile-time.
/// It is useful for iterating over template parameter packs, integer sequences,
/// or performing other meta-programming tasks where the iteration count is known
/// at compile-time.

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <poet/core/for_utils.hpp>

namespace poet {

namespace detail {

    template<typename Callable,
      std::intmax_t Begin,
      std::intmax_t Step,
      std::size_t BlockSize,
      std::size_t FullBlocks,
      std::size_t Remainder>
    POET_FORCEINLINE constexpr void static_loop_run_blocks(Callable &callable) {
        if constexpr (FullBlocks > 0) {
            emit_all_blocks<Callable, Begin, Step, BlockSize, 0, FullBlocks>(callable);
        }

        if constexpr (Remainder > 0) {
            static_loop_impl_block<Callable, Begin, Step, FullBlocks * BlockSize>(
              callable, std::make_index_sequence<Remainder>{});
        }
    }

    /// \brief Computes a safe default block size for loop unrolling.
    ///
    /// - For small loops, returns the total iteration count.
    /// - For large loops, clamps the size to `kMaxStaticLoopBlock` to prevent
    ///   massive symbol names and excessive compile times.
    ///
    /// \tparam Begin Range start.
    /// \tparam End Range end.
    /// \tparam Step Range step.
    /// \return Optimized block size.
    template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
    POET_CPP20_CONSTEVAL auto compute_default_static_loop_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) {
            return 1;
        } else if constexpr (count > detail::kMaxStaticLoopBlock) {
            return detail::kMaxStaticLoopBlock;
        } else {
            return count;
        }
    }

    /// \brief Core loop driver.
    ///
    /// Splits the range `[0, count)` into `full_blocks` chunks of size `BlockSize`,
    /// followed by a single `remainder` block. This partitioning strategy reduces
    /// template instantiation depth.
    ///
    /// \tparam Begin Range start.
    /// \tparam End Range end.
    /// \tparam Step Range step.
    /// \tparam BlockSize Number of iterations per block.
    /// \tparam Func Callable type.
    /// \param func Callable to invoke.

    POET_PUSH_OPTIMIZE
    template<std::intmax_t Begin,
      std::intmax_t End,
      std::intmax_t Step = 1,
      std::size_t BlockSize = compute_default_static_loop_block_size<Begin, End, Step>(),
      typename Func>
    POET_FORCEINLINE POET_FLATTEN constexpr void static_loop(Func &&func) {
        static_assert(BlockSize > 0, "static_loop requires BlockSize > 0");

        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) { return; }

        constexpr auto full_blocks = count / BlockSize;
        constexpr auto remainder = count % BlockSize;

        with_stored_callable(std::forward<Func>(func), [&](auto &callable) -> void {
            using callable_t = std::remove_reference_t<decltype(callable)>;
            static_loop_run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder>(callable);
        });
    }
    POET_POP_OPTIMIZE

}// namespace detail

/// \brief Adapts a callable to `static_loop` over a compile-time integer range.
///
/// This helper provides the unified `poet::static_for` API. It executes a
/// compile-time unrolled loop over the half-open range `[Begin, End)` using the
/// specified `Step`, where `Step` can be positive or negative. The iteration
/// space is partitioned into blocks of `BlockSize` elements, and each block is
/// emitted through `static_loop`.
///
/// Callables are supported in two forms:
/// - A callable that accepts a `std::integral_constant<std::intmax_t, I>`
///   argument. This form is constexpr-friendly and lets the implementation
///   forward the index as a compile-time value.
/// - A functor exposing `template <auto I>` call operators. In this case,
///   `static_for` internally adapts the functor to the integral-constant based
///   machinery.
///
/// The default `BlockSize` spans the entire range, clamped to the internal
/// `detail::kMaxStaticLoopBlock` cap (currently `256`), to balance unrolling
/// with compile-time cost. Lvalue callables are preserved by reference, while
/// rvalues are copied into a local instance for the duration of the loop.
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
///                   total iteration count, clamped to `1` for empty ranges and
///                   to `detail::kMaxStaticLoopBlock` for large ranges).
/// \tparam Func Callable type.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t Begin,
    std::intmax_t End,
    std::intmax_t Step = 1,
    std::size_t BlockSize = detail::compute_default_static_loop_block_size<Begin, End, Step>(),
    typename Func>
POET_FORCEINLINE POET_FLATTEN constexpr void static_for(Func &&func) {
    if constexpr (std::is_invocable_v<Func, std::integral_constant<std::intmax_t, Begin>>) {
        detail::static_loop<Begin, End, Step, BlockSize>(std::forward<Func>(func));
    } else {
        detail::with_stored_callable(std::forward<Func>(func), [&](auto &callable) -> void {
            using callable_t = std::remove_reference_t<decltype(callable)>;
            const detail::template_static_loop_invoker<callable_t> invoker{ &callable };
            detail::static_loop<Begin, End, Step, BlockSize>(invoker);
        });
    }
}

/// \brief Convenience overload for `static_for` iterating from 0 to `End`.
///
/// This is equivalent to `static_for<0, End>(func)`.
///
/// \tparam End Exclusive upper bound of the range.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t End, typename Func> POET_FORCEINLINE POET_FLATTEN constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}

}// namespace poet

#endif// POET_CORE_STATIC_FOR_HPP
