#ifndef POET_CORE_FOR_UTILS_HPP
#define POET_CORE_FOR_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

namespace poet {

inline constexpr std::size_t kMaxStaticLoopBlock = 256;

/// \brief Computes the number of elements produced by a compile-time range.
///
/// The resulting count corresponds to the number of iterations performed by a
/// loop starting at `Begin`, repeatedly adding `Step`, and stopping before
/// reaching `End`. Positive and negative step values are supported as long as
/// the range progresses toward the end point.
///
/// \tparam Begin The initial value of the range.
/// \tparam End The exclusive terminator of the range.
/// \tparam Step The increment applied after each iteration.
/// \return The number of steps required to reach the end of the range.
/// \throws Nothing.
/// \note Attempts to use a zero step or a step that cannot reach the end will
/// trigger a compile-time diagnostic.
template <std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
[[nodiscard]] constexpr std::size_t compute_range_count() noexcept {
  static_assert(Step != 0, "static_for requires a non-zero step");

  if constexpr (Step > 0) {
    static_assert(Begin <= End,
                  "static_for with a positive step requires Begin <= End");
    if constexpr (Begin == End) {
      return 0;
    }
    const auto distance = End - Begin;
    return static_cast<std::size_t>((distance + Step - 1) / Step);
  } else {
    static_assert(Begin >= End,
                  "static_for with a negative step requires Begin >= End");
    if constexpr (Begin == End) {
      return 0;
    }
    const auto distance = Begin - End;
    const auto magnitude = -Step;
    return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
  }
}

namespace detail {

template <typename Func, std::intmax_t Begin, std::intmax_t Step,
          std::size_t StartIndex, std::size_t... Is>
constexpr void static_loop_impl_block(
    Func &func, std::index_sequence<Is...> /*indices*/) {
  (func(std::integral_constant<std::intmax_t,
                               Begin + Step * static_cast<std::intmax_t>(
                                             StartIndex + Is)>{}), ...);
}

template <typename Func, std::intmax_t Begin, std::intmax_t Step,
          std::size_t BlockSize, std::size_t Offset, typename Tuple,
          std::size_t... Is>
constexpr void emit_block_chunk(Func &func, const Tuple & /*tuple*/,
                                std::index_sequence<Is...>) {
  (static_loop_impl_block<
       Func, Begin, Step,
       std::tuple_element_t<Offset + Is, Tuple>::value * BlockSize>(
       func, std::make_index_sequence<BlockSize>{}),
   ...);
}

template <typename Func, std::intmax_t Begin, std::intmax_t Step,
          std::size_t BlockSize, typename Tuple, std::size_t Offset,
          std::size_t Remaining>
constexpr void emit_all_blocks_from_tuple(Func &func, const Tuple &tuple) {
  if constexpr (Remaining > 0) {
    constexpr auto chunk_size =
        Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;
    emit_block_chunk<Func, Begin, Step, BlockSize, Offset>(
        func, tuple, std::make_index_sequence<chunk_size>{});
    emit_all_blocks_from_tuple<Func, Begin, Step, BlockSize, Tuple,
                               Offset + chunk_size, Remaining - chunk_size>(
        func, tuple);
  } else {
    (void)func;
    (void)tuple;
  }
}

template <typename Func, std::intmax_t Begin, std::intmax_t Step,
          std::size_t BlockSize, typename Tuple>
constexpr void emit_all_blocks(Func &func, const Tuple &tuple) {
  constexpr auto total_blocks = std::tuple_size<Tuple>::value;
  if constexpr (total_blocks > 0) {
    emit_all_blocks_from_tuple<Func, Begin, Step, BlockSize, Tuple, 0,
                               total_blocks>(func, tuple);
  } else {
    (void)func;
    (void)tuple;
  }
}

template <typename Functor>
struct template_static_loop_invoker {
  Functor &functor;

  template <std::intmax_t Value>
  constexpr void
  operator()(std::integral_constant<std::intmax_t, Value>) const {
    functor.template operator()<Value>();
  }
};

} // namespace poet::detail

} // namespace poet

#endif // POET_CORE_FOR_UTILS_HPP
