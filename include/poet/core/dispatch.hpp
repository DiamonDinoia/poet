#pragma once

/// \file dispatch.hpp
/// \brief Runtime-to-compile-time dispatch for integer choices and tuples.

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <poet/core/macros.hpp>
#include <poet/core/mdspan_utils.hpp>

namespace poet {

/// \brief Concise tuple syntax for `dispatch_set`.
template<auto... Vs> struct tuple_ {};

namespace detail {

    template<typename T>
    using result_holder = std::conditional_t<std::is_void_v<T>, std::optional<std::monostate>, std::optional<T>>;

    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args> struct seq_matcher;

    template<typename ValueType,
      ValueType... V,
      typename ResultType,
      typename RuntimeTuple,
      typename Functor,
      typename... Args>
    struct seq_matcher<std::integer_sequence<ValueType, V...>, ResultType, RuntimeTuple, Functor, Args...> {
        template<std::size_t... Idx, typename F>
        static auto
          impl(std::index_sequence<Idx...> /*idx_seq*/, const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
            -> result_holder<ResultType> {
            result_holder<ResultType> res;
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                if constexpr (std::is_void_v<ResultType>) {
                    std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                    res = std::monostate{};
                } else {
                    res = std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                }
            }
            return res;
        }

        template<typename F>
        static auto match_and_call(const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
          -> result_holder<ResultType> {
            return impl(std::make_index_sequence<sizeof...(V)>{},
              runtime_tuple,
              std::forward<F>(func),
              std::forward<Args>(args)...);
        }
    };

    template<typename Seq, typename Functor, typename... Args> struct seq_call_result;

    template<typename ValueType, ValueType... V, typename Functor, typename... Args>
    struct seq_call_result<std::integer_sequence<ValueType, V...>, Functor, Args...> {
        using type = decltype(std::declval<Functor>().template operator()<V...>(std::declval<Args>()...));
    };

    template<int Start, int... Is>
    auto inclusive_range_impl(std::integer_sequence<int, Is...>) -> std::integer_sequence<int, (Start + Is)...>;

}// namespace detail

/// \brief Inclusive integer sequence `[Start, End]`.
template<int Start, int End>
using inclusive_range =
  decltype(detail::inclusive_range_impl<Start>(std::make_integer_sequence<int, End - Start + 1>{}));

/// \brief Runtime value paired with the compile-time candidates to probe.
template<typename Seq> struct dispatch_param {
    int runtime_val;
    using seq_type = Seq;
};

namespace detail {
    template<typename T> struct is_dispatch_param : std::false_type {};
    template<typename Seq> struct is_dispatch_param<dispatch_param<Seq>> : std::true_type {};

    template<typename T> inline constexpr bool is_dispatch_param_v = is_dispatch_param<std::decay_t<T>>::value;

    template<typename T> struct is_dispatch_param_tuple : std::false_type {};

    template<typename... Ts>
    struct is_dispatch_param_tuple<std::tuple<Ts...>> : std::bool_constant<(is_dispatch_param_v<Ts> && ...)> {};

    template<typename T>
    inline constexpr bool is_dispatch_param_tuple_v = is_dispatch_param_tuple<std::decay_t<T>>::value;
}// namespace detail

namespace detail {

    template<typename Sequence> struct sequence_size;

    template<typename T, T... Values>
    struct sequence_size<std::integer_sequence<T, Values...>>
      : std::integral_constant<std::size_t, sizeof...(Values)> {};

    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<int First, int... Rest>
    struct is_contiguous_sequence<std::integer_sequence<int, First, Rest...>>
      : std::bool_constant<(
          std::max({ First, Rest... }) - std::min({ First, Rest... }) + 1 == static_cast<int>(1 + sizeof...(Rest)))> {};

    template<typename ParamTuple, typename = std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>>
    struct all_contiguous;

