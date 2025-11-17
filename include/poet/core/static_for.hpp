#ifndef POET_CORE_STATIC_FOR_HPP
#define POET_CORE_STATIC_FOR_HPP

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include <poet/core/for_utils.hpp>

namespace poet {

namespace detail {

template <typename Func, std::intmax_t Begin, std::intmax_t Step,
          std::size_t BlockSize, std::size_t... BlockIndices>
constexpr void static_loop_emit_all_blocks(
    Func &func, std::index_sequence<BlockIndices...> /*blocks*/) {
  using blocks_tuple =
      std::tuple<std::integral_constant<std::size_t, BlockIndices>...>;
  emit_all_blocks<Func, Begin, Step, BlockSize>(func, blocks_tuple{});
}

template <std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
constexpr std::size_t compute_default_static_loop_block_size() noexcept {
  constexpr auto count = detail::compute_range_count<Begin, End, Step>();
  if constexpr (count == 0) {
    return 1;
  }
  if constexpr (count > detail::kMaxStaticLoopBlock) {
    return detail::kMaxStaticLoopBlock;
  }
  return count;
}

template <std::intmax_t Begin, std::intmax_t End, std::intmax_t Step = 1,
          std::size_t BlockSize =
              compute_default_static_loop_block_size<Begin, End, Step>(),
          typename Func>
constexpr void static_loop(Func &&func) {
  static_assert(BlockSize > 0, "static_loop requires BlockSize > 0");
  using Callable = std::remove_reference_t<Func>;
  Callable callable(std::forward<Func>(func));

  constexpr auto count = detail::compute_range_count<Begin, End, Step>();
  if constexpr (count == 0) {
    (void)callable;
    return;
  }

  constexpr auto full_blocks = count / BlockSize;
  constexpr auto remainder = count % BlockSize;

  if constexpr (full_blocks > 0) {
    static_loop_emit_all_blocks<Callable, Begin, Step, BlockSize>(
        callable, std::make_index_sequence<full_blocks>{});
  }

  if constexpr (remainder > 0) {
    static_loop_impl_block<Callable, Begin, Step, full_blocks * BlockSize>(
        callable, std::make_index_sequence<remainder>{});
  }
}

} // namespace poet::detail

/// \brief Adapts a functor with a `template <auto I>` call operator to
/// `static_loop` using the specified step.
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
/// total iteration count, clamped to `1` for empty ranges).
/// \tparam Func Functor exposing `template <auto I>` call operators.
/// \param func Functor instance invoked for each iteration.
// Variant that forwards callables that accept an integral-constant
// parameter. This is the constexpr-friendly form: the lambda receives a
// `std::integral_constant` and can therefore access its value in a
// compile-time context via `decltype(arg)::value`.
// Unified `static_for` API.
// Prefer callables invocable with `std::integral_constant` (constexpr-friendly
// lambdas). If the callable is not invocable that way, fall back to the
// original adapter that wraps functors exposing `template <auto I>` call
// operators.
template <std::intmax_t Begin, std::intmax_t End, std::intmax_t Step = 1,
          std::size_t BlockSize =
              detail::compute_default_static_loop_block_size<Begin, End, Step>(),
          typename Func>
constexpr void static_for(Func &&func) {
  using Functor = std::remove_reference_t<Func>;

  if constexpr (std::is_invocable_v<Functor,
                                    std::integral_constant<std::intmax_t,
                                                           Begin>>) {
    // Constexpr-friendly callable: forwards integral_constant to the
    // user-provided callable. Preserve lvalue references so the
    // original callable (e.g., accumulators) is updated rather than a
    // local copy.
    if constexpr (std::is_lvalue_reference_v<Func>) {
      detail::static_loop<Begin, End, Step, BlockSize>(func);
    } else {
      Functor callable(std::forward<Func>(func));
      detail::static_loop<Begin, End, Step, BlockSize>(callable);
    }
  } else {
    // Fallback: adapter for functors exposing `template <auto I>` call
    // operators (original behaviour). For lvalues we pass the address of
    // the original object so the adapter updates it; for rvalues we make a
    // local copy and point at that.
    if constexpr (std::is_lvalue_reference_v<Func>) {
      // Wrap the lvalue functor in a lambda that accepts the
      // integral_constant and invokes the functor's template operator.
      auto adapter = [&](auto index_constant) {
        func.template operator()<decltype(index_constant)::value>();
      };
      detail::static_loop<Begin, End, Step, BlockSize>(adapter);
    } else {
      Functor functor(std::forward<Func>(func));
      auto adapter = [&](auto index_constant) {
        functor.template operator()<decltype(index_constant)::value>();
      };
      detail::static_loop<Begin, End, Step, BlockSize>(adapter);
    }
  }
}

} // namespace poet

#endif // POET_CORE_STATIC_FOR_HPP
