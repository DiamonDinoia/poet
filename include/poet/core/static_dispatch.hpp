#ifndef POET_CORE_STATIC_DISPATCH_HPP
#define POET_CORE_STATIC_DISPATCH_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace poet {

/// \brief Helper generating integer sequences shifted by an offset.
///
/// The alias \c make_range builds a sequence of integers in the range
/// `[Start, End]` inclusive. The implementation internally uses
/// `std::integer_sequence` and composes the offset with the supplied range.
namespace detail {

    template<int Offset, typename Seq> struct offset_sequence;

    template<int Offset, int... Values> struct offset_sequence<Offset, std::integer_sequence<int, Values...>> {
        using type = std::integer_sequence<int, (Offset + Values)...>;
    };

}// namespace detail

// Public alias that composes the offset with an integer sequence.
template<int Start, int End>
using make_range = typename detail::offset_sequence<Start, std::make_integer_sequence<int, End - Start + 1>>::type;

/// \brief Wraps runtime dispatch parameters and their candidate sequences.
///
/// Each runtime value is paired with a `std::integer_sequence` that describes
/// the compile-time options that should be probed when dispatching.
/// The dispatcher compares the runtime inputs against all combinations of the
/// provided sequences and invokes the functor when a match is found.
template<typename Seq> struct DispatchParam {
    int runtime_val;
    using seq_type = Seq;
};

namespace detail {

    // Helper to extract the minimum value from a sequence
    template<typename Sequence> struct sequence_min;

    template<int First, int... Rest>
    struct sequence_min<std::integer_sequence<int, First, Rest...>> : std::integral_constant<int, First> {};

    // Helper to extract the maximum value from a sequence (uses fold expression to avoid O(N) recursion depth)
    template<typename Sequence> struct sequence_max;

    template<int First, int... Rest>
    struct sequence_max<std::integer_sequence<int, First, Rest...>>
      : std::integral_constant<int, std::max({First, Rest...})> {};

    // Helper to compute the size of a sequence
    template<typename Sequence> struct sequence_size;

    template<int... Values>
    struct sequence_size<std::integer_sequence<int, Values...>>
      : std::integral_constant<std::size_t, sizeof...(Values)> {};

    // Helper to find the index of a value in a sequence at compile-time
    template<int Value, typename Sequence> struct sequence_index_of;

    template<int Value, int First, int... Rest>
    struct sequence_index_of<Value, std::integer_sequence<int, First, Rest...>> {
        static constexpr std::size_t value =
          (Value == First) ? 0 : (1 + sequence_index_of<Value, std::integer_sequence<int, Rest...>>::value);
    };

    template<int Value> struct sequence_index_of<Value, std::integer_sequence<int>> {
        static constexpr std::size_t value = std::size_t(-1);// Not found
    };

    // Helper to check if a value exists in a sequence
    template<int Value, typename Sequence> struct sequence_contains;

    template<int Value, int... Values>
    struct sequence_contains<Value, std::integer_sequence<int, Values...>>
      : std::bool_constant<((Value == Values) || ...)> {};

    // Helper to check that all values in a sequence are unique
    template<typename Sequence> struct sequence_unique;

    template<> struct sequence_unique<std::integer_sequence<int>> : std::true_type {};

    template<int First, int... Rest>
    struct sequence_unique<std::integer_sequence<int, First, Rest...>>
        : std::bool_constant<!sequence_contains<First, std::integer_sequence<int, Rest...>>::value && sequence_unique<std::integer_sequence<int, Rest...>>::value> {};

    // Compile-time predicate: true when sequence is contiguous ascending and unique
    // Primary template: false for non-sequence types
    template<typename Seq>
    struct is_contiguous_sequence : std::false_type {};

    // Specialization for integer_sequence: checks (max - min + 1 == count) && unique
    template<int... Values>
    struct is_contiguous_sequence<std::integer_sequence<int, Values...>>
        : std::bool_constant<((sequence_max<std::integer_sequence<int, Values...>>::value 
            - sequence_min<std::integer_sequence<int, Values...>>::value + 1) 
            == static_cast<int>(sizeof...(Values))) 
            && sequence_unique<std::integer_sequence<int, Values...>>::value> {};

    // Runtime helper to find index of a value in a sequence
    template<typename Sequence> struct sequence_runtime_index;

    template<int... Values> struct sequence_runtime_index<std::integer_sequence<int, Values...>> {
        static std::optional<std::size_t> find(int value) {
            constexpr std::array<int, sizeof...(Values)> vals = { Values... };
            for (std::size_t i = 0; i < vals.size(); ++i) {
                if (vals[i] == value) { return i; }
            }
            return std::nullopt;
        }
    };

    template<typename Sequence> struct sequence_first;

    template<int First, int... Rest>
    struct sequence_first<std::integer_sequence<int, First, Rest...>> : std::integral_constant<int, First> {};