    template<typename ParamTuple, std::size_t... Idx>
    struct all_contiguous<ParamTuple, std::index_sequence<Idx...>>
      : std::bool_constant<(
          is_contiguous_sequence<typename std::tuple_element_t<Idx, std::decay_t<ParamTuple>>::seq_type>::value
          && ...)> {};

    template<typename ParamTuple> inline constexpr bool all_contiguous_v = all_contiguous<ParamTuple>::value;

    template<typename Sequence> struct sequence_first;

    template<typename Seq> struct sparse_index;

    template<int... Values> struct sparse_index<std::integer_sequence<int, Values...>> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<int, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        static constexpr sorted_data_t sorted_data = []() constexpr -> sorted_data_t {
            sorted_data_t out{};
            out.sorted_keys = std::array<int, value_count>{ Values... };
            for (std::size_t i = 0; i < value_count; ++i) { out.sorted_indices[i] = i; }
            for (std::size_t i = 1; i < value_count; ++i) {
                const int current_key = out.sorted_keys[i];
                const std::size_t current_index = out.sorted_indices[i];
                std::size_t insert_pos = i;
                while (insert_pos > 0 && out.sorted_keys[insert_pos - 1] > current_key) {
                    out.sorted_keys[insert_pos] = out.sorted_keys[insert_pos - 1];
                    out.sorted_indices[insert_pos] = out.sorted_indices[insert_pos - 1];
                    --insert_pos;
                }
                out.sorted_keys[insert_pos] = current_key;
                out.sorted_indices[insert_pos] = current_index;
            }
            return out;
        }();

        static constexpr std::size_t unique_count = []() constexpr -> std::size_t {
            if constexpr (value_count == 0) { return 0; }
            std::size_t count = 1;
            for (std::size_t i = 1; i < value_count; ++i) {
                if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) { ++count; }
            }
            return count;
        }();

