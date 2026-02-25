#pragma once

/// \file dispatch.hpp
/// \brief Runtime-to-compile-time dispatch via function pointer tables.
///
/// Maps runtime integers (or tuples of integers) to compile-time template parameters
/// using compile-time generated function pointer arrays with O(1) runtime indexing.
///
/// Dispatch modes:
/// - **Contiguous ranges**: O(1) arithmetic lookup via `seq_lookup`.
/// - **Sparse sequences**: O(log N) binary search on sorted keys.
/// - **N-D ranges**: Flattened to a single function pointer array with row-major strides.
/// - **DispatchSet**: Explicit tuple matching for non-cartesian combinations.
///
/// Key optimizations:
/// - Stateless functors eliminate the functor pointer from table entries.
/// - Parameter type introspection passes small trivial types by value.

#include <algorithm>
#include <array>
#include <cassert>
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

/// \brief Helper for concise tuple syntax in DispatchSet
///
/// Use `T<1, 2>` to represent a tuple `(1, 2)` inside a `DispatchSet`.
template<auto... Vs> struct T {};

namespace detail {

    template<typename T>
    using result_holder = std::conditional_t<std::is_void_v<T>, std::optional<std::monostate>, std::optional<T>>;

    /// \brief Helper: call functor with the integer pack from an integer_sequence
    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args> struct seq_matcher;

    /// \brief Specialization of seq_matcher for std::integer_sequence.
    /// Matches runtime values against compile-time sequence V... and invokes functor if they match.
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
            // N compile-time values (V...) and N runtime values in runtime_tuple.
            // We expand the fold expression ((std::get<Idx>(runtime_tuple) == V) && ...)
            // to check if *every* runtime value matches its corresponding compile-time value V.
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                // Match found! Invoke the functor with the sequence V... as template arguments.
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

    /// \brief Helper to compute result type for tuple-sequence calls.
    template<typename Seq, typename Functor, typename... Args> struct seq_call_result;

    template<typename ValueType, ValueType... V, typename Functor, typename... Args>
    struct seq_call_result<std::integer_sequence<ValueType, V...>, Functor, Args...> {
        using type = decltype(std::declval<Functor>().template operator()<V...>(std::declval<Args>()...));
    };

    template<int Start, int... Is>
    auto make_range_impl(std::integer_sequence<int, Is...>) -> std::integer_sequence<int, (Start + Is)...>;

}// namespace detail

/// \brief Generates an inclusive integer sequence `[Start, End]`.
template<int Start, int End>
using make_range = decltype(detail::make_range_impl<Start>(std::make_integer_sequence<int, End - Start + 1>{}));

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
    // Trait to detect DispatchParam types
    template<typename T> struct is_dispatch_param : std::false_type {};
    template<typename Seq> struct is_dispatch_param<DispatchParam<Seq>> : std::true_type {};

    template<typename T> inline constexpr bool is_dispatch_param_v = is_dispatch_param<std::decay_t<T>>::value;

    // Trait to detect if a type is a tuple of DispatchParams
    template<typename T> struct is_dispatch_param_tuple : std::false_type {};

    template<typename... Ts>
    struct is_dispatch_param_tuple<std::tuple<Ts...>> : std::bool_constant<(is_dispatch_param_v<Ts> && ...)> {};

    template<typename T>
    inline constexpr bool is_dispatch_param_tuple_v = is_dispatch_param_tuple<std::decay_t<T>>::value;
}// namespace detail

namespace detail {

    // Helper to compute the size of a sequence
    template<typename Sequence> struct sequence_size;

    template<typename T, T... Values>
    struct sequence_size<std::integer_sequence<T, Values...>>
      : std::integral_constant<std::size_t, sizeof...(Values)> {};

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
      : std::bool_constant<!sequence_contains<First, std::integer_sequence<int, Rest...>>::value
                           && sequence_unique<std::integer_sequence<int, Rest...>>::value> {};

    // Compile-time predicate: true when sequence is contiguous (ascending or descending) and unique.
    // A contiguous sequence has max - min + 1 == count.
    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<int First, int... Rest>
    struct is_contiguous_sequence<std::integer_sequence<int, First, Rest...>>
      : std::bool_constant<(std::max({ First, Rest... }) - std::min({ First, Rest... }) + 1
                             == static_cast<int>(1 + sizeof...(Rest)))
                           && sequence_unique<std::integer_sequence<int, First, Rest...>>::value> {};