    template<typename Tuple, std::size_t... Indices>
    auto extract_sequences_impl(const Tuple &, std::index_sequence<Indices...>) {
        using TupleType = std::remove_reference_t<Tuple>;
        return std::make_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{}...);
    }

    template<typename Tuple> auto extract_sequences(const Tuple &tuple) {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl(tuple, std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

    template<typename Functor, typename ArgumentTuple, typename... Seq> struct dispatch_result_helper {
        template<std::size_t... Indices>
        static auto test(std::index_sequence<Indices...>)
          -> decltype(std::declval<Functor>().template operator()<sequence_first<Seq>::value...>(
            std::get<Indices>(std::declval<ArgumentTuple>())...));

        using type = decltype(test(std::make_index_sequence<std::tuple_size_v<ArgumentTuple>>{}));
    };

    template<typename Functor, typename ArgumentTuple, typename SequenceTuple> struct dispatch_result;

    template<typename Functor, typename ArgumentTuple, typename... Seq>
    struct dispatch_result<Functor, ArgumentTuple, std::tuple<Seq...>> {
        using type = typename dispatch_result_helper<Functor, ArgumentTuple, Seq...>::type;
    };

    template<typename Functor, typename ArgumentTuple, typename SequenceTuple>
    using dispatch_result_t = typename dispatch_result<Functor, ArgumentTuple, SequenceTuple>::type;

    // Variant + visit dispatch (O(1) mapping per-dimension for contiguous ascending integer sequences)
    template<int... I> struct variant_maker {
        using V = std::variant<std::integral_constant<int, I>...>;
        
        template<std::size_t... Idx>
        static constexpr std::array<V, sizeof...(I)> make_table(std::index_sequence<Idx...>) {
            return { V(std::in_place_index<Idx>)... };
        }

        static V make_by_index(std::size_t idx) {
            static constexpr auto tbl = make_table(std::make_index_sequence<sizeof...(I)>{});
            return tbl[idx];
        }
    };

    template<typename Seq> struct variant_from_seq;
    template<int... I> struct variant_from_seq<std::integer_sequence<int, I...>> {
        using maker = variant_maker<I...>;
        using type  = typename maker::V;
    };
    template<typename Seq> using variant_from_seq_t = typename variant_from_seq<Seq>::type;

    // Overload enabled when the sequence is contiguous+unique: construct and
    // return the corresponding variant alternative for the runtime value.
    template<typename Seq, std::enable_if_t<is_contiguous_sequence<Seq>::value, int> = 0>
    std::optional<variant_from_seq_t<Seq>> to_variant(int v, Seq) {
        constexpr int first = sequence_first<Seq>::value;
        constexpr std::size_t len = sequence_size<Seq>::value;
        const int idx = v - first;
        // Single-comparison bounds check: casting to unsigned makes negative idx wrap to large value
        if (static_cast<unsigned>(idx) >= static_cast<unsigned>(len)) return std::nullopt;
        return variant_from_seq<Seq>::maker::make_by_index(static_cast<std::size_t>(idx));
    }

    // Overload for general sequences (linear search)
    template<typename Seq, std::enable_if_t<!is_contiguous_sequence<Seq>::value, int> = 0>
    std::optional<variant_from_seq_t<Seq>> to_variant(int v, Seq) {
        auto idx = sequence_runtime_index<Seq>::find(v);
        if (!idx) return std::nullopt;
        return variant_from_seq<Seq>::maker::make_by_index(*idx);
    }

    template<typename ParamTuple, std::size_t... Idx>
    auto make_variants(const ParamTuple &params, std::index_sequence<Idx...>) {
        using P = std::decay_t<ParamTuple>;
        return std::make_tuple(
            to_variant(std::get<Idx>(params).runtime_val,
                       typename std::tuple_element<Idx, P>::type::seq_type{})...);
    }

    template<typename Functor, typename ArgumentTuple>
    struct VisitCaller {
      Functor *func;
      ArgumentTuple *args;
      template<typename... IC>
      auto operator()(IC...) -> decltype(auto) {
                // move the stored argument tuple into the call so move-only arguments are forwarded
                return std::apply([this](auto &&...a) -> decltype(auto) {
                    return this->func->template operator()<IC::value...>(std::forward<decltype(a)>(a)...);
                }, std::move(*args));
      }
    };

}// namespace detail

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
template<typename Functor, typename ParamTuple, typename... Args>
decltype(auto) dispatch(Functor &&functor, ParamTuple &&params, Args &&...args) {
    auto sequences = detail::extract_sequences(params);
    using argument_tuple_type = std::tuple<std::decay_t<Args>...>;
    argument_tuple_type argument_tuple(std::forward<Args>(args)...);
    using result_type = detail::dispatch_result_t<Functor, argument_tuple_type, decltype(sequences)>;

    // Build variants for each runtime value
    constexpr std::size_t N = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
    auto variants = detail::make_variants(params, std::make_index_sequence<N>{});

    // ensure all mapped successfully at runtime
    bool ok = std::apply([](auto const&... vo){ return (vo.has_value() && ...); }, variants);

    if (ok) {
        detail::VisitCaller<std::decay_t<Functor>, decltype(argument_tuple)> caller{ &functor, &argument_tuple };
        if constexpr (std::is_void_v<result_type>) {
            std::apply([&](auto&... vo){ std::visit(caller, *vo...); }, variants);
        } else {
            return std::apply([&](auto&... vo){ return std::visit(caller, *vo...); }, variants);
        }
    } else {
         if constexpr (!std::is_void_v<result_type>) return result_type{};
    }
}

}// namespace poet

#endif// POET_CORE_STATIC_DISPATCH_HPP
