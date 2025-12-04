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
#include <stdexcept>

namespace poet {

// Helper for concise tuple syntax in DispatchSet
template<auto... Vs> struct T {};

/// \brief Helper generating integer sequences shifted by an offset.
///
/// The alias \c make_range builds a sequence of integers in the range
/// `[Start, End]` inclusive. The implementation internally uses
/// `std::integer_sequence` and composes the offset with the supplied range.
namespace detail {

    template<typename T> struct result_holder {
        std::optional<T> val;
        template<typename U> void set(U&& v) { val = std::forward<U>(v); }
        T get() { return *val; }
        bool has_value() const { return val.has_value(); }
    };
    template<> struct result_holder<void> {
        bool set_called = false;
        void set() { set_called = true; }
        void get() {}
        bool has_value() const { return set_called; }
    };

    // Helper: call functor with the integer pack from an integer_sequence
    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args>
    struct seq_matcher_helper;

    template<typename ValueType, ValueType... V, typename ResultType, typename RuntimeTuple, typename Functor, typename... Args>
    struct seq_matcher_helper<std::integer_sequence<ValueType, V...>, ResultType, RuntimeTuple, Functor, Args...> {
        template<std::size_t... Idx, typename F>
        static result_holder<ResultType> impl(std::index_sequence<Idx...>, const RuntimeTuple &rt, F &&f, Args&&... args) {
            result_holder<ResultType> res;
            if (((std::get<Idx>(rt) == V) && ...)) {
                if constexpr (std::is_void_v<ResultType>) {
                    std::forward<F>(f).template operator()<V...>(std::forward<Args>(args)...);
                    res.set();
                } else {
                    res.set(std::forward<F>(f).template operator()<V...>(std::forward<Args>(args)...));
                }
            }
            return res;
        }

        template<typename F>
        static result_holder<ResultType> match_and_call(const RuntimeTuple &rt, F &&f, Args&&... args) {
            return impl(std::make_index_sequence<sizeof...(V)>{}, rt, std::forward<F>(f), std::forward<Args>(args)...);
        }
    };

    // Helper to compute result type for tuple-sequence calls
    template<typename Seq, typename Functor, typename... Args>
    struct result_of_seq_call;

    template<typename ValueType, ValueType... V, typename Functor, typename... Args>
    struct result_of_seq_call<std::integer_sequence<ValueType, V...>, Functor, Args...> {
        using type = decltype(std::declval<Functor>().template operator()<V...>(std::declval<Args>()...));
    };


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

} // namespace detail


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

    // Helper: compare two integer_sequences for element-wise equality
    template<typename A, typename B> struct seq_equal;
    template<typename T, T... A, T... B>
    struct seq_equal<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>> : std::bool_constant<((A == B) && ...)> {};

    // Helper: check uniqueness among a pack of sequences
    template<typename... S> struct unique_helper;
    template<> struct unique_helper<> : std::true_type {};
    template<typename Head, typename... Rest>
    struct unique_helper<Head, Rest...> : std::bool_constant<(!(seq_equal<Head, Rest>::value || ...) && unique_helper<Rest...>::value)> {};



    // Helper trait: detect std::tuple specializations
    template<typename T> struct is_tuple : std::false_type {};
    template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    template<typename S>
    constexpr auto as_seq_tuple(S) {
        if constexpr (is_tuple<S>::value) {
            return S{}; // already a tuple of sequences
        } else {
            return std::tuple<S>{S{}}; // wrap single sequence into a tuple
        }
    }

    template<typename Tuple, std::size_t... Indices>
    auto extract_sequences_impl(const Tuple &, std::index_sequence<Indices...>) {
        using TupleType = std::remove_reference_t<Tuple>;
        // For each param's seq_type (which may itself be a std::tuple of sequences),
        // produce a tuple and concatenate them into a single flat tuple of sequences.
        return std::tuple_cat(as_seq_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{})...);
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

    // For a tuple-valued sequence (i.e. a compile-time list of tuples),
    // create a variant whose alternatives are tuples of integral_constants
    template<typename Seq> struct tuple_alt_from_seq;
    template<int... Vs>
    struct tuple_alt_from_seq<std::integer_sequence<int, Vs...>> {
        using type = std::tuple<std::integral_constant<int, Vs>...>;
    };

