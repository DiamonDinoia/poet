#ifndef POET_CORE_STATIC_DISPATCH_HPP
#define POET_CORE_STATIC_DISPATCH_HPP

/// \file static_dispatch.hpp
/// \brief Provides mechanisms for dispatching runtime values to compile-time template parameters.
///
/// This header implements `dispatch` and `dispatch_tuples`, which allow efficient
/// mapping of runtime integers (or tuples of integers) to compile-time constants.
/// It supports:
/// - Range-based dispatch (e.g., dispatching an integer to `[0..N]`).
/// - Sparse dispatch using `DispatchSet`.
/// - O(1) table-based dispatch for contiguous ranges.
/// - O(log N) key lookup for non-contiguous ranges.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>
#include <poet/core/mdspan_utils.hpp>

namespace poet {

/// \brief Helper for concise tuple syntax in DispatchSet
///
/// Use `T<1, 2>` to represent a tuple `(1, 2)` inside a `DispatchSet`.
template<auto... Vs> struct T {};

/// \brief Helper generating integer sequences shifted by an offset.
///
/// The alias \c make_range builds a sequence of integers in the range
/// `[Start, End]` inclusive. The implementation internally uses
/// `std::integer_sequence` and composes the offset with the supplied range.
namespace detail {

    template<typename T> struct result_holder {
      private:
        std::optional<T> val;

      public:
        template<typename U> void set(U &&value) { val = std::forward<U>(value); }
        auto get() -> T {
            assert(val.has_value());
            return val.value();
        }// NOLINT
        [[nodiscard]] auto has_value() const -> bool { return val.has_value(); }
    };
    template<> struct result_holder<void> {
      private:
        bool set_called = false;

      public:
        void set() { set_called = true; }
        void get() {}
        [[nodiscard]] auto has_value() const -> bool { return set_called; }
    };

    /// \brief Helper: call functor with the integer pack from an integer_sequence
    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args> struct seq_matcher_helper;

    /// \brief Specialization of seq_matcher_helper for std::integer_sequence.
    /// Matches runtime values against compile-time sequence V... and invokes functor if they match.
    template<typename ValueType,
      ValueType... V,
      typename ResultType,
      typename RuntimeTuple,
      typename Functor,
      typename... Args>
    struct seq_matcher_helper<std::integer_sequence<ValueType, V...>, ResultType, RuntimeTuple, Functor, Args...> {
        template<std::size_t... Idx, typename F>
        static auto
          impl(std::index_sequence<Idx...> /*idx_seq*/, const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
            -> result_holder<ResultType> {
            result_holder<ResultType> res;
            if constexpr (sizeof...(V) == 0) {
                // Case: The compile-time integer_sequence is empty.
                // This typically means the user is dispatching to a function with no template parameters,
                // or an empty parameter list. We only match if the runtime tuple provided is also empty.
                if constexpr (sizeof...(Idx) == 0) {
                    // Invoke the functor directly without template arguments.
                    if constexpr (std::is_void_v<ResultType>) {
                        std::forward<F>(func)(std::forward<Args>(args)...);
                        res.set();
                    } else {
                        res.set(std::forward<F>(func)(std::forward<Args>(args)...));
                    }
                }
            } else {
                // Case: N compile-time values (V...) and N runtime values in runtime_tuple.
                // We expand the fold expression ((std::get<Idx>(runtime_tuple) == V) && ...)
                // to check if *every* runtime value matches its corresponding compile-time value V.
                if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                    // Match found! Invoke the functor with the sequence V... as template arguments.
                    if constexpr (std::is_void_v<ResultType>) {
                        std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                        res.set();
                    } else {
                        res.set(std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...));
                    }
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
    template<typename Seq, typename Functor, typename... Args> struct result_of_seq_call;

    // Specialization for empty integer_sequence
    template<typename ValueType, typename Functor, typename... Args>
    struct result_of_seq_call<std::integer_sequence<ValueType>, Functor, Args...> {
        using type = decltype(std::declval<Functor>()(std::declval<Args>()...));
    };

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
    // Trait to detect DispatchParam types
    template<typename T> struct is_dispatch_param : std::false_type {};
    template<typename Seq> struct is_dispatch_param<DispatchParam<Seq>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_dispatch_param_v = is_dispatch_param<std::decay_t<T>>::value;

    // Trait to detect if a type is a tuple of DispatchParams
    template<typename T> struct is_dispatch_param_tuple : std::false_type {};

    template<typename... Ts>
    struct is_dispatch_param_tuple<std::tuple<Ts...>>
        : std::bool_constant<(is_dispatch_param_v<Ts> && ...)> {};

    template<typename T>
    inline constexpr bool is_dispatch_param_tuple_v = is_dispatch_param_tuple<std::decay_t<T>>::value;
}// namespace detail

namespace detail {

    // Helper to extract the minimum value from a sequence
    template<typename Sequence> struct sequence_min;

    template<int First, int... Rest>
    struct sequence_min<std::integer_sequence<int, First, Rest...>> : std::integral_constant<int, First> {};

    // Note: Empty sequence (std::integer_sequence<int>) is intentionally not specialized.
    // Attempting to use it will result in a compile-time "incomplete type" error.

    // Helper to extract the maximum value from a sequence (uses fold expression to avoid O(N) recursion depth)
    template<typename Sequence> struct sequence_max;

    template<int First, int... Rest>
    struct sequence_max<std::integer_sequence<int, First, Rest...>>
      : std::integral_constant<int, std::max({ First, Rest... })> {};

    // Note: Empty sequence (std::integer_sequence<int>) is intentionally not specialized.
    // Attempting to use it will result in a compile-time "incomplete type" error.

    // Helper to compute the size of a sequence
    template<typename Sequence> struct sequence_size;

    template<int... Values>
    struct sequence_size<std::integer_sequence<int, Values...>>
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

    // Compile-time predicate: true when sequence is contiguous ascending and unique
    // Primary template: false for non-sequence types
    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    // Specialization for integer_sequence: checks (max - min + 1 == count) && unique
    template<int... Values>
    struct is_contiguous_sequence<std::integer_sequence<int, Values...>>
      : std::bool_constant<((sequence_max<std::integer_sequence<int, Values...>>::value
                              - sequence_min<std::integer_sequence<int, Values...>>::value + 1)
                             == static_cast<int>(sizeof...(Values)))
                           && sequence_unique<std::integer_sequence<int, Values...>>::value> {};

    template<typename Sequence> struct sequence_first;

    // Compact sparse lookup metadata: sorted (value, first_index) pairs.
    template<typename Seq> struct sparse_sequence_index_data;

    template<int... Values>
    struct sparse_sequence_index_data<std::integer_sequence<int, Values...>> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<int, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        template<std::size_t... I>
        static POET_CPP20_CONSTEVAL auto make_sorted_data_impl(std::index_sequence<I...> /*idxs*/) -> sorted_data_t {
            sorted_data_t out{ { Values... }, { I... } };

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
        }

        static constexpr sorted_data_t sorted_data = make_sorted_data_impl(std::make_index_sequence<value_count>{});

        static POET_CPP20_CONSTEVAL auto compute_unique_count() -> std::size_t {
            if constexpr (value_count == 0) {
                return 0;
            } else {
                std::size_t count = 1;
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        ++count;
                    }
                }
                return count;
            }
        }
        static constexpr std::size_t unique_count = compute_unique_count();

