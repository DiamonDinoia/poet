#pragma once

/// \file static_for.hpp
/// \brief Compile-time loop unrolling over integer ranges.

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

    template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::ptrdiff_t Step>
    POET_CPP20_CONSTEVAL auto default_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        return count == 0 ? 1 : count;
    }

}// namespace detail

/// \brief Runs a compile-time unrolled loop over `[Begin, End)`.
///
/// `func` may take `std::integral_constant<std::ptrdiff_t, I>` or expose
/// `template <auto I> operator()()`. `BlockSize` defaults to the full range;
/// pass a smaller value to isolate heavier bodies into separate outlined blocks.
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

    using callable_t = std::remove_reference_t<Func>;

    auto do_for = [&](auto &ref) POET_ALWAYS_INLINE_LAMBDA -> void {
        if constexpr (std::is_invocable_v<callable_t &, std::integral_constant<std::ptrdiff_t, Begin>>) {
            detail::run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder>(ref);
        } else {
            using invoker_t = detail::template_invoker<callable_t>;
            invoker_t invoker{ ref };
            detail::run_blocks<invoker_t, Begin, Step, BlockSize, full_blocks, remainder>(invoker);
        }
    };

    if constexpr (std::is_lvalue_reference_v<Func>) {
        do_for(func);
    } else {
        callable_t callable(std::forward<Func>(func));
        do_for(callable);
    }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/// \brief Convenience overload for `static_for<0, End>(func)`.
template<std::ptrdiff_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}
#endif

}// namespace poet
