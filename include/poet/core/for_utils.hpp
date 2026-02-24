#pragma once

/// \file for_utils.hpp
/// \brief Internal utilities for compile-time loop unrolling and range computation.

#include <cstddef>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>

namespace poet::detail {

/// \brief Computes the number of iterations for a compile-time range.
template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::ptrdiff_t Step>
[[nodiscard]] POET_CPP20_CONSTEVAL auto compute_range_count() noexcept -> std::size_t {
    static_assert(Step != 0, "static_for requires a non-zero step");
    if constexpr (Step > 0) {
        static_assert(Begin <= End, "static_for with a positive step requires Begin <= End");
    } else {
        static_assert(Begin >= End, "static_for with a negative step requires Begin >= End");
    }
    if constexpr (Begin == End) { return 0; }
    constexpr auto abs = [](auto x) -> decltype(x) { return x < 0 ? -x : x; };
    constexpr auto distance = abs(End - Begin);
    constexpr auto magnitude = abs(Step);
    return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
}

/// \brief Executes one unrolled block of iterations (always-inline).
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_FORCEINLINE constexpr auto run_block(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr std::ptrdiff_t Base = Begin + (Step * static_cast<std::ptrdiff_t>(StartIndex));
    (func(std::integral_constant<std::ptrdiff_t, Base + (Step * static_cast<std::ptrdiff_t>(Is))>{}), ...);
}

/// \brief Executes one unrolled block with register-pressure isolation (noinline).
///
/// Each call gets its own register-allocation scope, preventing the compiler
/// from interleaving computations across block boundaries.
POET_PUSH_OPTIMIZE

template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_NOINLINE_FLATTEN constexpr auto run_block_iso(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr std::ptrdiff_t Base = Begin + (Step * static_cast<std::ptrdiff_t>(StartIndex));
    (func(std::integral_constant<std::ptrdiff_t, Base + (Step * static_cast<std::ptrdiff_t>(Is))>{}), ...);
}

/// \brief Emits N full blocks, inline.
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t BlockSize, std::size_t... Is>
POET_FORCEINLINE constexpr auto emit_blocks(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    (run_block<Func, Begin, Step, Is * BlockSize>(func, std::make_index_sequence<BlockSize>{}), ...);
}

/// \brief Emits N full blocks, each noinline-isolated.
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t BlockSize, std::size_t... Is>
POET_FORCEINLINE constexpr auto emit_blocks_iso(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    (run_block_iso<Func, Begin, Step, Is * BlockSize>(func, std::make_index_sequence<BlockSize>{}), ...);
}

POET_POP_OPTIMIZE

/// \brief Adapter for callables that expose `template <auto I> operator()()`.
///
/// Bridges the integral_constant-based static_for machinery to functors
/// with a template call operator instead of an argument-taking operator.
template<typename Functor> struct template_invoker {
    Functor &functor;

    template<std::ptrdiff_t Value>
    POET_FORCEINLINE constexpr auto operator()(std::integral_constant<std::ptrdiff_t, Value> /*ic*/) const -> void {
        functor.template operator()<Value>();
    }
};

}// namespace poet::detail