        static constexpr std::array<int, unique_count> keys = []() constexpr -> std::array<int, unique_count> {
            std::array<int, unique_count> out{};
            if constexpr (value_count > 0) {
                std::size_t out_i = 0;
                out[out_i++] = sorted_data.sorted_keys[0];
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        out[out_i++] = sorted_data.sorted_keys[i];
                    }
                }
            }
            return out;
        }();

        static constexpr std::array<std::size_t, unique_count> indices =
          []() constexpr -> std::array<std::size_t, unique_count> {
            std::array<std::size_t, unique_count> out{};
            if constexpr (value_count > 0) {
                std::size_t out_i = 0;
                out[out_i++] = sorted_data.sorted_indices[0];
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        out[out_i++] = sorted_data.sorted_indices[i];
                    }
                }
            }
            return out;
        }();
    };

    inline constexpr std::size_t dispatch_npos = static_cast<std::size_t>(-1);

    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct seq_lookup;

    template<int... Values> struct seq_lookup<std::integer_sequence<int, Values...>, true> {
        static constexpr int first = sequence_first<std::integer_sequence<int, Values...>>::value;
        static constexpr std::size_t len = sizeof...(Values);
        static constexpr bool ascending = (first == std::min({ Values... }));

        static POET_FORCEINLINE auto find(int value) -> std::size_t {
            std::size_t idx = 0;
            if constexpr (ascending) {
                idx = static_cast<std::size_t>(static_cast<unsigned int>(value) - static_cast<unsigned int>(first));
            } else {
                idx = static_cast<std::size_t>(static_cast<unsigned int>(first) - static_cast<unsigned int>(value));
            }
            if (POET_LIKELY(idx < len)) { return idx; }
            return dispatch_npos;
        }
    };

    template<int... Values> struct seq_lookup<std::integer_sequence<int, Values...>, false> {
        using sparse_data = sparse_index<std::integer_sequence<int, Values...>>;

        static constexpr bool is_strided = []() constexpr -> bool {
            if constexpr (sparse_data::unique_count < 2) {
                return false;
            } else {
                constexpr int stride0 = sparse_data::keys[1] - sparse_data::keys[0];
                if constexpr (stride0 <= 0) { return false; }
                // cppcheck-suppress syntaxError
                for (std::size_t i = 2; i < sparse_data::unique_count; ++i) {
                    if (sparse_data::keys[i] - sparse_data::keys[i - 1] != stride0) { return false; }
                }
                return true;
            }
        }();

        static POET_FORCEINLINE auto find(int value) -> std::size_t {
            if constexpr (is_strided) {
                static constexpr int first = sparse_data::keys[0];
                static constexpr int stride = sparse_data::keys[1] - sparse_data::keys[0];
                const int diff = value - first;
                if (diff < 0 || diff % stride != 0) { return dispatch_npos; }
                const auto idx = static_cast<std::size_t>(diff / stride);
                if (POET_LIKELY(idx < sparse_data::unique_count)) { return sparse_data::indices[idx]; }
                return dispatch_npos;
            } else {
                const auto pos = std::lower_bound(sparse_data::keys.begin(), sparse_data::keys.end(), value);
                if (pos != sparse_data::keys.end() && *pos == value) {
                    return sparse_data::indices[static_cast<std::size_t>(pos - sparse_data::keys.begin())];
                }
                return dispatch_npos;
            }
        }
    };

    template<int First, int... Rest>
    struct sequence_first<std::integer_sequence<int, First, Rest...>> : std::integral_constant<int, First> {};

    template<typename ParamTuple, std::size_t... Idx>
    POET_CPP20_CONSTEVAL auto dimensions_of_impl(std::index_sequence<Idx...> /*idxs*/)
      -> std::array<std::size_t, sizeof...(Idx)> {
        using P = std::decay_t<ParamTuple>;
        return std::array<std::size_t, sizeof...(Idx)>{
            sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value...
        };
    }

    template<typename ParamTuple>
    POET_CPP20_CONSTEVAL auto dimensions_of() -> std::array<std::size_t, std::tuple_size_v<std::decay_t<ParamTuple>>> {
        return dimensions_of_impl<ParamTuple>(std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
    }

    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index_sparse(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/)
      -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        const std::array<std::size_t, sizeof...(Idx)> indices = {
            seq_lookup<typename std::tuple_element_t<Idx, P>::seq_type>::find(std::get<Idx>(params).runtime_val)...
        };

        const bool all_hit = ((indices[Idx] != dispatch_npos) && ...);
        if (POET_UNLIKELY(!all_hit)) { return dispatch_npos; }

        return ((indices[Idx] * strides[Idx]) + ...);
    }

    template<typename Seq> POET_FORCEINLINE constexpr auto contiguous_offset(int value) noexcept -> std::size_t {
        constexpr auto ufirst = static_cast<unsigned int>(sequence_first<Seq>::value);
        const auto uval = static_cast<unsigned int>(value);
        if constexpr (seq_lookup<Seq>::ascending) {
            return static_cast<std::size_t>(uval - ufirst);
        } else {
            return static_cast<std::size_t>(ufirst - uval);
        }
    }

    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index_contiguous(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/)
      -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        const std::array<std::size_t, sizeof...(Idx)> mapped = {
            contiguous_offset<typename std::tuple_element_t<Idx, P>::seq_type>(std::get<Idx>(params).runtime_val)...
        };

        const std::size_t oob = (static_cast<std::size_t>(
                                   mapped[Idx] >= sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value)
                                 | ...);

        const std::size_t flat = ((mapped[Idx] * strides[Idx]) + ...);

        return POET_LIKELY(oob == 0) ? flat : dispatch_npos;
    }

    template<typename ParamTuple> POET_FORCEINLINE auto extract_flat_index(const ParamTuple &params) -> std::size_t {
        constexpr std::size_t num_dims = std::tuple_size_v<std::decay_t<ParamTuple>>;
        if constexpr (all_contiguous_v<ParamTuple>) {
            return flat_index_contiguous(params, std::make_index_sequence<num_dims>{});
        } else {
            return flat_index_sparse(params, std::make_index_sequence<num_dims>{});
        }
    }

    template<typename A, typename B> struct seq_equal;
    template<typename T, T... A, T... B>
    struct seq_equal<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>>
      : std::bool_constant<((A == B) && ...)> {};

    template<typename... S> struct unique_helper;
    template<> struct unique_helper<> : std::true_type {};
    template<typename Head, typename... Rest>
    struct unique_helper<Head, Rest...>
      : std::bool_constant<(!(seq_equal<Head, Rest>::value || ...) && unique_helper<Rest...>::value)> {};

    template<typename T> struct is_tuple : std::false_type {};
    template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    template<typename S> POET_CPP20_CONSTEVAL auto as_seq_tuple(S /*seq*/) {
        if constexpr (is_tuple<S>::value) {
            return S{};
        } else {
            return std::tuple<S>{ S{} };
        }
    }

    template<typename Tuple, std::size_t... Indices>
    POET_CPP20_CONSTEVAL auto extract_sequences_impl(std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        return std::tuple_cat(as_seq_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{})...);
    }

    template<typename Tuple> POET_CPP20_CONSTEVAL auto extract_sequences() {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl<TupleType>(std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

    template<typename Functor, typename... Seq> struct dispatch_result_helper {
        // First preference: value-argument form (passes std::integral_constant values as parameters).
        template<typename... Args>
        static auto compute_impl(std::true_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>()(std::integral_constant<int, sequence_first<Seq>::value>{}...,
            std::declval<Args>()...));

        // Fallback: template-parameter form.
        template<typename... Args>
        static auto compute_impl(std::false_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>().template operator()<sequence_first<Seq>::value...>(
            std::declval<Args>()...));

        // Detection of value-argument viability using std::is_invocable
        template<typename... Args>
        static auto compute() -> decltype(compute_impl<Args...>(std::integral_constant<bool,
          std::is_invocable_v<Functor &, std::integral_constant<int, sequence_first<Seq>::value>..., Args...>>{})) {
            return compute_impl<Args...>(std::integral_constant<bool,
              std::is_invocable_v<Functor &, std::integral_constant<int, sequence_first<Seq>::value>..., Args...>>{});
        }
    };

    template<typename Functor, typename SequenceTuple, typename... Args> struct dispatch_result;

    template<typename Functor, typename... Seq, typename... Args>
    struct dispatch_result<Functor, std::tuple<Seq...>, Args...> {
        using type = decltype(dispatch_result_helper<Functor, Seq...>::template compute<Args...>());
    };

    template<typename Functor, typename SequenceTuple, typename... Args>
    using dispatch_result_t = typename dispatch_result<Functor, SequenceTuple, Args...>::type;

    template<typename... Args> struct arg_pack {};

    template<typename T>
    inline constexpr bool is_stateless_v = std::is_empty_v<T> && std::is_default_constructible_v<T>;

    template<typename T> struct arg_pass {
        using raw = std::remove_reference_t<T>;
        using raw_unqual = std::remove_cv_t<raw>;
        static constexpr bool is_small_trivial =
          std::is_trivially_copyable_v<raw_unqual> && (sizeof(raw_unqual) <= 2 * sizeof(void *));

        static constexpr bool caller_allows_copy =
          std::is_rvalue_reference_v<T> || (std::is_lvalue_reference_v<T> && std::is_const_v<raw>);

        static constexpr bool by_value = is_small_trivial && caller_allows_copy;

        using type = std::conditional_t<by_value, raw_unqual, T>;
    };

    template<typename T> using pass_t = typename arg_pass<T>::type;

    template<typename Functor, int Value, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, int Value, typename... Args>
    struct can_use_value_form<Functor, Value, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, std::integral_constant<int, Value>, Args &&...>> {};

    template<typename Functor, typename ArgPack, typename R, int... Values> struct table_builder;

    template<typename Functor, typename... Args, typename R, int... Values>
    struct table_builder<Functor, arg_pack<Args...>, R, Values...> {
        static constexpr int first_value = sequence_first<std::integer_sequence<int, Values...>>::value;

        template<int V> static POET_CPP20_CONSTEVAL auto make_entry() {
            if constexpr (is_stateless_v<Functor>) {
                return +[](pass_t<Args &&>... args) -> R {
                    Functor func{};
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, std::forward<Args>(args)...);
                    } else {
                        return func.template operator()<V>(std::forward<Args>(args)...);
                    }
                };
            } else {
                return +[](Functor &func, pass_t<Args &&>... args) -> R {
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, std::forward<Args>(args)...);
                    } else {
                        return func.template operator()<V>(std::forward<Args>(args)...);
                    }
                };
            }
        }

        static POET_CPP20_CONSTEVAL auto make() {
            using fn_type = decltype(make_entry<first_value>());
            return std::array<fn_type, sizeof...(Values)>{ make_entry<Values>()... };
        }
    };

    template<typename Functor, typename ArgPack, typename R, int... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<int, Values...> /*seq*/) {
        return table_builder<Functor, ArgPack, R, Values...>::make();
    }

    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq> struct nd_table_builder;

    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct nd_table_builder<Functor, arg_pack<Args...>, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        static constexpr std::array<std::size_t, sizeof...(Seqs)> dims_ = { sequence_size<Seqs>::value... };
        static constexpr std::array<std::size_t, sizeof...(Seqs)> strides_ = compute_strides(dims_);

        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, int... Values> struct get_sequence_value<I, std::integer_sequence<int, Values...>> {
            static constexpr std::array<int, sizeof...(Values)> values = { Values... };
            static constexpr int value = values[I];
        };

        template<std::size_t FlatIdx, std::size_t DimIdx>
        static constexpr std::size_t dim_index_v = FlatIdx / strides_[DimIdx] % dims_[DimIdx];

        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            static constexpr std::array<int, sizeof...(SeqIdx)> values = {
                get_sequence_value<dim_index_v<FlatIdx, SeqIdx>,
                  std::tuple_element_t<SeqIdx, std::tuple<Seqs...>>>::value...
            };

            template<std::size_t N> using ic = std::integral_constant<int, values[N]>;
        };

        template<std::size_t FlatIdx> struct nd_index_caller {
            template<std::size_t... Is>
            static auto make_ve(std::index_sequence<Is...>) -> value_extractor<FlatIdx, Is...>;
            using VE = decltype(make_ve(std::make_index_sequence<sizeof...(Seqs)>{}));

            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto invoke(Functor &func, std::index_sequence<SeqIdx...> /*idx*/, Args &&...args)
              -> R {
                using VE_local = value_extractor<FlatIdx, SeqIdx...>;
                constexpr bool use_value_form =
                  std::is_invocable_v<Functor &, typename VE_local::template ic<SeqIdx>..., Args &&...>;
                if constexpr (use_value_form) {
                    return func(typename VE_local::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                } else {
                    return func.template operator()<VE_local::template ic<SeqIdx>::value...>(
                      std::forward<Args>(args)...);
                }
            }

            template<typename R> static POET_FORCEINLINE auto call(Functor &func, pass_t<Args &&>... args) -> R {
                return invoke<R>(func, std::make_index_sequence<sizeof...(Seqs)>{}, std::forward<Args>(args)...);
            }

            template<typename R> static POET_FORCEINLINE auto call_stateless(pass_t<Args &&>... args) -> R {
                Functor func{};
                return invoke<R>(func, std::make_index_sequence<sizeof...(Seqs)>{}, std::forward<Args>(args)...);
            }
        };

        template<typename R> static constexpr auto make_table() {
            if constexpr (is_stateless_v<Functor>) {
                using fn_type = decltype(&nd_index_caller<0>::template call_stateless<R>);
                return std::array<fn_type, sizeof...(FlatIndices)>{
                    &nd_index_caller<FlatIndices>::template call_stateless<R>...
                };
            } else {
                using fn_type = decltype(&nd_index_caller<0>::template call<R>);
                return std::array<fn_type, sizeof...(FlatIndices)>{
                    &nd_index_caller<FlatIndices>::template call<R>...
                };
            }
        }
    };

    template<typename Functor, typename ArgPack, typename R, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table(std::tuple<Seqs...> /*seqs*/) {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return nd_table_builder<Functor, ArgPack, std::tuple<Seqs...>, std::make_index_sequence<total_size>>::
          template make_table<R>();
    }

}// namespace detail

