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
      std::size_t Remainder,
      bool Isolate>
    POET_FORCEINLINE constexpr void static_loop_run_blocks(Callable &callable) {
        if constexpr (FullBlocks > 0) {
            // When BlockSize is explicitly tuned, use register-isolated (noinline)
            // blocks for multi-block loops.  This prevents the compiler from
            // interleaving computations across blocks, which would cause excessive
            // register spills on x86-64.
            //
            // When BlockSize is the default (Isolate == false), always inline —
            // the user hasn't opted into register-pressure tuning, so prefer
            // maximum inlining to let the compiler optimise freely.
            if constexpr (Isolate && FullBlocks > 1) {
                emit_all_blocks_isolated<Callable, Begin, Step, BlockSize, 0, FullBlocks>(callable);
            } else {
                emit_all_blocks<Callable, Begin, Step, BlockSize, 0, FullBlocks>(callable);
            }
        }

        if constexpr (Remainder > 0) {
            // Remainder is always small (< BlockSize), so always inline it.
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

}// namespace detail

/// \brief Compile-time unrolled loop over the half-open range `[Begin, End)`.
///
/// Executes a compile-time unrolled loop using the specified `Step`, where
/// `Step` can be positive or negative. The iteration space is partitioned
/// into blocks of `BlockSize` elements to manage template instantiation depth.
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
/// with compile-time cost.  With the default block size, **all iterations are
/// fully inlined** so the compiler can optimise freely across the entire loop.
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
/// GCC vectorizes the loop body `lanes_64`-wide: each vector group packs
/// `lanes_64` iterations into one SIMD register, plus the FMA chain needs
/// ~`fma_depth + 1` constant/temporary registers as shared overhead.  The
/// spill-free limit is therefore:
///
///   `max_bs = (vec_regs − (fma_depth + 1)) × lanes_64`
///
/// Measured spill cliffs (GCC 15, -O3, 5-deep FMA chain):
///
///   | ISA     | vec_regs | lanes_64 | spill-free max | first spills |
///   |---------|----------|----------|----------------|--------------|
///   | SSE2    |    16    |    2     | >128 (scalar)  | N/A          |
///   | AVX2    |    16    |    4     |  40 accs       | 44 accs      |
///   | AVX-512 |    32    |    8     | >256 (batched) | N/A          |
///
/// In practice icache pressure caps the useful block size well below the
/// spill limit.  A good starting point:
///
///   `optimal_bs ≈ vec_regs × lanes_64 / 2`
///   SSE2 → 16,  AVX2 → 32,  AVX-512 → 128
///
/// Use the `poet_bench_static_for_native` benchmark to measure the effect
/// of different block sizes on your workload.
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
POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_assert(BlockSize > 0, "static_for requires BlockSize > 0");

    constexpr auto count = detail::compute_range_count<Begin, End, Step>();
    if constexpr (count == 0) { return; }

    constexpr auto full_blocks = count / BlockSize;
    constexpr auto remainder = count % BlockSize;

    // Register isolation is only applied when the user explicitly tuned
    // BlockSize.  The default path fully inlines everything to let the
    // compiler optimise without artificial register-scope boundaries.
    constexpr bool isolate = (BlockSize != detail::compute_default_static_loop_block_size<Begin, End, Step>());

    // Callable storage is handled inline (rather than via a helper) to
    // avoid introducing an indirection that compilers may refuse to
    // inline across the block emission pipeline.
    using callable_t = std::remove_reference_t<Func>;

    if constexpr (std::is_invocable_v<Func, std::integral_constant<std::intmax_t, Begin>>) {
        if constexpr (std::is_lvalue_reference_v<Func>) {
            detail::static_loop_run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(func);
        } else {
            callable_t callable(std::forward<Func>(func));
            detail::static_loop_run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(
              callable);
        }
    } else {
        using invoker_t = detail::template_static_loop_invoker<callable_t>;
        if constexpr (std::is_lvalue_reference_v<Func>) {
            const invoker_t invoker{ &func };
            detail::static_loop_run_blocks<const invoker_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(
              invoker);
        } else {
            callable_t callable(std::forward<Func>(func));
            const invoker_t invoker{ &callable };
            detail::static_loop_run_blocks<const invoker_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(
              invoker);
        }
    }
}

/// \brief Convenience overload for `static_for` iterating from 0 to `End`.
///
/// This is equivalent to `static_for<0, End>(func)`.
///
/// \tparam End Exclusive upper bound of the range.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}

}// namespace poet

#endif// POET_CORE_STATIC_FOR_HPP
