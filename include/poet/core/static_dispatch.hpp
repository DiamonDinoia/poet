#ifndef POET_CORE_STATIC_DISPATCH_HPP
#define POET_CORE_STATIC_DISPATCH_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <optional>
#include <functional>

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

// Helper to extract the minimum value from a sequence
template <typename Sequence>
struct sequence_min;

template <int First, int... Rest>
struct sequence_min<std::integer_sequence<int, First, Rest...>>
    : std::integral_constant<int, First> {};

// Helper to extract the maximum value from a sequence
template <typename Sequence>
struct sequence_max;

template <int First>
struct sequence_max<std::integer_sequence<int, First>>
    : std::integral_constant<int, First> {};

template <int First, int Second, int... Rest>
struct sequence_max<std::integer_sequence<int, First, Second, Rest...>>
    : std::integral_constant<int, (First > sequence_max<std::integer_sequence<int, Second, Rest...>>::value
                                   ? First
                                   : sequence_max<std::integer_sequence<int, Second, Rest...>>::value)> {};

// Helper to compute the size of a sequence
template <typename Sequence>
struct sequence_size;

template <int... Values>
struct sequence_size<std::integer_sequence<int, Values...>>
    : std::integral_constant<std::size_t, sizeof...(Values)> {};

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

// O(1) dispatcher using array-based lookup
template <typename Functor, typename ArgumentTuple, typename ResultType, typename... Seq>
struct array_dispatcher {
  static constexpr std::size_t table_size = (sequence_size<Seq>::value * ...);

  using function_type = std::conditional_t<std::is_void_v<ResultType>,
                                           std::function<void()>,
                                           std::function<ResultType()>>;

  // Compute index from compile-time parameters
  template <int... Params>
  static constexpr std::size_t param_index() {
    return param_index_impl<0, Params...>();
  }

  template <std::size_t Idx, int Param, int... Rest>
  static constexpr std::size_t param_index_impl() {
    using CurrentSeq = std::tuple_element_t<Idx, std::tuple<Seq...>>;
    constexpr int min_val = sequence_min<CurrentSeq>::value;
    constexpr std::size_t offset = static_cast<std::size_t>(Param - min_val);

    if constexpr (sizeof...(Rest) > 0) {
      constexpr std::size_t stride = remaining_product<Idx + 1>();
      return offset * stride + param_index_impl<Idx + 1, Rest...>();
    } else {
      return offset;
    }
  }

  template <std::size_t StartIdx>
  static constexpr std::size_t remaining_product() {
    return remaining_product_impl<StartIdx>(std::make_index_sequence<sizeof...(Seq) - StartIdx>{});
  }

  template <std::size_t StartIdx, std::size_t... Is>
  static constexpr std::size_t remaining_product_impl(std::index_sequence<Is...>) {
    return (sequence_size<std::tuple_element_t<StartIdx + Is, std::tuple<Seq...>>>::value * ...);
  }

  // Compute index from runtime values
  static std::optional<std::size_t> runtime_index(const std::array<int, sizeof...(Seq)>& runtime_vals) {
    return runtime_index_impl<0>(runtime_vals);
  }

  template <std::size_t Idx>
  static std::optional<std::size_t> runtime_index_impl(const std::array<int, sizeof...(Seq)>& vals) {
    if constexpr (Idx >= sizeof...(Seq)) {
      return 0;
    } else {
      using CurrentSeq = std::tuple_element_t<Idx, std::tuple<Seq...>>;
      constexpr int min_val = sequence_min<CurrentSeq>::value;
      constexpr int max_val = sequence_max<CurrentSeq>::value;
      const int val = vals[Idx];

      if (val < min_val || val > max_val) {
        return std::nullopt;
      }

      const std::size_t offset = static_cast<std::size_t>(val - min_val);

      if constexpr (Idx + 1 < sizeof...(Seq)) {
        auto rest = runtime_index_impl<Idx + 1>(vals);
        if (!rest) {
          return std::nullopt;
        }
        constexpr std::size_t stride = remaining_product<Idx + 1>();
        return offset * stride + *rest;
      } else {
        return offset;
      }
    }
  }