/// \brief Exact set of allowed tuples for sparse dispatch.
template<typename ValueType, typename... Tuples> struct dispatch_set {
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<tuple_<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;
    using first_t = std::tuple_element_t<0, seq_type>;

    static_assert(sizeof...(Tuples) >= 1, "dispatch_set requires at least one allowed tuple");

    static constexpr std::size_t tuple_arity = detail::sequence_size<first_t>::value;

    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in dispatch_set must have the same arity");

    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "dispatch_set contains duplicate allowed tuples");

    using runtime_array_t = std::array<ValueType, tuple_arity>;

  private:
    runtime_array_t runtime_val;

  public:
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit dispatch_set(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_on_no_match{};

/// \brief Thrown when a `throw_on_no_match` dispatch has no matching specialization.
struct no_match_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

    POET_PUSH_OPTIMIZE

    template<typename R, typename EntryFn, typename FunctorFwd, typename... Args>
    POET_FORCEINLINE auto invoke_table_entry(FunctorFwd &functor, EntryFn entry, Args &&...args) -> R {
        using FT = std::decay_t<FunctorFwd>;
        if constexpr (is_stateless_v<FT>) {
            if constexpr (std::is_void_v<R>) {
                entry(std::forward<Args>(args)...);
                return;
            } else {
                return entry(std::forward<Args>(args)...);
            }
        } else {
            if constexpr (std::is_void_v<R>) {
                entry(static_cast<FT &>(functor), std::forward<Args>(args)...);
                return;
            } else {
                return entry(static_cast<FT &>(functor), std::forward<Args>(args)...);
            }
        }
    }

    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_1d(Functor &functor, ParamTuple const &params, Args &&...args) -> R {
        using FirstParam = std::tuple_element_t<0, std::remove_reference_t<ParamTuple>>;
        using Seq = typename FirstParam::seq_type;
        const int runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = seq_lookup<Seq>::find(runtime_val);

        if (POET_LIKELY(idx != dispatch_npos)) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_dispatch_table<FunctorT, arg_pack<Args...>, R>(Seq{});
            return invoke_table_entry<R>(functor, table[idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time combination for runtime inputs");
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_nd(Functor &functor, ParamTuple const &params, Args &&...args) -> R {
        constexpr auto dimensions = dimensions_of<ParamTuple>();
        constexpr std::size_t table_size = compute_total_size(dimensions);

        const std::size_t flat_idx = extract_flat_index(params);
        if (POET_LIKELY(flat_idx != dispatch_npos)) {
            POET_ASSUME(flat_idx < table_size);

            using sequences_t = decltype(extract_sequences<ParamTuple>());
            static constexpr sequences_t sequences{};

            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(sequences);
            return invoke_table_entry<R>(functor, table[flat_idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time combination for runtime inputs");
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_impl(Functor &functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
        constexpr std::size_t param_count = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        using sequences_t = decltype(extract_sequences<ParamTuple>());
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

namespace detail {
    template<typename... Ts> struct leading_param_count;

    template<> struct leading_param_count<> {
        static constexpr std::size_t value = 0;
    };

    template<typename First, typename... Rest> struct leading_param_count<First, Rest...> {
        static constexpr std::size_t value = is_dispatch_param_v<First> ? (1 + leading_param_count<Rest...>::value) : 0;
    };

    template<bool ThrowOnNoMatch, typename Functor, std::size_t... ParamIdx, std::size_t... ArgIdx, typename... All>
    POET_FORCEINLINE auto dispatch_split_impl(Functor &functor,
      std::index_sequence<ParamIdx...> /*p*/,
      std::index_sequence<ArgIdx...> /*a*/,
      All &&...all) -> decltype(auto) {

        constexpr std::size_t num_params = sizeof...(ParamIdx);
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        return dispatch_impl<ThrowOnNoMatch>(functor,
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor &functor, FirstParam &&first_param, Rest &&...rest)
      -> decltype(auto) {
        constexpr std::size_t num_params = 1 + leading_param_count<Rest...>::value;
        constexpr std::size_t num_args = sizeof...(Rest) + 1 - num_params;

        if constexpr (num_args == 0) {
            auto params = std::make_tuple(std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
            return dispatch_impl<ThrowOnNoMatch>(functor, params);
        } else {
            return dispatch_split_impl<ThrowOnNoMatch>(functor,
              std::make_index_sequence<num_params>{},
              std::make_index_sequence<num_args>{},
              std::forward<FirstParam>(first_param),
              std::forward<Rest>(rest)...);
        }
    }
}// namespace detail

/// \brief Dispatches runtime integers to compile-time specializations.
///
/// Accepts either leading `dispatch_param` arguments or a tuple of them. On miss,
/// the non-throwing overload returns `void` or a default-constructed result.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid
                                // copy; internally always used by lvalue ref
  FirstParam &&first_param,
  Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<false>(
      functor, std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Tuple overload for `dispatch_param` dispatch.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid
                                // copy; internally always used by lvalue ref
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<false>(functor, params, std::forward<Args>(args)...);
}

namespace detail {
    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor &&functor,
      TupleList const & /*tl*/,
      const RuntimeTuple &runtime_tuple,
      Args &&...args)// NOLINT(cppcoreguidelines-missing-std-forward) forwarded inside short-circuiting fold
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type = typename seq_call_result<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

        result_holder<result_type> out;

        using FunctorT = std::decay_t<Functor>;
        FunctorT functor_copy(std::forward<Functor>(functor));

        const bool matched = std::apply(
          [&](auto... seqs) POET_ALWAYS_INLINE_LAMBDA -> bool {
              return ([&](auto &seq) POET_ALWAYS_INLINE_LAMBDA -> bool {
                  using SeqType = std::decay_t<decltype(seq)>;
                  auto result = seq_matcher<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
                    runtime_tuple, functor_copy, std::forward<Args>(args)...);

                  if (result.has_value()) {
                      out = std::move(result);
                      return true;
                  }
                  return false;
              }(seqs) || ...);
          },
          TL{});

        if (matched) {
            if constexpr (std::is_void_v<result_type>) {
                return;
            } else {
                return result_type(std::move(*out));
            }
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch_tuples: no matching compile-time tuple for runtime inputs");
        } else if constexpr (!std::is_void_v<result_type>) {
            return result_type{};
        }
    }
}// namespace detail

/// \brief Dispatches using a `dispatch_set`.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(Functor &&functor, const dispatch_set<Tuples...> &set, Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(std::forward<Functor>(functor),
      typename dispatch_set<Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `dispatch_set` dispatch.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/, Functor &&functor, const dispatch_set<Tuples...> &set, Args &&...args)
  -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(std::forward<Functor>(functor),
      typename dispatch_set<Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing `dispatch_param` overload.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid copy;
                    // internally always used by lvalue ref
  FirstParam &&first_param,
  Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<true>(
      functor, std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Throwing tuple overload for `dispatch_param` dispatch.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid copy;
                    // internally always used by lvalue ref
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<true>(functor, params, std::forward<Args>(args)...);
}

}// namespace poet