    template<typename SeqTuple> struct variant_from_tuple_seq;
    template<typename... Seq>
    struct variant_from_tuple_seq<std::tuple<Seq...>> {
        using type = std::variant<typename tuple_alt_from_seq<Seq>::type...>;
    };
    template<typename SeqTuple> using variant_from_tuple_seq_t = typename variant_from_tuple_seq<SeqTuple>::type;

    // Helper: compare runtime array against an integer_sequence
    template<int... Vs, std::size_t N>
    bool match_array_to_sequence(const std::array<int, N> &arr, std::integer_sequence<int, Vs...>) {
        if (N != sizeof...(Vs)) return false;
        constexpr int seq_vals[] = { Vs... };
        for (std::size_t i = 0; i < N; ++i) if (arr[i] != seq_vals[i]) return false;
        return true;
    }

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
                       typename std::tuple_element_t<Idx, P>::seq_type{})...);
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

// New helper type: DispatchSet representing a compile-time discrete set of allowed tuples
template<typename ValueType, typename... Tuples> struct DispatchSet {
    // Helper to convert T<Vs...> to std::integer_sequence<ValueType, Vs...>
    template<typename TupleHelper> struct convert_tuple;
    
    template<auto... Vs>
    struct convert_tuple<T<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;
    using first_t = std::tuple_element_t<0, seq_type>;

    static_assert(sizeof...(Tuples) >= 1, "DispatchSet requires at least one allowed tuple");

    // Ensure all tuples have the same arity
    template<typename S> struct seq_len;
    template<typename VType, VType... V> struct seq_len<std::integer_sequence<VType, V...>> : std::integral_constant<std::size_t, sizeof...(V)> {};

    static constexpr std::size_t tuple_arity = seq_len<first_t>::value;

    template<typename S> struct same_arity : std::bool_constant<seq_len<S>::value == tuple_arity> {};

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...), "All tuples in DispatchSet must have the same arity");

    // Ensure no duplicate tuples (compile-time equality)
    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value, "DispatchSet contains duplicate allowed tuples");

    // runtime stored tuple to compare against the allowed tuples (array sized by arity)
    using runtime_array_t = std::array<ValueType, tuple_arity>;
    runtime_array_t runtime_val;

    // Construct from variadic arguments
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    DispatchSet(Args&&... args) : runtime_val{static_cast<ValueType>(std::forward<Args>(args))...} {}

    // Produce a runtime std::tuple<ValueType,...> from the stored array
    template<std::size_t... Idx>
    auto runtime_tuple_impl(std::index_sequence<Idx...>) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

public:
    auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

namespace detail {
    template<typename T> struct is_dispatch_set : std::false_type {};
    template<typename V, typename... Tuples> struct is_dispatch_set<DispatchSet<V, Tuples...>> : std::true_type {};
}

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_t{};

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
template<typename Functor, typename ParamTuple, typename... Args, std::enable_if_t<!detail::is_dispatch_set<std::decay_t<ParamTuple>>::value && !std::is_same_v<std::decay_t<Functor>, throw_on_no_match_t>, int> = 0>
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






/// \brief Dispatch to an explicit compile-time list of tuples.
///
/// This overload allows dispatching based on an exact set of allowed tuple
/// combinations. The caller supplies a compile-time tuple of `std::integer_sequence<int, ...>`
/// values where each element encodes one allowed tuple (all tuples must have the
/// same arity). The runtime tuple is compared against each allowed tuple in
/// order; when a match is found the functor is invoked with the tuple elements
/// as non-type template parameters. If no match is found a default-constructed
/// result is returned (or an exception is thrown when `poet::throw_t` is used).
///
/// Example:
///   using List = std::tuple<std::integer_sequence<int,1,2>, std::integer_sequence<int,2,4>>;
///   auto rt = std::make_tuple(2,4);
///   dispatch_tuples(functor, List{}, rt, runtime_args...);
///
/// Optional policy tag: when passed as the first argument to `dispatch_tuples`,
/// requests that the function throw an exception if no matching compile-time tuple
/// is found for the runtime inputs.


