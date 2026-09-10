#pragma once

/// \file for_utils.hpp
/// \brief Internal helpers shared by the loop primitives.

#include <cstddef>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>

namespace poet::detail {

/// Binds an lvalue callable as-is; materialises an rvalue into a named local so
/// the loop bodies can take it by reference without a lambda indirection.
template<typename Func>
using callable_storage_t = std::conditional_t<std::is_lvalue_reference_v<Func>, Func, std::remove_reference_t<Func>>;

template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::ptrdiff_t Step>
[[nodiscard]] POET_CPP20_CONSTEVAL auto compute_range_count() noexcept -> std::size_t {
    static_assert(Step != 0, "static_for requires a non-zero step");
    if constexpr (Step > 0) {
        static_assert(Begin <= End, "static_for with a positive step requires Begin <= End");
    } else {
        static_assert(Begin >= End, "static_for with a negative step requires Begin >= End");
    }
    if constexpr (Begin == End) { return 0; }
    constexpr auto distance = (End - Begin) < 0 ? -(End - Begin) : (End - Begin);
    constexpr auto magnitude = Step < 0 ? -Step : Step;
    return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
}

/// Expands `[StartIndex, StartIndex + sizeof...(Is))` of the range as a fold.
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_FORCEINLINE constexpr auto run_block(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr std::ptrdiff_t Base = Begin + (Step * static_cast<std::ptrdiff_t>(StartIndex));
    (func(std::integral_constant<std::ptrdiff_t, Base + (Step * static_cast<std::ptrdiff_t>(Is))>{}), ...);
}

/// Same expansion, outlined so each block gets its own register allocation.
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_NOINLINE_FLATTEN constexpr auto run_block_isolated(Func &func, std::index_sequence<Is...> seq) -> void {
    run_block<Func, Begin, Step, StartIndex>(func, seq);
}

template<bool Isolate,
  typename Func,
  std::ptrdiff_t Begin,
  std::ptrdiff_t Step,
  std::size_t BlockSize,
  std::size_t... Is>
POET_FORCEINLINE constexpr auto emit_blocks(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr auto block = std::make_index_sequence<BlockSize>{};
    if constexpr (Isolate) {
        (run_block_isolated<Func, Begin, Step, Is * BlockSize>(func, block), ...);
    } else {
        (run_block<Func, Begin, Step, Is * BlockSize>(func, block), ...);
    }
}

template<typename Functor> struct template_invoker {
    Functor &functor;

    template<std::ptrdiff_t Value>
    POET_FORCEINLINE constexpr auto operator()(std::integral_constant<std::ptrdiff_t, Value> /*ic*/) const -> void {
        functor.template operator()<Value>();
    }
};

}// namespace poet::detail