        static POET_CPP20_CONSTEVAL auto make_keys() -> std::array<int, unique_count> {
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
        }
        static constexpr std::array<int, unique_count> keys = make_keys();

        static POET_CPP20_CONSTEVAL auto make_indices() -> std::array<std::size_t, unique_count> {
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
        }
        static constexpr std::array<std::size_t, unique_count> indices = make_indices();
    };

    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value>
    struct sequence_runtime_lookup;

    // O(1) arithmetic lookup for contiguous ascending sequences.
    template<int... Values>
    struct sequence_runtime_lookup<std::integer_sequence<int, Values...>, true> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr int first = sequence_first<seq_type>::value;
        static constexpr std::size_t len = sizeof...(Values);

        static POET_FORCEINLINE auto find(int value) -> std::optional<std::size_t> {
            const int idx = value - first;
            if (POET_LIKELY(static_cast<std::size_t>(idx) < len)) {
                return static_cast<std::size_t>(idx);
            }
            return std::nullopt;
        }
    };

    // O(log N) compact lookup for sparse/non-contiguous sequences.
    template<int... Values>
    struct sequence_runtime_lookup<std::integer_sequence<int, Values...>, false> {
        using seq_type = std::integer_sequence<int, Values...>;
        using sparse_data = sparse_sequence_index_data<seq_type>;