template<typename Functor, typename TupleList, typename RuntimeTuple, typename... Args, std::enable_if_t<!std::is_same_v<std::decay_t<Functor>, throw_on_no_match_t>, int> = 0>
decltype(auto) dispatch_tuples(Functor &&functor, TupleList /*tuple-list*/, const RuntimeTuple &rt, Args &&...args) {
    using TL = std::decay_t<TupleList>;
    static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

    using first_seq = std::tuple_element_t<0, TL>;
    using result_type = typename detail::result_of_seq_call<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

    bool matched = false;
    detail::result_holder<result_type> out;

    // Expand over all allowed tuple sequences in the TupleList
    std::apply([&](auto... seqs){
        // Use initializer_list trick to evaluate in order and allow early assignment
        (void)std::initializer_list<int>{([&]{
            if (matched) return 0;
            using SeqType = std::decay_t<decltype(seqs)>;
            auto r = detail::seq_matcher_helper<SeqType, result_type, RuntimeTuple, std::decay_t<Functor>, Args...>::match_and_call(rt, std::forward<Functor>(functor), std::forward<Args>(args)...);
            if (r.has_value()) { matched = true; out = std::move(r); }
            return 0;
        }(), 0)...};
    }, TL{});

    if (matched) {
        if constexpr (std::is_void_v<result_type>) return; else return out.get();
    } else {
        if constexpr (!std::is_void_v<result_type>) return result_type{};
    }
}

template<typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
decltype(auto) dispatch_tuples(throw_on_no_match_t, Functor &&functor, TupleList /*tuple-list*/, const RuntimeTuple &rt, Args &&...args) {
    using TL = std::decay_t<TupleList>;
    static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

    using first_seq = std::tuple_element_t<0, TL>;
    using result_type = typename detail::result_of_seq_call<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

    bool matched = false;
    detail::result_holder<result_type> out;

    std::apply([&](auto... seqs){
        (void)std::initializer_list<int>{([&]{
            if (matched) return 0;
            using SeqType = std::decay_t<decltype(seqs)>;
            auto r = detail::seq_matcher_helper<SeqType, result_type, RuntimeTuple, std::decay_t<Functor>, Args...>::match_and_call(rt, std::forward<Functor>(functor), std::forward<Args>(args)...);
            if (r.has_value()) { matched = true; out = std::move(r); }
            return 0;
        }(), 0)...};
    }, TL{});

    if (matched) {
        if constexpr (std::is_void_v<result_type>) return; else return out.get();
    } else {
        throw std::runtime_error("poet::dispatch_tuples: no matching compile-time tuple for runtime inputs");
    }
}

// Convenience wrappers to call `dispatch_tuples` using a `DispatchSet` instance.
template<typename Functor, typename... Tuples, typename... Args>
decltype(auto) dispatch(Functor &&functor, const DispatchSet<Tuples...> &ds, Args &&...args) {
    return dispatch_tuples(std::forward<Functor>(functor), typename DispatchSet<Tuples...>::seq_type{}, ds.runtime_tuple(), std::forward<Args>(args)...);
}

template<typename Functor, typename... Tuples, typename... Args>
decltype(auto) dispatch(throw_on_no_match_t, Functor &&functor, const DispatchSet<Tuples...> &ds, Args &&...args) {
    return dispatch_tuples(throw_on_no_match_t{}, std::forward<Functor>(functor), typename DispatchSet<Tuples...>::seq_type{}, ds.runtime_tuple(), std::forward<Args>(args)...);
}

// Note: nothrow is the default behavior of `dispatch` (no explicit tag).

/// 
/// Overload that accepts `throw_t` as a first argument.
/// When no match exists the function throws `std::runtime_error`.
template<typename Functor, typename ParamTuple, typename... Args, std::enable_if_t<!detail::is_dispatch_set<std::decay_t<ParamTuple>>::value, int> = 0>
decltype(auto) dispatch(throw_on_no_match_t, Functor &&functor, ParamTuple &&params, Args &&...args) {
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
        throw std::runtime_error("poet::dispatch: no matching compile-time combination for runtime inputs");
    }
}

}// namespace poet

#endif// POET_CORE_STATIC_DISPATCH_HPP
