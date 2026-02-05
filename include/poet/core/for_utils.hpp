#ifndef POET_CORE_FOR_UTILS_HPP
#define POET_CORE_FOR_UTILS_HPP

/// \file for_utils.hpp
/// \brief Internal utilities for compile-time loop unrolling and range computation.
///
/// This header provides core building blocks for `static_for` and `dynamic_for`.
/// It includes mechanisms for:
/// - Calculating iteration counts for compile-time ranges.
/// - Recursively emitting blocks of unrolled code to avoid compiler limits.
/// - Defining limits on unrolling depth (recursion depth).

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>

namespace poet {

namespace detail {

/// \brief Maximum chunk size for static loops to limit template instantiation depth.
/// Defines the maximum number of iterations or blocks processed in a single
/// recursive step. Reduced on MSVC to mitigate potential compiler stack overflows.
#if defined(_MSC_VER) && !defined(__clang__)
    inline constexpr std::size_t kMaxStaticLoopBlock = 128;
#else
    // Use a conservative upper bound for non-MSVC compilers to limit template
    // instantiation depth and keep compile times reasonable for tests.
    //
    // Capped at 256 to avoid excessive unrolling and long compile times.
    inline constexpr std::size_t kMaxStaticLoopBlock = 256;
#endif

    /// \brief Computes the number of iterations for a compile-time range.
    ///
    /// \tparam Begin Start of the range.
    /// \tparam End End of the range (exclusive).
    /// \tparam Step Iteration step (must be non-zero).
    /// \return The number of steps required to traverse from Begin to End.
    template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
    [[nodiscard]] constexpr auto compute_range_count() noexcept -> std::size_t {
        static_assert(Step != 0, "static_for requires a non-zero step");

        if constexpr (Step > 0) {
            static_assert(Begin <= End, "static_for with a positive step requires Begin <= End");
            if constexpr (Begin == End) { return 0; }
            const auto distance = End - Begin;
            return static_cast<std::size_t>((distance + Step - 1) / Step);
        } else {
            static_assert(Begin >= End, "static_for with a negative step requires Begin >= End");
            if constexpr (Begin == End) { return 0; }
            const auto distance = Begin - End;
            const auto magnitude = -Step;
            return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
        }
        // Unreachable: all cases covered by if constexpr branches above
        POET_UNREACHABLE();
    }

    /// \brief Executes a single block of unrolled loop iterations.
    ///
    /// Expands the provided indices pack into a sequence of function calls.
    /// Each call receives a `std::integral_constant` corresponding to the
    /// computed loop index.
    ///
    /// \tparam Func User callable type.
    /// \tparam Begin Range start value.
    /// \tparam Step Range step value.
    /// \tparam StartIndex The flat index offset for this block.
    /// \tparam Is Index sequence for unrolling (0, 1, ..., BlockSize-1).
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t StartIndex, std::size_t... Is>
    POET_FORCEINLINE constexpr void static_loop_impl_block(Func &func, std::index_sequence<Is...> /*indices*/) {
        // Fold expression over the index sequence Is...
        // For each compile-time index 'i' in Is:
        // 1. Compute the absolute iteration index: `StartIndex + i`
        // 2. Map to the actual value range: `Begin + (Step * absolute_index)`
        // 3. Construct an `std::integral_constant` for that value.
        // 4. Invoke `func` with that constant.
        // 5. The comma operator ... ensures sequential execution.
        // Optimization: Precompute Base = Begin + Step*StartIndex to reduce arithmetic per iteration.
        constexpr std::intmax_t Base = Begin + (Step * static_cast<std::intmax_t>(StartIndex));
        (func(std::integral_constant<std::intmax_t, Base + (Step * static_cast<std::intmax_t>(Is))>{}), ...);
    }