  // Populate one entry in the lookup table
  template <int... Params>
  static void populate_entry(std::array<std::optional<function_type>, table_size>& table,
                            Functor& functor, ArgumentTuple& args) {
    constexpr std::size_t idx = param_index<Params...>();

    if constexpr (std::is_void_v<ResultType>) {
      table[idx] = [&functor, &args]() {
        std::apply([&functor](auto&&... a) {
          functor.template operator()<Params...>(std::forward<decltype(a)>(a)...);
        }, args);
      };
    } else {
      table[idx] = [&functor, &args]() -> ResultType {
        return std::apply([&functor](auto&&... a) -> ResultType {
          return functor.template operator()<Params...>(std::forward<decltype(a)>(a)...);
        }, args);
      };
    }
  }

  // Cartesian product expansion
  template <int... HeadValues, typename... Tail>
  static void populate_all(std::array<std::optional<function_type>, table_size>& table,
                          Functor& functor, ArgumentTuple& args,
                          std::integer_sequence<int, HeadValues...>, Tail... tail) {
    (populate_with_prefix<HeadValues>(table, functor, args, tail...), ...);
  }

  template <int... Prefix, int... NextValues, typename... Tail>
  static void populate_with_prefix(std::array<std::optional<function_type>, table_size>& table,
                                   Functor& functor, ArgumentTuple& args,
                                   std::integer_sequence<int, NextValues...>, Tail... tail) {
    (populate_with_prefix<Prefix..., NextValues>(table, functor, args, tail...), ...);
  }

  template <int... Params>
  static void populate_with_prefix(std::array<std::optional<function_type>, table_size>& table,
                                   Functor& functor, ArgumentTuple& args) {
    populate_entry<Params...>(table, functor, args);
  }
};

}  // namespace poet::detail

/// \brief Dispatches runtime integers to compile-time template parameters with O(1) lookup.
///
/// The functor is invoked with the template parameter combination whose
/// values match the supplied runtime inputs. When no match exists, the functor
/// is never invoked and a default-constructed result is returned for
/// non-void functors.
///
/// This implementation uses an array-based lookup table for O(1) dispatch performance,
/// trading memory for speed compared to the linear search approach.
///
/// The functor must expose a templated call operator of the form
/// `template <int... Params> Result operator()(Args...)`, where `Args...` are
/// the runtime arguments forwarded through `dispatch`.
///
/// \param functor Callable exposing `template <int...>` call operators.
/// \param params Tuple of `DispatchParam` values describing the candidate ranges.
/// \param args Runtime arguments forwarded to the functor upon invocation.
/// \return The functor's result when non-void; otherwise `void`.
/// \note When `ResultType` is `void`, the helper performs the side effects but
///       does not return a value.
template <typename Functor, typename ParamTuple, typename... Args>
decltype(auto) dispatch(Functor &&functor, ParamTuple &&params, Args &&...args) {
  auto runtime_values = detail::extract_runtime_values(params);
  auto sequences = detail::extract_sequences(params);
  auto argument_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
  using result_type = detail::dispatch_result_t<Functor, decltype(argument_tuple),
                                                decltype(sequences)>;

  // Build lookup table
  return std::apply(
      [&](auto &&...seq) -> decltype(auto) {
        using dispatcher = detail::array_dispatcher<Functor, decltype(argument_tuple),
                                                    result_type, std::remove_reference_t<decltype(seq)>...>;

        typename dispatcher::function_type table[dispatcher::table_size] = {};
        std::array<std::optional<typename dispatcher::function_type>, dispatcher::table_size> opt_table;

        // Populate the table with all combinations
        dispatcher::populate_all(opt_table, functor, argument_tuple,
                                std::forward<decltype(seq)>(seq)...);

        // Lookup and invoke
        auto idx = dispatcher::runtime_index(runtime_values);
        if (idx && opt_table[*idx]) {
          if constexpr (std::is_void_v<result_type>) {
            (*opt_table[*idx])();
          } else {
            return (*opt_table[*idx])();
          }
        } else {
          if constexpr (!std::is_void_v<result_type>) {
            return result_type{};
          }
        }
      },
      sequences);
}

}  // namespace poet

#endif  // POET_CORE_STATIC_DISPATCH_HPP