        static POET_FORCEINLINE auto find(int value) -> std::optional<std::size_t> {
            std::size_t left = 0;
            std::size_t right = sparse_data::unique_count;
            while (left < right) {
                const std::size_t mid = left + ((right - left) / 2);
                const int key = sparse_data::keys[mid];
                if (key < value) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }

            if (left < sparse_data::unique_count && sparse_data::keys[left] == value) {
                return sparse_data::indices[left];
            }
            return std::nullopt;
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
    POET_FORCEINLINE POET_CPP20_CONSTEVAL auto extract_dimensions_impl(std::index_sequence<Idx...> /*idxs*/)
        -> std::array<std::size_t, sizeof...(Idx)> {
        using P = std::decay_t<ParamTuple>;
        return std::array<std::size_t, sizeof...(Idx)>{
          sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value...
        };
    }

    template<typename ParamTuple>
    POET_FORCEINLINE POET_CPP20_CONSTEVAL auto extract_dimensions() -> std::array<std::size_t, std::tuple_size_v<std::decay_t<ParamTuple>>> {
        return extract_dimensions_impl<ParamTuple>(
          std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{}
        );
    }

    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto extract_runtime_indices_impl(const ParamTuple& params, std::index_sequence<Idx...> /*idxs*/)
        -> std::optional<std::array<int, sizeof...(Idx)>> {
        using P = std::decay_t<ParamTuple>;
        std::array<int, sizeof...(Idx)> out{};
        bool all_matched = true;

        ([&]() -> void {
            using Seq = typename std::tuple_element_t<Idx, P>::seq_type;
            auto mapped = sequence_runtime_lookup<Seq>::find(std::get<Idx>(params).runtime_val);
            if (mapped.has_value()) {
                out[Idx] = static_cast<int>(*mapped);
            } else {
                all_matched = false;
            }
        }(), ...);

        if (!all_matched) {
            return std::nullopt;
        }
        return out;
    }

    template<typename ParamTuple>
    POET_FORCEINLINE auto extract_runtime_indices(const ParamTuple& params)
        -> std::optional<std::array<int, std::tuple_size_v<std::decay_t<ParamTuple>>>> {
        return extract_runtime_indices_impl(
            params,
            std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{}
        );
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
    auto extract_sequences_impl(const Tuple & /*tuple*/, std::index_sequence<Indices...> /*idxs*/) {
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
        // First preference: value-argument form (passes std::integral_constant values as parameters).
        template<std::size_t... Indices>
        static auto compute_impl(std::true_type /*use_value_args*/, std::index_sequence<Indices...> /*idxs*/)
          -> decltype(std::declval<Functor&>()(
            std::integral_constant<int, sequence_first<Seq>::value>{}...,
            std::get<Indices>(std::declval<ArgumentTuple&&>())...));

        // Fallback: template-parameter form.
        template<std::size_t... Indices>
        static auto compute_impl(std::false_type /*use_value_args*/, std::index_sequence<Indices...> /*idxs*/)
          -> decltype(std::declval<Functor&>().template operator()<sequence_first<Seq>::value...>(
            std::get<Indices>(std::declval<ArgumentTuple&&>())...));

        // Detection of value-argument viability
        template<std::size_t... Indices>
        static auto is_value_args_valid(std::index_sequence<Indices...> /*idxs*/) -> decltype(
          void(std::declval<Functor&>()(
            std::integral_constant<int, sequence_first<Seq>::value>{}...,
            std::get<Indices>(std::declval<ArgumentTuple&&>())...)),
          std::true_type{});
        static auto is_value_args_valid(...) -> std::false_type;

        template<std::size_t... Indices>
        static auto compute(std::index_sequence<Indices...> idxs)
          -> decltype(compute_impl(is_value_args_valid(idxs), idxs)) {
            return compute_impl(is_value_args_valid(idxs), idxs);
        }

        using type = decltype(compute(std::make_index_sequence<std::tuple_size_v<ArgumentTuple>>{}));
    };

    template<typename Functor, typename ArgumentTuple, typename SequenceTuple> struct dispatch_result;

    template<typename Functor, typename ArgumentTuple, typename... Seq>
    struct dispatch_result<Functor, ArgumentTuple, std::tuple<Seq...>> {
        using type = typename dispatch_result_helper<Functor, ArgumentTuple, Seq...>::type;
    };

    template<typename Functor, typename ArgumentTuple, typename SequenceTuple>
    using dispatch_result_t = typename dispatch_result<Functor, ArgumentTuple, SequenceTuple>::type;

    // ============================================================================
    // Function pointer table-based dispatch (C++17/20)
    // ============================================================================

    /// \brief Function pointer type for dispatch with no return value (void)
    template<typename... Args>
    using dispatch_function_ptr_void = void(*)(Args...);

    /// \brief Function pointer type for dispatch with return value
    template<typename R, typename... Args>
    using dispatch_function_ptr = R(*)(Args...);

    /// \brief Helper to detect value-argument form invocability
    template<typename Functor, typename ArgumentTuple, int Value>
    struct can_use_value_form {
        static POET_CPP20_CONSTEVAL auto compute() -> bool {
            if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                return std::is_invocable_v<Functor&, std::integral_constant<int, Value>>;
            } else {
                return std::is_invocable_v<Functor&, std::integral_constant<int, Value>,
                  decltype(std::get<0>(std::declval<ArgumentTuple&&>()))>;
            }
        }

        static constexpr bool value = compute();
    };

    /// \brief Generate function pointer table for contiguous 1D dispatch (void return).
    ///
    /// Creates a compile-time array of function pointers for O(1) dispatch.
    /// Each entry corresponds to one value in the sequence.
    ///
    /// \tparam Functor User callable type.
    /// \tparam Args Runtime argument types.
    /// \tparam Values Compile-time integer values in the sequence.
    template<typename Functor, typename ArgumentTuple, int... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table_void(std::integer_sequence<int, Values...> /*seq*/)
      -> std::array<dispatch_function_ptr_void<Functor*, ArgumentTuple&&>, sizeof...(Values)> {
        // Lambda that will be instantiated for each value
        return { +[](Functor* func, ArgumentTuple&& args) -> void {
            POET_ASSUME_NOT_NULL(func);
            constexpr bool use_value_form = can_use_value_form<Functor, ArgumentTuple, Values>::value;

                if constexpr (use_value_form) {
                if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                    (void)args;  // Suppress unused parameter warning
                    (*func)(std::integral_constant<int, Values>{});
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 1) {
                    (*func)(std::integral_constant<int, Values>{}, std::move(std::get<0>(args)));
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 2) {
                    (*func)(std::integral_constant<int, Values>{},
                           std::move(std::get<0>(args)), std::move(std::get<1>(args)));
                } else {
                    std::apply([func](auto&&... arg) -> void {
                        (*func)(std::integral_constant<int, Values>{}, std::forward<decltype(arg)>(arg)...);
                    }, std::move(args));
                }
            } else {
                // Template-parameter form
                if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                    (void)args;  // Suppress unused parameter warning
                    func->template operator()<Values>();
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 1) {
                    func->template operator()<Values>(std::move(std::get<0>(args)));
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 2) {
                    func->template operator()<Values>(std::move(std::get<0>(args)), std::move(std::get<1>(args)));
                } else {
                    std::apply([func](auto&&... arg) -> void {
                        func->template operator()<Values>(std::forward<decltype(arg)>(arg)...);
                    }, std::move(args));
                }
            }
        }... };
    }