    /// \brief Processes a chunk of loop blocks.
    ///
    /// Recursively invokes `static_loop_impl_block` for a subset of the total blocks.
    /// Used to decompose very large loops into smaller compilation units.
    template<typename Func,
      std::intmax_t Begin,
      std::intmax_t Step,
      std::size_t BlockSize,
      std::size_t Offset,
      typename Tuple,
      std::size_t... Is>
    POET_FORCEINLINE constexpr void emit_block_chunk(Func &func, const Tuple & /*tuple*/, std::index_sequence<Is...> /*indices*/) {
        // This function processes a "chunk" of blocks to limit recursion depth.
        // It iterates over `Is...` (0 to ChunkSize-1).
        // For each `i` in `Is`:
        //   - `tuple` contains the block indices {0, 1, 2, ... FullBlocks-1}.
        //   - We access the block index at `Offset + i`.
        //   - We convert that block index into a global start index: `block_index * BlockSize`.
        //   - We call `static_loop_impl_block` to emit the iterations for that block.
        (static_loop_impl_block<Func, Begin, Step, std::tuple_element_t<Offset + Is, Tuple>::value * BlockSize>(
           func, std::make_index_sequence<BlockSize>{}),
          ...);
    }

    template<typename Func,
      std::intmax_t Begin,
      std::intmax_t Step,
      std::size_t BlockSize,
      typename Tuple,
      std::size_t Offset,
      std::size_t Remaining>
    POET_FORCEINLINE constexpr void emit_all_blocks_from_tuple(Func &func, const Tuple &tuple) {
        // Recursive function used to iterate over the tuple of block indices.
        // It consumes 'chunk_size' blocks at a time, where 'chunk_size' is capped
        // by kMaxStaticLoopBlock. This prevents generating a single massive fold
        // expression for loops with thousands of blocks (e.g., 10k iterations unrolled).
        if constexpr (Remaining > 0) {
            constexpr auto chunk_size = Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;

            // Emit the current chunk of blocks.
            emit_block_chunk<Func, Begin, Step, BlockSize, Offset>(func, tuple, std::make_index_sequence<chunk_size>{});

            // Recursively process the rest of the blocks.
            emit_all_blocks_from_tuple<Func,
              Begin,
              Step,
              BlockSize,
              Tuple,
              Offset + chunk_size,
              Remaining - chunk_size>(func, tuple);
        } else {
            // Base case: no blocks remaining.
        }
    }

    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, typename Tuple>
    POET_FORCEINLINE constexpr void emit_all_blocks(Func &func, const Tuple &tuple) {
        constexpr auto total_blocks = std::tuple_size_v<Tuple>;
        if constexpr (total_blocks > 0) {
            emit_all_blocks_from_tuple<Func, Begin, Step, BlockSize, Tuple, 0, total_blocks>(func, tuple);
        } else {
            // Zero blocks to emit.
        }
    }

    template<typename Functor> struct template_static_loop_invoker {
        Functor *functor;

        // Adapter operator:
        // Receives an std::integral_constant<int, Value> from implementation internals.
        // Unpacks 'Value' and calls the user's template operator<Value>().
        // We call a small out-of-line helper to avoid forcing the caller to
        // inline every `operator()<Value>` instantiation. This reduces the
        // size of the unrolled caller bodies and improves instruction-cache
        // behavior and register allocation for large unrolls. The helper is
        // marked `POET_NOINLINE` so it is emitted as a separate function.
        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*integral_constant*/) const {
            // Forward to a noinline helper; the template instantiation for
            // `invoke_template_operator` still exists per Value, but it is
            // emitted out-of-line, keeping the caller compact.
            invoke_template_operator<Functor, Value>(functor);
        }
    };

    // Out-of-line invocation helper.
    template<typename Functor, std::intmax_t Value>
    POET_NOINLINE void invoke_template_operator(Functor *functor) {
        (*functor).template operator()<Value>();
    }

}// namespace detail

// --- Public compatibility wrappers ---

inline constexpr std::size_t kMaxStaticLoopBlock = detail::kMaxStaticLoopBlock;

template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
[[nodiscard]] constexpr auto compute_range_count() noexcept -> std::size_t {
    return detail::compute_range_count<Begin, End, Step>();
}

}// namespace poet

#endif// POET_CORE_FOR_UTILS_HPP
