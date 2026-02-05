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
#include <tuple>
#include <type_traits>
#include <utility>

#include <poet/core/for_utils.hpp>

namespace poet {

namespace detail {

    /// \brief Helper: Emits a sequence of full blocks.
    ///
    /// This splits the loop body into manageable chunks (blocks) and delegates
    /// to `emit_all_blocks`. This partitioning prevents massive single fold
    /// expressions which can overwhelm the compiler.
    ///
    /// \tparam Func Callable type.
    /// \tparam Begin Range start.
    /// \tparam Step Range step.
    /// \tparam BlockSize Number of iterations per block.
    /// \tparam BlockIndices Indices for the blocks (0, 1, ...).
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, std::size_t... BlockIndices>
    POET_FORCEINLINE constexpr void static_loop_emit_all_blocks(Func &func, std::index_sequence<BlockIndices...> /*blocks*/) {
        // Wrap block indices into integral_constants in a tuple.
        // This tuple is then processed recursively by emit_all_blocks to avoid excessive
        // instantiation depth that a single fold expression over all blocks might cause.
        using blocks_tuple = std::tuple<std::integral_constant<std::size_t, BlockIndices>...>;
        emit_all_blocks<Func, Begin, Step, BlockSize>(func, blocks_tuple{});
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
    constexpr auto compute_default_static_loop_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) { return 1; }
        if constexpr (count > detail::kMaxStaticLoopBlock) { return detail::kMaxStaticLoopBlock; }
        return count;
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
        using Callable = std::remove_reference_t<Func>;
        // Create a local copy of the callable to ensure state persistence across block calls
        // if passed by rvalue, or bind a reference if passed by lvalue.
        // This is crucial for stateful functors.
        Callable callable(std::forward<Func>(func));

        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) { return; }

        // Calculate partitioning
        constexpr auto full_blocks = count / BlockSize;
        constexpr auto remainder = count % BlockSize;

        // 1. Process all full blocks
        // delegating to static_loop_emit_all_blocks which handles recursion limits
        if constexpr (full_blocks > 0) {
            static_loop_emit_all_blocks<Callable, Begin, Step, BlockSize>(
              callable, std::make_index_sequence<full_blocks>{});
        }

        // 2. Process remaining tail elements
        // The tail block is always smaller than BlockSize, so we can emit it directly
        // as a single block.
        if constexpr (remainder > 0) {
            static_loop_impl_block<Callable, Begin, Step, full_blocks * BlockSize>(
              callable, std::make_index_sequence<remainder>{});
        }
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
/// The default `BlockSize` spans the entire range, clamped to
/// `poet::kMaxStaticLoopBlock` (currently `256`), to balance unrolling with
/// compile-time cost. Lvalue callables are preserved by reference, while
/// rvalues are copied into a local instance for the duration of the loop.
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
///                   total iteration count, clamped to `1` for empty ranges and
///                   to `poet::kMaxStaticLoopBlock` for large ranges).
/// \tparam Func Callable type.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t Begin,
    std::intmax_t End,
    std::intmax_t Step = 1,
    std::size_t BlockSize = detail::compute_default_static_loop_block_size<Begin, End, Step>(),
    typename Func>
POET_FORCEINLINE POET_FLATTEN constexpr void static_for(Func &&func) {
    // Check if the user functor accepts an integral_constant index directly.
    if constexpr (std::is_invocable_v<Func, std::integral_constant<std::intmax_t, Begin>>) {
        // Direct invocation mode: simply forward to static_loop.
        detail::static_loop<Begin, End, Step, BlockSize>(std::forward<Func>(func));
    } else {
        // Template operator invocation mode (func.template operator()<I>()).
        // We wrap the functor in a helper (template_static_loop_invoker) that exposes
        // the integral_constant call operator expected by the backend.

        using Functor = std::remove_reference_t<Func>;

        if constexpr (std::is_lvalue_reference_v<Func>) {
            // If we got an lvalue reference, we must keep referring to the original object
            // to allow state mutation.
            const detail::template_static_loop_invoker<Functor> invoker{ &func };
            detail::static_loop<Begin, End, Step, BlockSize>(invoker);
        } else {
            // If we got an rvalue, we move-construct a local copy to keep it alive
            // during the loop execution.
            Functor functor(std::forward<Func>(func));
            const detail::template_static_loop_invoker<Functor> invoker{ &functor };
            detail::static_loop<Begin, End, Step, BlockSize>(invoker);
        }
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