    /// \brief True when every sequence in a tuple of DispatchParams is contiguous.
    template<typename ParamTuple, typename = std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>>
    struct all_contiguous;

    template<typename ParamTuple, std::size_t... Idx>
    struct all_contiguous<ParamTuple, std::index_sequence<Idx...>>
      : std::bool_constant<(
          is_contiguous_sequence<typename std::tuple_element_t<Idx, std::decay_t<ParamTuple>>::seq_type>::value
          && ...)> {};

    template<typename ParamTuple> inline constexpr bool all_contiguous_v = all_contiguous<ParamTuple>::value;

    template<typename Sequence> struct sequence_first;

    // Compact sparse lookup metadata: sorted (value, first_index) pairs.
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
            // Stable insertion sort by key; duplicates keep source-order (first index first).
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

    /// Sentinel value returned by seq_lookup::find() on miss.
    inline constexpr std::size_t dispatch_npos = static_cast<std::size_t>(-1);

    // ========================================================================
    // seq_lookup: primary template + contiguous specialization
    // ========================================================================

    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct seq_lookup;

    // O(1) arithmetic lookup for contiguous sequences (ascending or descending).
    template<int... Values> struct seq_lookup<std::integer_sequence<int, Values...>, true> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr int first = sequence_first<seq_type>::value;
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

    // ========================================================================
    // Sparse (non-contiguous) lookup: strided O(1) or binary search
    // ========================================================================

    template<int... Values> struct seq_lookup<std::integer_sequence<int, Values...>, false> {
        using seq_type = std::integer_sequence<int, Values...>;
        using sparse_data = sparse_index<seq_type>;

        // True when the sorted unique keys have a constant positive stride.
        // The sorted keys always ascend, so stride = keys[1] - keys[0] > 0.
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
                // O(1): (value - first) / stride is the index into the sorted keys,
                // equivalent to lower_bound on the normalised sequence {0, 1, ..., N-1}.
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

    // Note: Empty sequence (std::integer_sequence<int>) is intentionally not specialized.
    // Attempting to use it will result in a compile-time "incomplete type" error.

    /// \brief Extract dimensions from a tuple of DispatchParam sequences.
    ///
    /// For a tuple of sequences like (make_range<0,3>, make_range<0,4>),
    /// extracts the dimensions [4, 5] (sizes of each sequence).
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

    /// \brief Combined index validation and flattening for N-D dispatch.
    /// Returns the flat index directly, or dispatch_npos on miss.
    /// Avoids std::optional overhead from extract_runtime_indices.
    ///
    /// Uses explicit arrays with pack-expansion in braced initializer lists to
    /// avoid [&] lambda mutable-capture patterns that produce poor GCC codegen.
    /// Step 1: all lookups (no captures, no mutation).
    /// Step 2: miss check via fold-and (short-circuit is implicit in the check,
    ///         but all lookups run unconditionally — acceptable for sparse paths
    ///         because seq_lookup is cheap).
    /// Step 3: flat accumulation via fold-add (only reached on all-hit).
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index_sparse(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/)
      -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        // Step 1: per-dimension mapped indices (pure expansion, no lambdas)
        const std::array<std::size_t, sizeof...(Idx)> indices = {
            seq_lookup<typename std::tuple_element_t<Idx, P>::seq_type>::find(std::get<Idx>(params).runtime_val)...
        };

        // Step 2: miss check — any npos → early return
        const bool all_hit = ((indices[Idx] != dispatch_npos) && ...);
        if (POET_UNLIKELY(!all_hit)) { return dispatch_npos; }

        // Step 3: flat index via fold-add (all hits guaranteed)
        return ((indices[Idx] * strides[Idx]) + ...);
    }

    /// \brief Direction-aware offset for a contiguous sequence.
    /// Ascending: value - first. Descending: first - value.
    template<typename Seq> POET_FORCEINLINE constexpr auto contiguous_offset(int value) noexcept -> std::size_t {
        constexpr auto ufirst = static_cast<unsigned int>(sequence_first<Seq>::value);
        const auto uval = static_cast<unsigned int>(value);
        if constexpr (seq_lookup<Seq>::ascending) {
            return static_cast<std::size_t>(uval - ufirst);
        } else {
            return static_cast<std::size_t>(ufirst - uval);
        }
    }

