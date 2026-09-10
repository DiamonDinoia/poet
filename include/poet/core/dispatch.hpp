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

#include <poet/core/macros.hpp>
#include <poet/core/mdspan_utils.hpp>

namespace poet {

/// \brief Concise tuple syntax for `dispatch_set`.
template<auto... Vs> struct tuple_ {};

namespace detail {

    /// Payload for void-returning dispatch, so a match is still an engaged optional.
    struct void_result {};

    template<typename T> using result_holder = std::optional<std::conditional_t<std::is_void_v<T>, void_result, T>>;

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
            // Every runtime slot must equal its compile-time candidate.
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                if constexpr (std::is_void_v<ResultType>) {
                    std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                    res = void_result{};
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

    template<typename V, V Start, V... Is>
    auto inclusive_range_impl(std::integer_sequence<V, Is...>)
      -> std::integer_sequence<V, static_cast<V>(Start + Is)...>;

}// namespace detail

/// \brief Inclusive integer sequence `[Start, End]`, in `Start`'s own value type.
template<auto Start, decltype(Start) End>
using inclusive_range = decltype(detail::inclusive_range_impl<decltype(Start), Start>(
  std::make_integer_sequence<decltype(Start), End - Start + 1>{}));

/// \brief Runtime value paired with the compile-time candidates to probe.
template<typename Seq> struct dispatch_param {
    using seq_type = Seq;
    /// The sequence's own value type, so brace-init rejects a narrowing runtime
    /// value instead of silently truncating.
    using value_type = typename Seq::value_type;
    value_type runtime_val;
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

    template<typename Sequence> struct sequence_first;

    template<typename V, V First, V... Rest>
    struct sequence_first<std::integer_sequence<V, First, Rest...>> : std::integral_constant<V, First> {};

    /// `high == low + 1`, phrased as a guarded difference so an unsigned value type
    /// cannot wrap a descending pair into a spurious unit step.
    template<typename V> constexpr auto steps_up(V low, V high) noexcept -> bool {
        return high > low && high - low == 1;
    }

    /// True when the values form a unit-stride run, ascending or descending.
    ///
    /// `seq_lookup` resolves runs by `position == distance from First`, so a
    /// span equal to the value count is not enough: a permutation such as
    /// `{2, 0, 1}` matches the span but not the positions.
    template<typename V, V... Values> POET_CPP20_CONSTEVAL auto is_unit_stride() noexcept -> bool {
        constexpr std::size_t count = sizeof...(Values);
        if constexpr (count < 2) {
            return true;
        } else {
            constexpr std::array<V, count> values = { Values... };
            constexpr bool ascending = steps_up(values[0], values[1]);
            if constexpr (!ascending && !steps_up(values[1], values[0])) {
                return false;
            } else {
                for (std::size_t i = 2; i < count; ++i) {
                    const bool unit =
                      ascending ? steps_up(values[i - 1], values[i]) : steps_up(values[i], values[i - 1]);
                    if (!unit) { return false; }
                }
                return true;
            }
        }
    }

    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<typename V, V First, V... Rest>
    struct is_contiguous_sequence<std::integer_sequence<V, First, Rest...>>
      : std::bool_constant<is_unit_stride<V, First, Rest...>()> {};

    template<typename Seq> struct sparse_index;

    template<typename V, V... Values> struct sparse_index<std::integer_sequence<V, Values...>> {
        using seq_type = std::integer_sequence<V, Values...>;
        using value_type = V;
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<V, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        // Insertion sort that carries original positions alongside keys, so the dispatch table
        // preserves user-declared slot order while lookups can use ordered search (binary/strided).
        static constexpr sorted_data_t sorted_data = []() constexpr -> sorted_data_t {
            sorted_data_t out{};
            out.sorted_keys = std::array<V, value_count>{ Values... };
            for (std::size_t i = 0; i < value_count; ++i) { out.sorted_indices[i] = i; }
            for (std::size_t i = 1; i < value_count; ++i) {
                const V current_key = out.sorted_keys[i];
                const std::size_t current_index = out.sorted_indices[i];
                std::size_t insert_pos = i;
                // Shift larger keys and their original-position tags right in
                // lockstep until the position of the first key not greater
                // than `current_key`.
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

        static constexpr std::array<V, unique_count> keys = []() constexpr -> std::array<V, unique_count> {
            std::array<V, unique_count> out{};
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

    /// Maps a runtime value to its slot in `Seq`.
    ///
    /// `find` returns a slot in `[0, count)` on a hit and some value `>= count`
    /// on a miss, by contract rather than by a fixed sentinel: the contiguous
    /// finder's raw unsigned difference underflows out of range on its own, so
    /// a hit costs one subtraction and no select. Callers test `idx < count`.
    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct seq_lookup;

    template<typename V, V... Values> struct seq_lookup<std::integer_sequence<V, Values...>, true> {
        static constexpr V first = sequence_first<std::integer_sequence<V, Values...>>::value;
        static constexpr std::size_t len = sizeof...(Values);
        static constexpr bool ascending = (first == std::min({ Values... }));

        static constexpr std::size_t count = len;

        static POET_FORCEINLINE auto find(V value) -> std::size_t {
            // Unsigned subtraction sends "below first" far above `count`, so the
            // caller's `idx < count` test covers underflow and overflow alike.
            // The width must be the value type's own, or a 64-bit miss could alias
            // back into range after a 32-bit truncation.
            using U = std::make_unsigned_t<V>;
            const auto lhs = static_cast<U>(ascending ? value : first);
            const auto rhs = static_cast<U>(ascending ? first : value);
            return static_cast<std::size_t>(static_cast<U>(lhs - rhs));
        }
    };

    // Non-contiguous sequences: detect a uniform positive stride at compile time and
    // specialize `find` to a div/mod (strided) instead of a binary search (truly sparse).
    template<typename V, V... Values> struct seq_lookup<std::integer_sequence<V, Values...>, false> {
        using sparse_data = sparse_index<std::integer_sequence<V, Values...>>;

        static constexpr bool is_strided = []() constexpr -> bool {
            if constexpr (sparse_data::unique_count < 2) {
                return false;
            } else {
                // Reject non-positive strides up front so `find` can use unsigned math.
                // Keys are sorted and unique, so the gap is positive in any value type.
                constexpr V stride0 = static_cast<V>(sparse_data::keys[1] - sparse_data::keys[0]);
                if constexpr (stride0 == 0) { return false; }
                // When any later gap differs from `stride0`, `find` uses binary search.
                // cppcheck-suppress syntaxError ; cppcheck cannot parse a loop inside if constexpr
                for (std::size_t i = 2; i < sparse_data::unique_count; ++i) {
                    if (static_cast<V>(sparse_data::keys[i] - sparse_data::keys[i - 1]) != stride0) { return false; }
                }
                return true;
            }
        }();

        static constexpr std::size_t count = sparse_data::value_count;

        /// `indices` is a permutation of `[0, count)`, but GCC and Clang cannot
        /// see that through the table load and re-check the bound the caller
        /// already applies. Stating the invariant drops the duplicate compare.
        static POET_FORCEINLINE auto bounded(std::size_t slot) -> std::size_t {
            if (slot >= count) { POET_UNREACHABLE(); }
            return slot;
        }

        static POET_FORCEINLINE auto find(V value) -> std::size_t {
            if constexpr (is_strided) {
                using U = std::make_unsigned_t<V>;
                static constexpr V first = sparse_data::keys[0];
                static constexpr V stride = static_cast<V>(sparse_data::keys[1] - sparse_data::keys[0]);
                // Unsigned arithmetic: "below first" wraps past the upper bound,
                // so the two range ends collapse into the single `slot >=` test
                // below. Keys are sorted, so `stride` is positive and the
                // division is a shift.
                const auto diff = static_cast<U>(static_cast<U>(value) - static_cast<U>(first));
                if (diff % static_cast<U>(stride) != 0) { return count; }
                const auto slot = static_cast<std::size_t>(diff / static_cast<U>(stride));
                if (slot >= sparse_data::unique_count) { return count; }
                // Map the sorted position back to the slot the user declared.
                return bounded(sparse_data::indices[slot]);
            } else {
                // Keys are sorted, so a binary search finds `value`; `indices`
                // maps the found position back to the declared slot.
                const auto pos = std::lower_bound(sparse_data::keys.begin(), sparse_data::keys.end(), value);
                if (pos == sparse_data::keys.end() || *pos != value) { return count; }
                return bounded(sparse_data::indices[static_cast<std::size_t>(pos - sparse_data::keys.begin())]);
            }
        }
    };

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

    /// Row-major flat index of the runtime coordinate, or `dispatch_npos` on a miss.
    ///
    /// `seq_lookup::find` already specializes each per-dimension lookup, so one
    /// flattening path serves every sequence shape.
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/) -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        using lookup = std::tuple<seq_lookup<typename std::tuple_element_t<Idx, P>::seq_type>...>;

        const std::array<std::size_t, sizeof...(Idx)> found = { std::tuple_element_t<Idx, lookup>::find(
          std::get<Idx>(params).runtime_val)... };

        // Bitwise-AND fold, not logical: no dimension's range test becomes a
        // branch, and the offset is summed unconditionally alongside the test
        // because a miss discards the sum anyway.
        const unsigned hit = ((static_cast<unsigned>(found[Idx] < std::tuple_element_t<Idx, lookup>::count)) & ...);
        const std::size_t flat = ((found[Idx] * strides[Idx]) + ...);

        return (hit != 0) ? flat : dispatch_npos;
    }

    template<typename ParamTuple> POET_FORCEINLINE auto extract_flat_index(const ParamTuple &params) -> std::size_t {
        return flat_index(params, std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
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

    /// A `dispatch_param` carries either one sequence or (via `dispatch_set`) a
    /// tuple of them; both flatten into one sequence tuple.
    template<typename S> struct as_seq_tuple {
        using type = std::tuple<S>;
    };
    template<typename... Ts> struct as_seq_tuple<std::tuple<Ts...>> {
        using type = std::tuple<Ts...>;
    };

    template<typename Tuple, std::size_t... Indices>
    POET_CPP20_CONSTEVAL auto extract_sequences_impl(std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        return std::tuple_cat(
          typename as_seq_tuple<typename std::tuple_element_t<Indices, TupleType>::seq_type>::type{}...);
    }

    template<typename Tuple> POET_CPP20_CONSTEVAL auto extract_sequences() {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl<TupleType>(std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

    // Computes the functor's return type by probing the two calling conventions:
    // `func(integral_constant<int, V>{}, args...)` (value form) and
    // `func.template operator()<V>(args...)` (template form). The value form is
    // preferred when viable.
    template<typename Functor, typename... Seq> struct dispatch_result_helper {
        template<typename... Args>
        static auto compute_impl(std::true_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>()(sequence_first<Seq>{}..., std::declval<Args>()...));

        template<typename... Args>
        static auto compute_impl(std::false_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>().template operator()<sequence_first<Seq>::value...>(
            std::declval<Args>()...));

        template<typename... Args>
        static auto compute() -> decltype(compute_impl<Args...>(
          std::integral_constant<bool, std::is_invocable_v<Functor &, sequence_first<Seq>..., Args...>>{}));
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

    // Picks the per-arg calling convention in the function-pointer table. A
    // small trivially-copyable rvalue or const-lvalue goes by value, which is
    // cheaper than synthesising a reference. Any other arg keeps its original
    // reference category.
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

    /// `IC` is the `integral_constant` the functor would receive, value type included.
    template<typename Functor, typename IC, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, typename IC, typename... Args>
    struct can_use_value_form<Functor, IC, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, IC, Args &&...>> {};

    template<typename Functor, typename ArgPack, typename R, typename V, V... Values> struct table_builder;

    template<typename Functor, typename... Args, typename R, typename V, V... Values>
    struct table_builder<Functor, arg_pack<Args...>, R, V, Values...> {
        static constexpr V first_value = sequence_first<std::integer_sequence<V, Values...>>::value;

        template<V Value> static POET_FORCEINLINE auto call(Functor &func, pass_t<Args &&>... args) -> R {
            using ic = std::integral_constant<V, Value>;
            if constexpr (can_use_value_form<Functor, ic, arg_pack<Args...>>::value) {
                return func(ic{}, std::forward<Args>(args)...);
            } else {
                return func.template operator()<Value>(std::forward<Args>(args)...);
            }
        }

        // Stateless functors are default-constructed inside the thunk; stateful
        // functors arrive by reference, so every table entry has one signature.
        // Two overloads, not one `if constexpr` with two returns: nvcc reports
        // the `if constexpr` form as a missing return. This note is referenced
        // from nd_table_builder below.
        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry(std::true_type /*stateless*/) {
            return +[](pass_t<Args &&>... args) -> R {
                Functor func{};
                return call<Value>(func, std::forward<Args>(args)...);
            };
        }

        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry(std::false_type /*stateless*/) {
            return +[](Functor &func, pass_t<Args &&>... args) -> R {
                return call<Value>(func, std::forward<Args>(args)...);
            };
        }

        using stateless_tag = std::bool_constant<is_stateless_v<Functor>>;

        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry() { return make_entry<Value>(stateless_tag{}); }

        static POET_CPP20_CONSTEVAL auto make() {
            using fn_type = decltype(make_entry<first_value>());
            return std::array<fn_type, sizeof...(Values)>{ make_entry<Values>()... };
        }
    };

    template<typename Functor, typename ArgPack, typename R, typename V, V... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<V, Values...> /*seq*/) {
        return table_builder<Functor, ArgPack, R, V, Values...>::make();
    }

    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq> struct nd_table_builder;

    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct nd_table_builder<Functor, arg_pack<Args...>, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        static constexpr std::array<std::size_t, sizeof...(Seqs)> dims_ = { sequence_size<Seqs>::value... };
        static constexpr std::array<std::size_t, sizeof...(Seqs)> strides_ = compute_strides(dims_);

        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, typename V, V... Values>
        struct get_sequence_value<I, std::integer_sequence<V, Values...>> {
            static constexpr std::array<V, sizeof...(Values)> values = { Values... };
            static constexpr V value = values[I];
        };

        template<std::size_t FlatIdx, std::size_t DimIdx>
        static constexpr std::size_t dim_index_v = FlatIdx / strides_[DimIdx] % dims_[DimIdx];

        // Exposes each dimension of a flat index as `ic<N>`, which is what the
        // functor receives. Dimensions have distinct value types, so no shared
        // array can hold them.
        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            template<std::size_t N> using seq_at = std::tuple_element_t<N, std::tuple<Seqs...>>;

            template<std::size_t N>
            using ic = std::integral_constant<typename seq_at<N>::value_type,
              get_sequence_value<dim_index_v<FlatIdx, N>, seq_at<N>>::value>;
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

        // Two overloads for the nvcc reason stated at table_builder::make_entry.
        template<typename R> static constexpr auto make_table(std::true_type /*stateless*/) {
            using fn_type = decltype(&nd_index_caller<0>::template call_stateless<R>);
            return std::array<fn_type, sizeof...(FlatIndices)>{
                &nd_index_caller<FlatIndices>::template call_stateless<R>...
            };
        }

        template<typename R> static constexpr auto make_table(std::false_type /*stateless*/) {
            using fn_type = decltype(&nd_index_caller<0>::template call<R>);
            return std::array<fn_type, sizeof...(FlatIndices)>{ &nd_index_caller<FlatIndices>::template call<R>... };
        }

        template<typename R> static constexpr auto make_table() {
            return make_table<R>(std::bool_constant<is_stateless_v<Functor>>{});
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
///
/// Where a tuple of `dispatch_param`s probes the full cartesian product,
/// `dispatch_set` enumerates only the combinations that exist:
///
/// ```cpp
/// using Shapes = poet::dispatch_set<int, poet::tuple_<2, 2>, poet::tuple_<4, 4>>;
/// poet::dispatch(MatMul{}, Shapes{rows, cols}, a, b, c);
/// ```
///
/// \note Unlike the `dispatch_param` overloads, this path calls **only** the
/// template form `functor.template operator()<Values...>(args...)`. A functor
/// taking `std::integral_constant` parameters will not compile here.
///
/// \tparam ValueType Type every tuple element is converted to.
/// \tparam Tuples The allowed combinations, as `tuple_<...>`. All must have the
///   same arity and be distinct.
template<typename ValueType, typename... Tuples> struct dispatch_set {
  private:
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<tuple_<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

  public:
    /// The allowed tuples as `std::integer_sequence`s; what `dispatch` matches against.
    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;

    /// Number of values in every allowed tuple.
    static constexpr std::size_t tuple_arity = detail::sequence_size<std::tuple_element_t<0, seq_type>>::value;

  private:
    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    std::array<ValueType, tuple_arity> runtime_val;

    static_assert(sizeof...(Tuples) >= 1, "dispatch_set requires at least one allowed tuple");

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in dispatch_set must have the same arity");

    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "dispatch_set contains duplicate allowed tuples");

  public:
    /// \brief Binds the runtime values to probe. Requires exactly `tuple_arity` of them.
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit dispatch_set(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_on_no_match{};

/// \brief Thrown when a `throw_on_no_match` dispatch has no matching specialization.
struct no_match_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

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
        const auto runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = seq_lookup<Seq>::find(runtime_val);

        if (idx < seq_lookup<Seq>::count) {
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
        const std::size_t flat_idx = extract_flat_index(params);
        if (POET_LIKELY(flat_idx != dispatch_npos)) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table =
              make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(decltype(extract_sequences<ParamTuple>()){});
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
        static_assert(param_count >= 1, "poet::dispatch requires at least one dispatch_param");
        using sequences_t = decltype(extract_sequences<ParamTuple>());
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        }
    }

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
        // A reference-tuple view over the whole pack permits two indexing passes
        // without copies.
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        // Copy the leading dispatch_params into a value tuple; each holds one
        // runtime int, so the copy is trivial.
        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        // `std::move(all_refs)` moves only the tuple; the references inside keep
        // the remaining entries' value categories.
        return dispatch_impl<ThrowOnNoMatch>(functor,
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    // Splits the pack into leading dispatch_params and trailing args: count
    // dispatch_param types until the first non-param; the rest are plain args.
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
/// Accepts leading `dispatch_param` arguments, followed by any remaining
/// arguments, which are forwarded to `functor` untouched. `functor` is invoked
/// in whichever form it provides, with the value form preferred when both are
/// viable (which is what makes a generic lambda work):
///
/// - `functor(std::integral_constant<V, Value>{}..., args...)`: values
/// - `functor.template operator()<Value...>(args...)`: template parameters
///
/// \warning On a miss this overload is **silent**: it returns a
/// default-constructed result (or nothing, for `void`) and never calls
/// `functor`. Prefix the call with `poet::throw_on_no_match` to get a
/// `no_match_error` instead.
///
/// \param functor The callable to specialize. Bound by reference, so a
///   stateful functor's mutations stay visible to the caller.
/// \param first_param First `dispatch_param`.
/// \param rest Further `dispatch_param`s (consecutive ones form a cartesian
///   product), then the arguments to forward.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
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
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<false>(functor, params, std::forward<Args>(args)...);
}

namespace detail {
    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor &functor,
      TupleList const & /*tl*/,
      const RuntimeTuple &runtime_tuple,
      Args &&...args)// NOLINT(cppcoreguidelines-missing-std-forward) forwarded inside short-circuiting fold
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type = typename seq_call_result<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

        result_holder<result_type> out;

        // By reference, not by copy: a stateful functor's mutations must be
        // visible to the caller, exactly as on the dispatch_param path.
        using FunctorT = std::decay_t<Functor>;
        FunctorT &functor_ref = functor;

        const bool matched = std::apply(
          [&](auto... seqs) POET_ALWAYS_INLINE_LAMBDA -> bool {
              return ([&](auto &seq) POET_ALWAYS_INLINE_LAMBDA -> bool {
                  using SeqType = std::decay_t<decltype(seq)>;
                  auto result = seq_matcher<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
                    runtime_tuple, functor_ref, std::forward<Args>(args)...);

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
template<typename Functor, typename ValueType, typename... Tuples, typename... Args>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
  const dispatch_set<ValueType, Tuples...> &set,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(functor,
      typename dispatch_set<ValueType, Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `dispatch_set` dispatch.
template<typename Functor, typename ValueType, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
  const dispatch_set<ValueType, Tuples...> &set,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(functor,
      typename dispatch_set<ValueType, Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing `dispatch_param` overload.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
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
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<true>(functor, params, std::forward<Args>(args)...);
}

}// namespace poet
