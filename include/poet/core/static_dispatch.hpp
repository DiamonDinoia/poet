#ifndef POET_CORE_STATIC_DISPATCH_HPP
#define POET_CORE_STATIC_DISPATCH_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace poet {

/// \brief Helper generating integer sequences shifted by an offset.
///
/// The alias \c make_range builds a sequence of integers in the range
/// `[Start, End]` inclusive. The implementation internally uses
/// `std::integer_sequence` and composes the offset with the supplied range.
namespace detail {

template <int Offset, typename Seq>
struct offset_sequence;

template <int Offset, int... Values>
struct offset_sequence<Offset, std::integer_sequence<int, Values...>> {
  using type = std::integer_sequence<int, (Offset + Values)...>;
};

}  // namespace poet::detail

// Public alias that composes the offset with an integer sequence.
template <int Start, int End>
using make_range = typename detail::offset_sequence<
    Start, std::make_integer_sequence<int, End - Start + 1>>::type;

/// \brief Wraps runtime dispatch parameters and their candidate sequences.
///
/// Each runtime value is paired with a `std::integer_sequence` that describes
/// the compile-time options that should be probed when dispatching.
/// The dispatcher compares the runtime inputs against all combinations of the
/// provided sequences and invokes the functor when a match is found.
template <typename Seq>
struct DispatchParam {
  int runtime_val;
  using seq_type = Seq;
};

namespace detail {

template <typename Functor, typename... Seq>
struct cartesian_product;

template <typename Functor, int... HeadValues, typename Seq, typename... Tail>
struct cartesian_product<Functor, std::integer_sequence<int, HeadValues...>, Seq,
                         Tail...> {
  template <int... Prefix>
  static void apply(Functor &functor) {
    (cartesian_product<Functor, Seq, Tail...>::template apply<Prefix..., HeadValues>(
         functor),
     ...);
  }
};

template <typename Functor, int... HeadValues>
struct cartesian_product<Functor, std::integer_sequence<int, HeadValues...>> {
  template <int... Prefix>
  static void apply(Functor &functor) {
    (functor.template operator()<Prefix..., HeadValues>(), ...);
  }
};

template <typename Functor, typename... Seq>
void product(Functor &functor, Seq...) {
  cartesian_product<Functor, Seq...>::template apply<>(functor);
}

template <typename Functor, std::size_t N, typename ArgumentTuple,
          typename ResultType>
struct dispatcher_caller {
  Functor &functor;
  const std::array<int, N> &runtime_values;
  ArgumentTuple &arguments;
  std::conditional_t<std::is_void_v<ResultType>, char, ResultType> result{};

  template <int... Params>
  void operator()() {
    static constexpr std::array<int, sizeof...(Params)> probe{Params...};
    if (probe == runtime_values) {
      if constexpr (std::is_void_v<ResultType>) {
        std::apply(
            [&](auto &&...args) {
              functor.template operator()<Params...>(
                  std::forward<decltype(args)>(args)...);
            },
            arguments);
      } else {
        result = std::apply(
            [&](auto &&...args) {
              return functor.template operator()<Params...>(
                  std::forward<decltype(args)>(args)...);
            },
            arguments);
      }
    }
  }
};

template <typename Sequence>
struct sequence_first;

template <int First, int... Rest>
struct sequence_first<std::integer_sequence<int, First, Rest...>>
    : std::integral_constant<int, First> {};

template <typename Tuple, std::size_t... Indices>
auto extract_runtime_values_impl(const Tuple &tuple,
                                 std::index_sequence<Indices...>) {
  return std::array<int, sizeof...(Indices)>{std::get<Indices>(tuple).runtime_val...};
}

template <typename Tuple>
auto extract_runtime_values(const Tuple &tuple) {
  using TupleType = std::remove_reference_t<Tuple>;
  return extract_runtime_values_impl(
      tuple, std::make_index_sequence<std::tuple_size_v<TupleType>>{});
}

template <typename Tuple, std::size_t... Indices>
auto extract_sequences_impl(const Tuple &,
                            std::index_sequence<Indices...>) {
  using TupleType = std::remove_reference_t<Tuple>;
  return std::make_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{}...);
}

template <typename Tuple>
auto extract_sequences(const Tuple &tuple) {
  using TupleType = std::remove_reference_t<Tuple>;
  return extract_sequences_impl(
      tuple, std::make_index_sequence<std::tuple_size_v<TupleType>>{});
}

template <typename Functor, typename ArgumentTuple, typename... Seq>
struct dispatch_result_helper {
  template <std::size_t... Indices>
  static auto test(std::index_sequence<Indices...>)
      -> decltype(std::declval<Functor>().template operator()<
                  sequence_first<Seq>::value...>(
          std::get<Indices>(std::declval<ArgumentTuple>())...));

  using type = decltype(test(std::make_index_sequence<
                          std::tuple_size_v<ArgumentTuple>>{}));
};

template <typename Functor, typename ArgumentTuple, typename SequenceTuple>
struct dispatch_result;

template <typename Functor, typename ArgumentTuple, typename... Seq>
struct dispatch_result<Functor, ArgumentTuple, std::tuple<Seq...>> {
  using type = typename dispatch_result_helper<Functor, ArgumentTuple, Seq...>::type;
};

template <typename Functor, typename ArgumentTuple, typename SequenceTuple>
using dispatch_result_t = typename dispatch_result<Functor, ArgumentTuple,
                                                   SequenceTuple>::type;

}  // namespace poet::detail

/// \brief Dispatches runtime integers to compile-time template parameters.
///
/// The functor is invoked with the first template parameter combination whose
/// values match the supplied runtime inputs. When no match exists, the functor
/// is never invoked and a default-constructed result is returned.
///
/// \param functor Callable exposing `template <int...>` call operators.
/// \param params Tuple of `DispatchParam` values describing the candidate ranges.
/// \param args Runtime arguments forwarded to the functor upon invocation.
/// \return The functor's result when non-void.
/// \note When `ResultType` is `void`, the helper performs the side effects but
///       does not return a value.
template <typename Functor, typename ParamTuple, typename... Args>
decltype(auto) dispatch(Functor &&functor, ParamTuple &&params, Args &&...args) {
  using TupleType = std::remove_reference_t<ParamTuple>;
  constexpr std::size_t param_count = std::tuple_size_v<TupleType>;
  auto runtime_values = detail::extract_runtime_values(params);
  auto sequences = detail::extract_sequences(params);
  auto argument_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
  using result_type = detail::dispatch_result_t<Functor, decltype(argument_tuple),
                                                decltype(sequences)>;
  detail::dispatcher_caller<Functor, param_count, decltype(argument_tuple),
                            result_type>
      caller{functor, runtime_values, argument_tuple};
  std::apply(
      [&](auto &&...seq) { detail::product(caller, std::forward<decltype(seq)>(seq)...); },
      sequences);
  if constexpr (!std::is_void_v<result_type>) {
    return caller.result;
  }
}

}  // namespace poet

#endif  // POET_CORE_STATIC_DISPATCH_HPP