    /// \brief Fused speculative index computation for all-contiguous N-D dispatch.
    /// Computes all indices unconditionally and checks OOB once at the end,
    /// allowing the CPU to pipeline index computations across dimensions.
    ///
    /// Uses explicit arrays with pack-expansion in braced initializer lists to
    /// avoid lambda/mutable-capture patterns that produce poor GCC codegen.
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index_contiguous(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/)
      -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        // Step 1: per-dimension mapped indices (pure expansion, no lambdas)
        const std::array<std::size_t, sizeof...(Idx)> mapped = {
            contiguous_offset<typename std::tuple_element_t<Idx, P>::seq_type>(std::get<Idx>(params).runtime_val)...
        };

        // Step 2: OOB check via fold-or (no mutable captures)
        const std::size_t oob = (static_cast<std::size_t>(
                                   mapped[Idx] >= sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value)
                                 | ...);

        // Step 3: flat index via fold-add
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

    // Helper: compare two integer_sequences for element-wise equality
    template<typename A, typename B> struct seq_equal;
    template<typename T, T... A, T... B>
    struct seq_equal<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>>
      : std::bool_constant<((A == B) && ...)> {};

    // Helper: check uniqueness among a pack of sequences
    template<typename... S> struct unique_helper;
    template<> struct unique_helper<> : std::true_type {};
    template<typename Head, typename... Rest>
    struct unique_helper<Head, Rest...>
      : std::bool_constant<(!(seq_equal<Head, Rest>::value || ...) && unique_helper<Rest...>::value)> {};

    // Helper trait: detect std::tuple specializations
    template<typename T> struct is_tuple : std::false_type {};
    template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    template<typename S> POET_CPP20_CONSTEVAL auto as_seq_tuple(S /*seq*/) {
        if constexpr (is_tuple<S>::value) {
            return S{};// already a tuple of sequences
        } else {
            return std::tuple<S>{ S{} };// wrap single sequence into a tuple
        }
    }

