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
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>

namespace poet::detail {

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
    [[nodiscard]] POET_CPP20_CONSTEVAL auto compute_range_count() noexcept -> std::size_t {
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
    }

    /// \brief Executes a single block of unrolled loop iterations (always-inline variant).
    ///
    /// Expands the provided indices pack into a sequence of function calls.
    /// Each call receives a `std::integral_constant` corresponding to the
    /// computed loop index.  Used for single-block loops and remainder
    /// iterations where the block is small enough that full inlining into the
    /// caller is desirable.
    ///
    /// \tparam Func User callable type.
    /// \tparam Begin Range start value.
    /// \tparam Step Range step value.
    /// \tparam StartIndex The flat index offset for this block.
    /// \tparam Is Index sequence for unrolling (0, 1, ..., BlockSize-1).
    POET_PUSH_OPTIMIZE
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t StartIndex, std::size_t... Is>
    POET_FORCEINLINE constexpr void static_loop_impl_block(Func &func, std::index_sequence<Is...> /*indices*/) {
        constexpr std::intmax_t Base = Begin + (Step * static_cast<std::intmax_t>(StartIndex));
        (func(std::integral_constant<std::intmax_t, Base + (Step * static_cast<std::intmax_t>(Is))>{}), ...);
    }
    POET_POP_OPTIMIZE

    /// \brief Executes a single block with register-pressure isolation (noinline variant).
    ///
    /// Used for multi-block loops where POET_FORCEINLINE would cause the
    /// compiler to see all iterations simultaneously, resulting in excessive
    /// register spills.  Each noinline block gets its own register allocation
    /// scope — the compiler fully optimises within the block but cannot
    /// interleave computations across block boundaries.
    POET_PUSH_OPTIMIZE
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t StartIndex, std::size_t... Is>
    POET_NOINLINE constexpr void static_loop_impl_block_isolated(Func &func, std::index_sequence<Is...> /*indices*/) {
        constexpr std::intmax_t Base = Begin + (Step * static_cast<std::intmax_t>(StartIndex));
        (func(std::integral_constant<std::intmax_t, Base + (Step * static_cast<std::intmax_t>(Is))>{}), ...);
    }
    POET_POP_OPTIMIZE

    /// \brief Emits a chunk of full blocks (always-inline variant).
    POET_PUSH_OPTIMIZE
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, std::size_t Offset, std::size_t... Is>
    POET_FORCEINLINE constexpr void emit_block_chunk(Func &func, std::index_sequence<Is...> /*indices*/) {
        (static_loop_impl_block<Func, Begin, Step, (Offset + Is) * BlockSize>(func, std::make_index_sequence<BlockSize>{}),
          ...);
    }
    POET_POP_OPTIMIZE

    /// \brief Emits a chunk of register-isolated blocks (noinline variant).
    POET_PUSH_OPTIMIZE
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, std::size_t Offset, std::size_t... Is>
    POET_FORCEINLINE constexpr void emit_block_chunk_isolated(Func &func, std::index_sequence<Is...> /*indices*/) {
        (static_loop_impl_block_isolated<Func, Begin, Step, (Offset + Is) * BlockSize>(func, std::make_index_sequence<BlockSize>{}),
          ...);
    }
    POET_POP_OPTIMIZE

    POET_PUSH_OPTIMIZE
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, std::size_t Offset, std::size_t Remaining>
    POET_FORCEINLINE constexpr void emit_all_blocks(Func &func) {
        if constexpr (Remaining > 0) {
            constexpr auto chunk_size = Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;

            emit_block_chunk<Func, Begin, Step, BlockSize, Offset>(func, std::make_index_sequence<chunk_size>{});

            emit_all_blocks<Func, Begin, Step, BlockSize, Offset + chunk_size, Remaining - chunk_size>(func);
        }
    }
    POET_POP_OPTIMIZE

    POET_PUSH_OPTIMIZE
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, std::size_t Offset, std::size_t Remaining>
    POET_FORCEINLINE constexpr void emit_all_blocks_isolated(Func &func) {
        if constexpr (Remaining > 0) {
            constexpr auto chunk_size = Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;

            emit_block_chunk_isolated<Func, Begin, Step, BlockSize, Offset>(func, std::make_index_sequence<chunk_size>{});

            emit_all_blocks_isolated<Func, Begin, Step, BlockSize, Offset + chunk_size, Remaining - chunk_size>(func);
        }
    }
    POET_POP_OPTIMIZE

    template<typename Functor> struct template_static_loop_invoker {
        Functor *functor;

        // Adapter operator: receives std::integral_constant<Value> and unpacks it
        // to call the user's template operator<Value>().
        // Force inline to eliminate adapter overhead and enable the compiler to
        // fully optimize the per-iteration body in unrolled loops.
        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*integral_constant*/) const {
            POET_ASSUME_NOT_NULL(functor);
            (*functor).template operator()<Value>();
        }
    };

}// namespace poet::detail

#endif// POET_CORE_FOR_UTILS_HPP