    /// \brief Generate function pointer table for contiguous 1D dispatch (non-void return).
    ///
    /// Creates a compile-time array of function pointers for O(1) dispatch.
    /// Each entry corresponds to one value in the sequence.
    ///
    /// \tparam Functor User callable type.
    /// \tparam Args Runtime argument types.
    /// \tparam R Return type.
    /// \tparam Values Compile-time integer values in the sequence.
    template<typename Functor, typename ArgumentTuple, typename R, int... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<int, Values...> /*seq*/)
      -> std::array<dispatch_function_ptr<R, Functor*, ArgumentTuple&&>, sizeof...(Values)> {
        return { +[](Functor* func, ArgumentTuple&& args) -> R {
            POET_ASSUME_NOT_NULL(func);
            constexpr bool use_value_form = can_use_value_form<Functor, ArgumentTuple, Values>::value;

                if constexpr (use_value_form) {
                if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                    (void)args;  // Suppress unused parameter warning
                    return (*func)(std::integral_constant<int, Values>{});
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 1) {
                    return (*func)(std::integral_constant<int, Values>{}, std::move(std::get<0>(args)));
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 2) {
                    return (*func)(std::integral_constant<int, Values>{},
                      std::move(std::get<0>(args)), std::move(std::get<1>(args)));
                } else {
                    return std::apply([func](auto&&... arg) -> R {
                        return (*func)(std::integral_constant<int, Values>{}, std::forward<decltype(arg)>(arg)...);
                    }, std::move(args));
                }
            } else {
                if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                    (void)args;  // Suppress unused parameter warning
                    return func->template operator()<Values>();
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 1) {
                    return func->template operator()<Values>(std::move(std::get<0>(args)));
                } else if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 2) {
                    return func->template operator()<Values>(
                      std::move(std::get<0>(args)), std::move(std::get<1>(args)));
                } else {
                    return std::apply([func](auto&&... arg) -> R {
                        return func->template operator()<Values>(std::forward<decltype(arg)>(arg)...);
                    }, std::move(args));
                }
            }
        }... };
    }

    // ============================================================================
    // Multidimensional dispatch table generation (N-D -> 1D flattening)
    // ============================================================================

    /// \brief Helper to generate all N-dimensional index combinations recursively.
    ///
    /// For dimensions [D0, D1, D2], generates function pointers for all combinations:
    /// (0,0,0), (0,0,1), ..., (D0-1,D1-1,D2-1)
    template<typename Functor, typename ArgumentTuple, typename SeqTuple, typename IndexSeq>
    struct make_nd_dispatch_table_helper;

    /// \brief Recursively generate function pointer for each flattened index.
    template<typename Functor, typename ArgumentTuple, typename... Seqs, std::size_t... FlatIndices>
    struct make_nd_dispatch_table_helper<Functor, ArgumentTuple, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        /// \brief Get the Ith value from a sequence at compile-time.
        template<std::size_t I, typename Seq>
        struct get_sequence_value;

        template<std::size_t I, int... Values>
            struct get_sequence_value<I, std::integer_sequence<int, Values...>> {
                // Select the I-th value from the integer pack without creating a temporary
                // std::array (which may emit calls in some libstdc++/libc++ builds).
                template<std::size_t J, int First, int... Rest>
                struct select_impl {
                    static constexpr int value = select_impl<J - 1, Rest...>::value;
                };

                template<int First, int... Rest>
                struct select_impl<0, First, Rest...> {
                    static constexpr int value = First;
                };

                static constexpr int value = select_impl<I, Values...>::value;
            };

        /// \brief Compute the index for a specific dimension from a flat index.
        template<std::size_t FlatIdx, std::size_t DimIdx, std::size_t... Dims>
        struct compute_dim_index {
            static POET_CPP20_CONSTEVAL auto compute() -> std::size_t {
                constexpr std::array<std::size_t, sizeof...(Dims)> dimensions = { Dims... };
                constexpr std::array<std::size_t, sizeof...(Dims)> strides = compute_strides(dimensions);
                return FlatIdx / strides[DimIdx] % dimensions[DimIdx];
            }

            static constexpr std::size_t value = compute();
        };

        /// \brief Extract the value for a specific dimension at compile-time.
        template<std::size_t FlatIdx, std::size_t DimIdx, std::size_t... Dims>
        static constexpr auto extract_value_for_dim() -> int {
            constexpr std::size_t idx_in_seq = compute_dim_index<FlatIdx, DimIdx, Dims...>::value;
            using Seq = std::tuple_element_t<DimIdx, std::tuple<Seqs...>>;
            return get_sequence_value<idx_in_seq, Seq>::value;
        }

        /// \brief Helper to build integral_constants from extracted values.
        template<std::size_t FlatIdx, std::size_t... SeqIdx>
        struct value_extractor {
            static constexpr std::array<int, sizeof...(SeqIdx)> values = {
                extract_value_for_dim<FlatIdx, SeqIdx, sequence_size<Seqs>::value...>()...
            };

            template<std::size_t N>
            using ic = std::integral_constant<int, values[N]>;
        };

        /// \brief Helper struct to call functor with values computed from flat index.
        template<std::size_t FlatIdx>
        struct nd_index_caller {
            /// \brief Helper to check if functor accepts integral_constant + runtime args
            template<typename ArgTuple, std::size_t... SeqIdx, std::size_t... ArgIdx>
            static constexpr auto check_invocability_impl(std::index_sequence<ArgIdx...> /*idxs*/) -> bool {
                using VE = value_extractor<FlatIdx, SeqIdx...>;
                return std::is_invocable_v<Functor&, typename VE::template ic<SeqIdx>...,
                                           decltype(std::get<ArgIdx>(std::declval<ArgTuple&&>()))...>;
            }

            template<typename R, typename VE, std::size_t... SeqIdx>
            static auto call_value_form(Functor* func, [[maybe_unused]] ArgumentTuple&& args, [[maybe_unused]] std::index_sequence<SeqIdx...> idxs) -> R {
                POET_ASSUME_NOT_NULL(func);
                if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                    if constexpr (std::is_void_v<R>) {
                        (*func)(typename VE::template ic<SeqIdx>{}...);
                        return;
                    } else {
                        return (*func)(typename VE::template ic<SeqIdx>{}...);
                    }
                } else {
                    if constexpr (std::is_void_v<R>) {
                        std::apply([func](auto&&... arg) -> void {
                            (*func)(typename VE::template ic<SeqIdx>{}..., std::forward<decltype(arg)>(arg)...);
                        }, std::move(args));
                        return;
                    } else {
                        return std::apply([func](auto&&... arg) -> R {
                            return (*func)(typename VE::template ic<SeqIdx>{}..., std::forward<decltype(arg)>(arg)...);
                        }, std::move(args));
                    }
                }
            }

            template<typename R, typename VE, std::size_t... SeqIdx>
            static auto call_template_form(Functor* func, [[maybe_unused]] ArgumentTuple&& args, [[maybe_unused]] std::index_sequence<SeqIdx...> idxs) -> R {
                POET_ASSUME_NOT_NULL(func);
                if constexpr (std::tuple_size_v<std::remove_reference_t<ArgumentTuple>> == 0) {
                    if constexpr (std::is_void_v<R>) {
                        func->template operator()<VE::template ic<SeqIdx>::value...>();
                        return;
                    } else {
                        return func->template operator()<VE::template ic<SeqIdx>::value...>();
                    }
                } else {
                    if constexpr (std::is_void_v<R>) {
                        std::apply([func](auto&&... arg) -> void {
                            func->template operator()<VE::template ic<SeqIdx>::value...>(std::forward<decltype(arg)>(arg)...);
                        }, std::move(args));
                        return;
                    } else {
                        return std::apply([func](auto&&... arg) -> R {
                            return func->template operator()<VE::template ic<SeqIdx>::value...>(std::forward<decltype(arg)>(arg)...);
                        }, std::move(args));
                    }
                }
            }

            template<typename R, std::size_t... SeqIdx>
            static auto call_impl(Functor* func, ArgumentTuple&& args, std::index_sequence<SeqIdx...> /*idxs*/) -> R {
                using VE = value_extractor<FlatIdx, SeqIdx...>;

                // Detect which form the functor supports: template parameters or integral_constant values
                constexpr bool use_value_form = []() POET_CPP20_CONSTEVAL -> bool {
                    using AT = std::remove_reference_t<ArgumentTuple>;
                    if constexpr (std::tuple_size_v<AT> == 0) {
                        // No runtime arguments - check if functor accepts integral_constant values only
                        return std::is_invocable_v<Functor&, typename VE::template ic<SeqIdx>...>;
                    }
                    // With runtime arguments - check if functor accepts integral_constants + runtime args
                    return check_invocability_impl<ArgumentTuple, SeqIdx...>(
                        std::make_index_sequence<std::tuple_size_v<AT>>{}
                    );
                }();

                if constexpr (use_value_form) {
                    return call_value_form<R, VE>(func, std::move(args), std::index_sequence<SeqIdx...>{});
                } else {
                    return call_template_form<R, VE>(func, std::move(args), std::index_sequence<SeqIdx...>{});
                }
            }

            template<typename R>
            static auto call(Functor* func, ArgumentTuple&& args) -> R {
                return call_impl<R>(func, std::move(args), std::make_index_sequence<sizeof...(Seqs)>{});
            }
        };

        /// \brief Generate function pointer table for all N-D combinations (void return).
        static constexpr auto make_table_void()
          -> std::array<dispatch_function_ptr_void<Functor*, ArgumentTuple&&>, sizeof...(FlatIndices)> {

            return { &nd_index_caller<FlatIndices>::template call<void>... };
        }

        /// \brief Generate function pointer table for all N-D combinations (non-void return).
        template<typename R>
        static constexpr auto make_table()
          -> std::array<dispatch_function_ptr<R, Functor*, ArgumentTuple&&>, sizeof...(FlatIndices)> {

            return { &nd_index_caller<FlatIndices>::template call<R>... };
        }
    };

    /// \brief Generate N-D dispatch table for contiguous multidimensional dispatch.
    template<typename Functor, typename ArgumentTuple, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table_void(std::tuple<Seqs...> /*seqs*/)
      -> std::array<dispatch_function_ptr_void<Functor*, ArgumentTuple&&>,
                   (sequence_size<Seqs>::value * ... * 1)> {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return make_nd_dispatch_table_helper<Functor, ArgumentTuple, std::tuple<Seqs...>,
                                            std::make_index_sequence<total_size>>::make_table_void();
    }

    /// \brief Generate N-D dispatch table for contiguous multidimensional dispatch (non-void return).
    template<typename Functor, typename ArgumentTuple, typename R, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table(std::tuple<Seqs...> /*seqs*/)
      -> std::array<dispatch_function_ptr<R, Functor*, ArgumentTuple&&>,
                   (sequence_size<Seqs>::value * ... * 1)> {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return make_nd_dispatch_table_helper<Functor, ArgumentTuple, std::tuple<Seqs...>,
                                            std::make_index_sequence<total_size>>::template make_table<R>();
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

    // Ensure all tuples have the same arity
    template<typename S> struct seq_len;
    template<typename VType, VType... V>
    struct seq_len<std::integer_sequence<VType, V...>> : std::integral_constant<std::size_t, sizeof...(V)> {};

    static constexpr std::size_t tuple_arity = seq_len<first_t>::value;

    template<typename S> struct same_arity : std::bool_constant<seq_len<S>::value == tuple_arity> {};

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

namespace detail {
}// namespace detail

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_t{};

namespace detail {
    inline constexpr const char *k_no_match_error =
      "poet::dispatch: no matching compile-time combination for runtime inputs";

    /// \brief 1D dispatch through function-pointer tables (contiguous and sparse).
    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FLATTEN
    auto dispatch_1d_contiguous(Functor&& functor, ParamTuple const &params, Args &&...args) -> R {
        using FirstParam = std::tuple_element_t<0, std::remove_reference_t<ParamTuple>>;
        using Seq = typename FirstParam::seq_type;
        const int runtime_val = std::get<0>(params).runtime_val;
        auto idx = sequence_runtime_lookup<Seq>::find(runtime_val);

        if (POET_LIKELY(idx.has_value())) {
            using FunctorT = std::decay_t<Functor>;
            auto &&forwarded_functor = std::forward<Functor>(functor);
            FunctorT *functor_ptr = std::addressof(static_cast<FunctorT&>(forwarded_functor));

            using argument_tuple_type = std::tuple<std::decay_t<Args>...>;
            argument_tuple_type argument_tuple(std::forward<Args>(args)...);

            if constexpr (std::is_void_v<R>) {
                static constexpr auto table = make_dispatch_table_void<FunctorT, argument_tuple_type>(Seq{});
                table[*idx](functor_ptr, std::move(argument_tuple));
                return;
            } else {
                static constexpr auto table = make_dispatch_table<FunctorT, argument_tuple_type, R>(Seq{});
                return table[*idx](functor_ptr, std::move(argument_tuple));
            }
        } else {
            if constexpr (ThrowOnNoMatch) {
                throw std::runtime_error(k_no_match_error);
            } else {
                if constexpr (!std::is_void_v<R>) {
                    return R{};
                }
            }
        }
    }

    /// \brief N-D dispatch through flattened function-pointer tables.
    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FLATTEN
    auto dispatch_nd_contiguous(Functor&& functor, ParamTuple const &params, Args &&...args) -> R {
        constexpr auto dimensions = extract_dimensions<ParamTuple>();
        constexpr auto strides = compute_strides(dimensions);
        constexpr std::size_t table_size = compute_total_size(dimensions);

        const auto mapped_indices = extract_runtime_indices(params);
        if (POET_LIKELY(mapped_indices.has_value())) {
            const std::size_t flat_idx = flatten_indices(*mapped_indices, strides);
            POET_ASSUME(flat_idx < table_size);

            using argument_tuple_type = std::tuple<std::decay_t<Args>...>;
            argument_tuple_type argument_tuple(std::forward<Args>(args)...);
            using sequences_t = decltype(extract_sequences(std::declval<ParamTuple>()));
            static constexpr sequences_t sequences{};

            using FunctorT = std::decay_t<Functor>;
            auto &&forwarded_functor = std::forward<Functor>(functor);
            FunctorT *functor_ptr = std::addressof(static_cast<FunctorT&>(forwarded_functor));
            if constexpr (std::is_void_v<R>) {
                static constexpr auto table = make_nd_dispatch_table_void<FunctorT, argument_tuple_type>(sequences);
                table[flat_idx](functor_ptr, std::move(argument_tuple));
                return;
            } else {
                static constexpr auto table = make_nd_dispatch_table<FunctorT, argument_tuple_type, R>(sequences);
                return table[flat_idx](functor_ptr, std::move(argument_tuple));
            }
        } else {
            if constexpr (ThrowOnNoMatch) {
                throw std::runtime_error(k_no_match_error);
            } else {
                if constexpr (!std::is_void_v<R>) {
                    return R{};
                }
            }
        }
    }

    /// \brief Internal implementation for dispatch with compile-time error handling policy.
    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    POET_FLATTEN
    auto dispatch_impl(Functor&& functor, ParamTuple const &params, Args&&... args) -> decltype(auto) {
        constexpr std::size_t param_count = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        using sequences_t = decltype(extract_sequences(std::declval<ParamTuple>()));
        using argument_tuple_type = std::tuple<std::decay_t<Args>...>;
        using result_type = dispatch_result_t<Functor, argument_tuple_type, sequences_t>;

        if constexpr (param_count == 1) {
            return dispatch_1d_contiguous<ThrowOnNoMatch, result_type>(
              std::forward<Functor>(functor), params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd_contiguous<ThrowOnNoMatch, result_type>(
              std::forward<Functor>(functor), params, std::forward<Args>(args)...);
        }
    }
}// namespace detail

// DispatchParam-based API is variadic:
//   dispatch(func, DispatchParam<Seq1>{v1}, DispatchParam<Seq2>{v2}, args...)

namespace detail {
    // Helper to count leading DispatchParam arguments
    template<typename... Ts> struct count_leading_dispatch_params;

    template<> struct count_leading_dispatch_params<> {
        static constexpr std::size_t value = 0;
    };

    template<typename First, typename... Rest>
    struct count_leading_dispatch_params<First, Rest...> {
        static constexpr std::size_t value =
            is_dispatch_param_v<First> ? (1 + count_leading_dispatch_params<Rest...>::value) : 0;
    };

    // Split a variadic call into leading DispatchParams and trailing runtime args.
    template<bool ThrowOnNoMatch, typename Functor, std::size_t... ParamIdx, std::size_t... ArgIdx, typename... All>
    POET_FORCEINLINE auto dispatch_multi_impl(
        Functor&& functor,
        std::index_sequence<ParamIdx...> /*p*/,
        std::index_sequence<ArgIdx...> /*a*/,
        All&&... all) -> decltype(auto) {

        constexpr std::size_t num_params = sizeof...(ParamIdx);
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        // Extract DispatchParams into dedicated tuple.
        auto params = std::make_tuple(std::get<ParamIdx>(std::move(all_refs))...);

        // Forward remaining runtime arguments directly.
        return dispatch_impl<ThrowOnNoMatch>(
            std::forward<Functor>(functor),
            params,
            std::get<num_params + ArgIdx>(std::move(all_refs))...
        );
    }

    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor&& functor, FirstParam&& first_param, Rest&&... rest)
      -> decltype(auto) {
        // Count leading DispatchParams to split params from runtime arguments.
        constexpr std::size_t num_params = 1 + count_leading_dispatch_params<Rest...>::value;
        constexpr std::size_t num_args = sizeof...(Rest) + 1 - num_params;

        if constexpr (num_args == 0) {
            auto params = std::make_tuple(std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
            return dispatch_impl<ThrowOnNoMatch>(std::forward<Functor>(functor), params);
        } else {
            return dispatch_multi_impl<ThrowOnNoMatch>(
              std::forward<Functor>(functor),
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
POET_FLATTEN
auto dispatch(Functor&& functor, FirstParam&& first_param, Rest&&... rest) -> decltype(auto) {
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
POET_FLATTEN
auto dispatch(Functor&& functor, ParamTuple const& params, Args&&... args) -> decltype(auto) {
    return detail::dispatch_impl<false>(std::forward<Functor>(functor), params, std::forward<Args>(args)...);
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

namespace detail {
    /// \brief Internal implementation for dispatch_tuples with compile-time error handling policy.
    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor functor, TupleList const & /*unused*/, const RuntimeTuple &runtime_tuple, Args... args)
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type =
          typename result_of_seq_call<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

        bool matched = false;
        result_holder<result_type> out;

        using FunctorT = std::decay_t<Functor>;
        FunctorT functor_copy(std::forward<Functor>(functor));

        // Expand over all allowed tuple sequences in the TupleList
        std::apply(
          [&](auto... seqs) -> auto {
              // We use a fold-like construct with std::initializer_list to iterate over each allowed sequence `seq`.
              // This ensures sequential evaluation order.
              [[maybe_unused]] auto unused_init = std::initializer_list<int>{ (
                [&](auto &seq) -> auto {
                    // Short-circuit: if we already found a match, stop trying others.
                    if (matched) { return 0; }

                    using SeqType = std::decay_t<decltype(seq)>;
                    // Try to match the current runtime tuple against the compile-time sequence `seq`.
                    // helper::match_and_call will iterate the elements and compare runtime vs compile-time.
                    auto result =
                      seq_matcher_helper<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
                        runtime_tuple, functor_copy, std::move(args)...);

                    // If match succeeded, store the result and mark matched = true.
                    if (result.has_value()) {
                        matched = true;
                        out = std::move(result);
                    }
                    return 0;
                }(seqs),
                0)... };
          },
          TL{});

        if (matched) {
            if constexpr (std::is_void_v<result_type>) {
                return;
            } else {
                return out.get();
            }
        } else {
            if constexpr (ThrowOnNoMatch) {
                throw std::runtime_error("poet::dispatch_tuples: no matching compile-time tuple for runtime inputs");
            } else {
                if constexpr (!std::is_void_v<result_type>) { return result_type{}; }
            }
        }
    }
}// namespace detail

template<typename Functor,
  typename TupleList,
  typename RuntimeTuple,
  typename... Args,
  std::enable_if_t<!std::is_same_v<std::decay_t<Functor>, throw_on_no_match_t>, int> = 0>
auto dispatch_tuples(Functor functor, TupleList const &tuple_list, const RuntimeTuple &runtime_tuple, Args... args)
  -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(
      std::forward<Functor>(functor), tuple_list, runtime_tuple, std::move(args)...);
}

template<typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
auto dispatch_tuples(throw_on_no_match_t /*tag*/,
  Functor functor,
  TupleList const &tuple_list,
  const RuntimeTuple &runtime_tuple,
  Args... args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(
      std::forward<Functor>(functor), tuple_list, runtime_tuple, std::move(args)...);
}

/// \brief Dispatches using a specific `DispatchSet`.
///
/// Routes the call to `dispatch_tuples` using the set's allowed combinations.
template<typename Functor, typename... Tuples, typename... Args>
POET_FLATTEN
auto dispatch(Functor &&functor, const DispatchSet<Tuples...> &dispatch_set, Args &&...args) -> decltype(auto) {
        return dispatch_tuples(std::forward<Functor>(functor),
            typename DispatchSet<Tuples...>::seq_type{},
            dispatch_set.runtime_tuple(),
            std::forward<Args>(args)...);
}

/// \brief Throwing overload for `DispatchSet` dispatch.
template<typename Functor, typename... Tuples, typename... Args>
POET_FLATTEN
auto dispatch(throw_on_no_match_t /*tag*/, Functor functor, const DispatchSet<Tuples...> &dispatch_set, Args... args)
    -> decltype(auto) {
        return dispatch_tuples(throw_on_no_match_t{},
            std::move(functor),
            typename DispatchSet<Tuples...>::seq_type{},
            dispatch_set.runtime_tuple(),
            std::move(args)...);
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
/// \param tag throw_on_no_match_t tag to request exception on mismatch.
/// \param functor Callable exposing `template <int...>` call operators.
/// \param first_param First DispatchParam.
/// \param rest Remaining DispatchParams followed by runtime arguments.
/// \return The functor's result.
/// \throws std::runtime_error if no matching dispatch is found.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
POET_FLATTEN
auto dispatch(throw_on_no_match_t /*tag*/, Functor&& functor, FirstParam&& first_param, Rest&&... rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<true>(
      std::forward<Functor>(functor), std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Dispatch using a tuple of DispatchParams with exception on no match.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
POET_FLATTEN
auto dispatch(throw_on_no_match_t /*tag*/, Functor&& functor, ParamTuple const& params, Args&&... args) -> decltype(auto) {
    return detail::dispatch_impl<true>(std::forward<Functor>(functor), params, std::forward<Args>(args)...);
}

}// namespace poet

#endif// POET_CORE_STATIC_DISPATCH_HPP