    template<typename Tuple, std::size_t... Indices>
    POET_CPP20_CONSTEVAL auto extract_sequences_impl(std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        // For each param's seq_type (which may itself be a std::tuple of sequences),
        // produce a tuple and concatenate them into a single flat tuple of sequences.
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

    // ============================================================================
    // Function pointer table-based dispatch (C++17/20)
    // ============================================================================

    template<typename... Args> struct arg_pack {};

    /// \brief True when a functor can be default-constructed inside table entries.
    /// Requires both is_empty (no state) and is_default_constructible (C++20 lambdas).
    template<typename T>
    inline constexpr bool is_stateless_v = std::is_empty_v<T> && std::is_default_constructible_v<T>;

    /// \brief Detects if a functor accepts a type by-value for given template parameters.
    ///
    /// This trait checks whether `Functor::operator()<Vs...>(U)` is callable where U is
    /// the by-value version of T. If the functor accepts by-value, we can safely
    /// optimize the function pointer signature to pass by-value even if the caller
    /// passed a non-const lvalue reference (since by-value proves no output semantics).
    ///
    /// Covers both 1D dispatch (single V) and N-D dispatch (pack Vs...).
    ///
    /// \tparam Functor The functor type to introspect
    /// \tparam T The caller's argument type (possibly a reference)
    /// \tparam Vs The template parameter values (one per dimension)
    template<typename Functor, typename T, int... Vs> struct functor_accepts_by_value {
        using raw = std::remove_cv_t<std::remove_reference_t<T>>;

        // Only consider small trivially copyable types as optimization candidates
        static constexpr bool is_candidate = std::is_trivially_copyable_v<raw> && sizeof(raw) <= 2 * sizeof(void *);

        // Test template form: Functor::template operator()<Vs...>(U)
        template<typename F,
          typename U,
          typename = decltype(std::declval<F>().template operator()<Vs...>(std::declval<U>()))>
        static auto test_template(int) -> std::true_type;

        template<typename F, typename U> static auto test_template(...) -> std::false_type;

        // Test value form: Functor::operator()(integral_constant<int, Vs>..., U)
        template<typename F,
          typename U,
          typename = decltype(std::declval<F>()(std::integral_constant<int, Vs>{}..., std::declval<U>()))>
        static auto test_value(int) -> std::true_type;

        template<typename F, typename U> static auto test_value(...) -> std::false_type;

        static constexpr bool value =
          is_candidate
          && (decltype(test_template<Functor, raw>(0))::value || decltype(test_value<Functor, raw>(0))::value);
    };

    /// \brief Optimizes argument passing for dispatch function pointer tables.
    /// Small trivially-copyable types are passed by value to keep them in
    /// registers instead of spilling to stack. Applies to:
    /// - Rvalue references (safe: caller doesn't observe the move)
    /// - Const lvalue references (safe: const guarantees no output-parameter semantics)
    /// - Non-const lvalue references IF functor accepts by-value (safe: proves no output semantics)
    ///
    /// Covers both 1D dispatch (single V) and N-D dispatch (pack Vs...).
    template<typename T, typename Functor = void, int... Vs> struct arg_pass {
        using raw = std::remove_reference_t<T>;
        using raw_unqual = std::remove_cv_t<raw>;
        static constexpr bool is_small_trivial =
          std::is_trivially_copyable_v<raw_unqual> && (sizeof(raw_unqual) <= 2 * sizeof(void *));

        // Original conditions: caller explicitly allows copying
        static constexpr bool caller_allows_copy =
          std::is_rvalue_reference_v<T> || (std::is_lvalue_reference_v<T> && std::is_const_v<raw>);

        // New condition: functor parameter type introspection
        // If functor accepts by-value, it cannot have output semantics
        static constexpr bool functor_allows_copy =
          !std::is_void_v<Functor> && functor_accepts_by_value<Functor, T, Vs...>::value;

        static constexpr bool by_value = is_small_trivial && (caller_allows_copy || functor_allows_copy);

        using type = std::conditional_t<by_value, raw_unqual, T>;
    };

    template<typename T, typename Functor, int... Vs> using pass_t = typename arg_pass<T, Functor, Vs...>::type;
    template<typename T, typename Functor, int... Vs> using pass_nd_t = typename arg_pass<T, Functor, Vs...>::type;

    /// \brief Helper to detect value-argument form invocability
    template<typename Functor, int Value, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, int Value, typename... Args>
    struct can_use_value_form<Functor, Value, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, std::integral_constant<int, Value>, Args &&...>> {};

    /// \brief Generate function pointer table for contiguous 1D dispatch.
    ///
    /// Creates a compile-time array of function pointers for O(1) dispatch.
    /// Each entry corresponds to one value in the sequence. Works for both
    /// void and non-void return types via `return void_expr;` idiom.
    ///
    /// \tparam Functor User callable type.
    /// \tparam ArgPack arg_pack<Args...> with runtime argument types.
    /// \tparam R Return type (may be void).
    /// \tparam Values Compile-time integer values in the sequence.
    template<typename Functor, typename ArgPack, typename R, int... Values> struct table_builder;

    template<typename Functor, typename... Args, typename R, int... Values>
    struct table_builder<Functor, arg_pack<Args...>, R, Values...> {
        static constexpr int first_value = sequence_first<std::integer_sequence<int, Values...>>::value;

        // Helper to create a single lambda for a specific V value
        template<int V> struct lambda_maker {
            static POET_CPP20_CONSTEVAL auto make_stateless() {
                return +[](pass_t<Args &&, Functor, first_value>... args) -> R {
                    Functor func{};
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, static_cast<Args &&>(args)...);
                    } else {
                        return func.template operator()<V>(static_cast<Args &&>(args)...);
                    }
                };
            }

            static POET_CPP20_CONSTEVAL auto make_stateful() {
                return +[](Functor &func, pass_t<Args &&, Functor, first_value>... args) -> R {
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, static_cast<Args &&>(args)...);
                    } else {
                        return func.template operator()<V>(static_cast<Args &&>(args)...);
                    }
                };
            }
        };

        static POET_CPP20_CONSTEVAL auto make() {
            if constexpr (is_stateless_v<Functor>) {
                using fn_type = decltype(lambda_maker<first_value>::make_stateless());
                return std::array<fn_type, sizeof...(Values)>{ lambda_maker<Values>::make_stateless()... };
            } else {
                using fn_type = decltype(lambda_maker<first_value>::make_stateful());
                return std::array<fn_type, sizeof...(Values)>{ lambda_maker<Values>::make_stateful()... };
            }
        }
    };

    template<typename Functor, typename ArgPack, typename R, int... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<int, Values...> /*seq*/) {
        return table_builder<Functor, ArgPack, R, Values...>::make();
    }

    // ============================================================================
    // Multidimensional dispatch table generation (N-D -> 1D flattening)
    // ============================================================================

    /// \brief Helper to generate all N-dimensional index combinations recursively.
    ///
    /// For dimensions [D0, D1, D2], generates function pointers for all combinations:
    /// (0,0,0), (0,0,1), ..., (D0-1,D1-1,D2-1)
    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq> struct nd_table_builder;

    /// \brief Recursively generate function pointer for each flattened index.
    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct nd_table_builder<Functor, arg_pack<Args...>, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        /// \brief Get the Ith value from a sequence at compile-time.
        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, int... Values> struct get_sequence_value<I, std::integer_sequence<int, Values...>> {
            // Select the I-th value from the integer pack without creating a temporary
            // std::array (which may emit calls in some libstdc++/libc++ builds).
            template<std::size_t J, int First, int... Rest> struct select_impl {
                static constexpr int value = select_impl<J - 1, Rest...>::value;
            };

            template<int First, int... Rest> struct select_impl<0, First, Rest...> {
                static constexpr int value = First;
            };

            static constexpr int value = select_impl<I, Values...>::value;
        };

        /// \brief Compute the index for a specific dimension from a flat index.
        template<std::size_t FlatIdx, std::size_t DimIdx, std::size_t... Dims> struct compute_dim_index {
            static constexpr std::size_t value = []() constexpr -> std::size_t {
                constexpr std::array<std::size_t, sizeof...(Dims)> dimensions = { Dims... };
                constexpr std::array<std::size_t, sizeof...(Dims)> strides = compute_strides(dimensions);
                return FlatIdx / strides[DimIdx] % dimensions[DimIdx];
            }();
        };

        /// \brief Extract the value for a specific dimension at compile-time.
        template<std::size_t FlatIdx, std::size_t DimIdx, std::size_t... Dims>
        static constexpr auto extract_value_for_dim() -> int {
            constexpr std::size_t idx_in_seq = compute_dim_index<FlatIdx, DimIdx, Dims...>::value;
            using Seq = std::tuple_element_t<DimIdx, std::tuple<Seqs...>>;
            return get_sequence_value<idx_in_seq, Seq>::value;
        }

        /// \brief Helper to build integral_constants from extracted values.
        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            static constexpr std::array<int, sizeof...(SeqIdx)> values = {
                extract_value_for_dim<FlatIdx, SeqIdx, sequence_size<Seqs>::value...>()...
            };

            template<std::size_t N> using ic = std::integral_constant<int, values[N]>;
        };

        /// \brief Helper struct to call functor with values computed from flat index.
        template<std::size_t FlatIdx> struct nd_index_caller {
            // Helper to build value_extractor type with proper index sequence
            template<std::size_t... Is>
            static auto make_ve(std::index_sequence<Is...>) -> value_extractor<FlatIdx, Is...>;

            using VE = decltype(make_ve(std::make_index_sequence<sizeof...(Seqs)>{}));

            // Expand index sequence to build parameter types using VE::values
            template<typename T, std::size_t... Is>
            static auto make_opt_type_impl(std::index_sequence<Is...>) -> pass_nd_t<T, Functor, VE::values[Is]...>;

            template<typename T>
            using opt_type = decltype(make_opt_type_impl<T>(std::make_index_sequence<sizeof...(Seqs)>{}));

            template<typename R, typename VE_inner, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto call_value_form(Functor &func, Args &&...args) -> R {
                if constexpr (std::is_void_v<R>) {
                    func(typename VE_inner::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                    return;
                } else {
                    return func(typename VE_inner::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                }
            }

            template<typename R, typename VE_inner, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto call_template_form(Functor &func, Args &&...args) -> R {
                if constexpr (std::is_void_v<R>) {
                    func.template operator()<VE_inner::template ic<SeqIdx>::value...>(std::forward<Args>(args)...);
                    return;
                } else {
                    return func.template operator()<VE_inner::template ic<SeqIdx>::value...>(
                      std::forward<Args>(args)...);
                }
            }

            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto call_impl(Functor &func, Args &&...args) -> R {
                using VE_local = value_extractor<FlatIdx, SeqIdx...>;

                // Detect which form the functor supports: template parameters or integral_constant values
                constexpr bool use_value_form =
                  std::is_invocable_v<Functor &, typename VE_local::template ic<SeqIdx>..., Args &&...>;

                if constexpr (use_value_form) {
                    return call_value_form<R, VE_local, SeqIdx...>(func, std::forward<Args>(args)...);
                } else {
                    return call_template_form<R, VE_local, SeqIdx...>(func, std::forward<Args>(args)...);
                }
            }

            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto
              call_with_indices(Functor &func, std::index_sequence<SeqIdx...> /*indices*/, Args &&...args) -> R {
                return call_impl<R, SeqIdx...>(func, std::forward<Args>(args)...);
            }

            // Helper to build optimized type for a single argument using representative values (FlatIdx=0)
            template<typename T, typename IndexSeq> struct repr_opt_type_helper_impl;

            template<typename T, std::size_t... Is> struct repr_opt_type_helper_impl<T, std::index_sequence<Is...>> {
                // Extract values from FlatIdx=0
                using type = pass_nd_t<T, Functor, extract_value_for_dim<0, Is, sequence_size<Seqs>::value...>()...>;
            };

            template<typename T>
            using repr_opt_type =
              typename repr_opt_type_helper_impl<T, std::make_index_sequence<sizeof...(Seqs)>>::type;

            template<typename R> static POET_FORCEINLINE auto call(Functor &func, repr_opt_type<Args &&>... args) -> R {
                return call_with_indices<R>(
                  func, std::make_index_sequence<sizeof...(Seqs)>{}, static_cast<Args &&>(args)...);
            }

            template<typename R> static POET_FORCEINLINE auto call_stateless(repr_opt_type<Args &&>... args) -> R {
                Functor func{};
                return call_with_indices<R>(
                  func, std::make_index_sequence<sizeof...(Seqs)>{}, static_cast<Args &&>(args)...);
            }
        };

        /// \brief Generate function pointer table for all N-D combinations.
        /// Works for both void and non-void return types.
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

    /// \brief Generate N-D dispatch table for contiguous multidimensional dispatch.
    template<typename Functor, typename ArgPack, typename R, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table(std::tuple<Seqs...> /*seqs*/) {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return nd_table_builder<Functor, ArgPack, std::tuple<Seqs...>, std::make_index_sequence<total_size>>::
          template make_table<R>();
    }

}// namespace detail

/// \brief Represents a discrete set of allowed compile-time tuples.
///
/// Defines a strict set of allowed parameter combinations for dispatch.
/// Unlike cartesian product dispatching, `DispatchSet` allows specifying exact
/// combinations (e.g., (1,2) and (3,4) allowed, but (1,4) disallowed).
///
/// \tparam ValueType The shared type of values in the tuples (e.g., int).
/// \tparam Tuples... A pack of `poet::T<...>` types defining valid tuples.
template<typename ValueType, typename... Tuples> struct DispatchSet {
    // Helper to convert T<Vs...> to std::integer_sequence<ValueType, Vs...>
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<T<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;
    using first_t = std::tuple_element_t<0, seq_type>;

    static_assert(sizeof...(Tuples) >= 1, "DispatchSet requires at least one allowed tuple");

    // Ensure all tuples have the same arity (reuses detail::sequence_size)
    static constexpr std::size_t tuple_arity = detail::sequence_size<first_t>::value;

    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in DispatchSet must have the same arity");

    // Ensure no duplicate tuples (compile-time equality)
    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "DispatchSet contains duplicate allowed tuples");

    // runtime stored tuple to compare against the allowed tuples (array sized by arity)
    using runtime_array_t = std::array<ValueType, tuple_arity>;

  private:
    runtime_array_t runtime_val;

  public:
    // Construct from variadic arguments
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit DispatchSet(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    // Produce a runtime std::tuple<ValueType,...> from the stored array
    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_t{};

namespace detail {
    inline constexpr const char k_no_match_error[] =
      "poet::dispatch: no matching compile-time combination for runtime inputs";

    POET_PUSH_OPTIMIZE

    /// \brief Invoke a single table entry, handling stateless/stateful and void/non-void return.
    template<typename R, typename EntryFn, typename FunctorFwd, typename... Args>
    POET_FORCEINLINE auto invoke_table_entry(FunctorFwd &&functor, EntryFn entry, Args &&...args) -> R {
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

    /// \brief 1D dispatch through a function-pointer table.
    /// O(1) for contiguous sequences, O(log N) for sparse — selected by seq_lookup.
    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_1d(Functor &&functor, ParamTuple const &params, Args &&...args) -> R {
        using FirstParam = std::tuple_element_t<0, std::remove_reference_t<ParamTuple>>;
        using Seq = typename FirstParam::seq_type;
        const int runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = seq_lookup<Seq>::find(runtime_val);

        if (POET_LIKELY(idx != dispatch_npos)) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_dispatch_table<FunctorT, arg_pack<Args...>, R>(Seq{});
            return invoke_table_entry<R>(std::forward<Functor>(functor), table[idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw std::runtime_error(k_no_match_error);
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    /// \brief N-D dispatch through a flattened function-pointer table.
    /// Uses fused speculative indexing for all-contiguous dimensions,
    /// short-circuit validation for mixed contiguous/sparse.
    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_nd(Functor &&functor, ParamTuple const &params, Args &&...args) -> R {
        constexpr auto dimensions = dimensions_of<ParamTuple>();
        constexpr std::size_t table_size = compute_total_size(dimensions);

        const std::size_t flat_idx = extract_flat_index(params);
        if (POET_LIKELY(flat_idx != dispatch_npos)) {
            POET_ASSUME(flat_idx < table_size);

            using sequences_t = decltype(extract_sequences<ParamTuple>());
            static constexpr sequences_t sequences{};

            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(sequences);
            return invoke_table_entry<R>(std::forward<Functor>(functor), table[flat_idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw std::runtime_error(k_no_match_error);
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    /// \brief Internal implementation for dispatch with compile-time error handling policy.
    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_impl(Functor &&functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
        constexpr std::size_t param_count = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        using sequences_t = decltype(extract_sequences<ParamTuple>());
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(
              std::forward<Functor>(functor), params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(
              std::forward<Functor>(functor), params, std::forward<Args>(args)...);
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// DispatchParam-based API is variadic:
//   dispatch(func, DispatchParam<Seq1>{v1}, DispatchParam<Seq2>{v2}, args...)

namespace detail {
    // Helper to count leading DispatchParam arguments
    template<typename... Ts> struct leading_param_count;

    template<> struct leading_param_count<> {
        static constexpr std::size_t value = 0;
    };

    template<typename First, typename... Rest> struct leading_param_count<First, Rest...> {
        static constexpr std::size_t value = is_dispatch_param_v<First> ? (1 + leading_param_count<Rest...>::value) : 0;
    };

    // Split a variadic call into leading DispatchParams and trailing runtime args.
    template<bool ThrowOnNoMatch, typename Functor, std::size_t... ParamIdx, std::size_t... ArgIdx, typename... All>
    POET_FORCEINLINE auto dispatch_split_impl(Functor &&functor,
      std::index_sequence<ParamIdx...> /*p*/,
      std::index_sequence<ArgIdx...> /*a*/,
      All &&...all) -> decltype(auto) {

        constexpr std::size_t num_params = sizeof...(ParamIdx);
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        // Extract DispatchParams into dedicated tuple (copy — DispatchParam is trivially copyable).
        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        // Forward remaining runtime arguments preserving value category.
        // std::move(all_refs) in a pack expansion only casts to rvalue ref; each get() accesses a disjoint index.
        return dispatch_impl<ThrowOnNoMatch>(std::forward<Functor>(functor),
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor &&functor, FirstParam &&first_param, Rest &&...rest)
      -> decltype(auto) {
        // Count leading DispatchParams to split params from runtime arguments.
        constexpr std::size_t num_params = 1 + leading_param_count<Rest...>::value;
        constexpr std::size_t num_args = sizeof...(Rest) + 1 - num_params;

        if constexpr (num_args == 0) {
            auto params = std::make_tuple(std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
            return dispatch_impl<ThrowOnNoMatch>(std::forward<Functor>(functor), params);
        } else {
            return dispatch_split_impl<ThrowOnNoMatch>(std::forward<Functor>(functor),
              std::make_index_sequence<num_params>{},
              std::make_index_sequence<num_args>{},
              std::forward<FirstParam>(first_param),
              std::forward<Rest>(rest)...);
        }
    }
}// namespace detail

/// \brief Dispatches runtime integers to compile-time template parameters.
///
/// The functor is invoked with the template parameter combination whose
/// values match the supplied runtime inputs. When no match exists, the functor
/// is never invoked and a default-constructed result is returned for
/// non-void functors.
///
/// This implementation uses array-based lookup tables for O(1) contiguous
/// dispatch and O(log N) sparse lookup.
///
/// Example usage:
///   // 1D dispatch
///   dispatch(func, DispatchParam<make_range<0, 7>>{runtime_val});
///
///   // 1D dispatch with arguments
///   dispatch(func, DispatchParam<make_range<0, 7>>{runtime_val}, arg1, arg2);
///
///   // 2D dispatch
///   dispatch(func,
///     DispatchParam<make_range<0, 3>>{x},
///     DispatchParam<make_range<0, 3>>{y});
///
/// \param functor Callable exposing `template <int...>` call operators or accepting
///                std::integral_constant parameters.
/// \param first_param First DispatchParam (required to distinguish this overload).
/// \param rest Remaining DispatchParams followed by runtime arguments.
/// \return The functor's result when non-void; otherwise `void`.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(Functor &&functor, FirstParam &&first_param, Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<false>(
      std::forward<Functor>(functor), std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Dispatch using a tuple of DispatchParams.
///
/// This overload accepts a std::tuple of DispatchParam objects directly,
/// which is convenient when the parameters are built dynamically.
///
/// Example:
///   auto params = std::make_tuple(
///     DispatchParam<make_range<0, 3>>{x},
///     DispatchParam<make_range<0, 3>>{y});
///   dispatch(functor, params, runtime_args...);
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(Functor &&functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<false>(std::forward<Functor>(functor), params, std::forward<Args>(args)...);
}

namespace detail {
    /// \brief Internal implementation for dispatch_tuples with compile-time error handling policy.
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

        // Expand over all allowed tuple sequences using || fold for true short-circuiting.
        const bool matched = std::apply(
          [&](auto... seqs) -> bool {
              return ([&](auto &seq) -> bool {
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
            throw std::runtime_error("poet::dispatch_tuples: no matching compile-time tuple for runtime inputs");
        } else if constexpr (!std::is_void_v<result_type>) {
            return result_type{};
        }
    }
}// namespace detail

/// \brief Dispatches using a specific `DispatchSet`.
///
/// Routes the call to the internal tuple dispatch using the set's allowed combinations.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(Functor &&functor, const DispatchSet<Tuples...> &dispatch_set, Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(std::forward<Functor>(functor),
      typename DispatchSet<Tuples...>::seq_type{},
      dispatch_set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `DispatchSet` dispatch.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,
  const DispatchSet<Tuples...> &dispatch_set,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(std::forward<Functor>(functor),
      typename DispatchSet<Tuples...>::seq_type{},
      dispatch_set.runtime_tuple(),
      std::forward<Args>(args)...);
}

// Note: nothrow is the default behavior of `dispatch` (no explicit tag).

/// \brief Dispatch with exception on no match.
///
/// This overload throws `std::runtime_error` when no matching dispatch is found.
/// Uses the same dispatch path as the non-throwing version.
///
/// Example:
///   dispatch(throw_t, func, DispatchParam<Seq1>{v1}, DispatchParam<Seq2>{v2}, arg1, arg2)
///
/// \param functor Callable exposing `template <int...>` call operators.
/// \param first_param First DispatchParam.
/// \param rest Remaining DispatchParams followed by runtime arguments.
/// \return The functor's result.
/// \throws std::runtime_error if no matching dispatch is found.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/, Functor &&functor, FirstParam &&first_param, Rest &&...rest)
  -> decltype(auto) {
    return detail::dispatch_variadic_impl<true>(
      std::forward<Functor>(functor), std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Dispatch using a tuple of DispatchParams with exception on no match.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/, Functor &&functor, ParamTuple const &params, Args &&...args)
  -> decltype(auto) {
    return detail::dispatch_impl<true>(std::forward<Functor>(functor), params, std::forward<Args>(args)...);
}

}// namespace poet
