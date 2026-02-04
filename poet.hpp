/* Auto-generated single-header. Do not edit directly. */

#ifndef POET_SINGLE_HEADER_GOLDBOT_HPP
#define POET_SINGLE_HEADER_GOLDBOT_HPP

// BEGIN_FILE: include/poet/poet.hpp

/// \file poet.hpp
/// \brief Umbrella header aggregating the public POET API surface.
///
/// This header centralizes includes for the public headers found under
/// include/poet/ so that downstream projects can simply include <poet/poet.hpp>
/// to access the stable API surface.

// clang-format off
// IMPORTANT: Include order matters! macros.hpp must come first, undef_macros.hpp must come last
#include <poet/core/macros.hpp>
/* Begin inline (angle): include/poet/core/dynamic_for.hpp */
// BEGIN_FILE: include/poet/core/dynamic_for.hpp

/// \file dynamic_for.hpp
/// \brief Provides runtime loop execution with compile-time unrolling.
///
/// This header defines `dynamic_for`, a utility that allows executing a loop
/// where the range is determined at runtime, but the body is unrolled at
/// compile-time. This is achieved by combining `static_for` for the bulk of
/// iterations and a runtime loop (or dispatch) strategy.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>
/* Begin inline (angle): include/poet/core/static_dispatch.hpp */
// BEGIN_FILE: include/poet/core/static_dispatch.hpp

/// \file static_dispatch.hpp
/// \brief Provides mechanisms for dispatching runtime values to compile-time template parameters.
///
/// This header implements `dispatch` and `dispatch_tuples`, which allow efficient
/// mapping of runtime integers (or tuples of integers) to compile-time constants.
/// It supports:
/// - Range-based dispatch (e.g., dispatching an integer to `[0..N]`).
/// - Sparse dispatch using `DispatchSet`.
/// - O(1) table-based dispatch for contiguous ranges.
/// - Fallback strategies for non-contiguous ranges.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <poet/core/macros.hpp>

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

    // Helper to find the index of a value in a sequence at compile-time
    template<int Value, typename Sequence> struct sequence_index_of;

    template<int Value, int First, int... Rest>
    struct sequence_index_of<Value, std::integer_sequence<int, First, Rest...>> {
        static constexpr std::size_t value =
          (Value == First) ? 0 : (1 + sequence_index_of<Value, std::integer_sequence<int, Rest...>>::value);
    };

    template<int Value> struct sequence_index_of<Value, std::integer_sequence<int>> {
        static constexpr std::size_t value = static_cast<std::size_t>(-1);// Not found
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

    // Runtime helper to find index of a value in a sequence
    template<typename Sequence> struct sequence_runtime_index;

    template<int... Values> struct sequence_runtime_index<std::integer_sequence<int, Values...>> {
        static auto find(int value) -> std::optional<std::size_t> {
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

    // Note: Empty sequence (std::integer_sequence<int>) is intentionally not specialized.
    // Attempting to use it will result in a compile-time "incomplete type" error.

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

    template<typename S> constexpr auto as_seq_tuple(S /*seq*/) {
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

    // Variant + visit dispatch (O(1) mapping per-dimension for contiguous ascending integer sequences)
    template<int... I> struct variant_maker {
        using V = std::variant<std::integral_constant<int, I>...>;

        template<std::size_t... Idx>
        static constexpr auto make_table(std::index_sequence<Idx...> /*seq_idx*/) -> std::array<V, sizeof...(I)> {
            return { V(std::in_place_index<Idx>)... };
        }

        static auto make_by_index(std::size_t idx) -> V {
            static constexpr auto tbl = make_table(std::make_index_sequence<sizeof...(I)>{});
            return tbl.at(idx);
        }
    };

    template<typename Seq> struct variant_from_seq;
    template<int... I> struct variant_from_seq<std::integer_sequence<int, I...>> {
        using maker = variant_maker<I...>;
        using type = typename maker::V;
    };
    template<typename Seq> using variant_from_seq_t = typename variant_from_seq<Seq>::type;

    // Helper: compare runtime array against an integer_sequence
    template<int... Vs, std::size_t N>
    auto match_array_to_sequence(const std::array<int, N> &arr, std::integer_sequence<int, Vs...> /*seq*/) -> bool {
        if (N != sizeof...(Vs)) { return false; }
        constexpr std::array<int, sizeof...(Vs)> seq_vals = { Vs... };
        for (std::size_t i = 0; i < N; ++i) {
            if (arr[i] != seq_vals[i]) { return false; }
        }
        return true;
    }

    /// \brief Attempts to map a runtime value to a compile-time sequence variant.
    ///
    /// - For contiguous sequences: Uses O(1) arithmetic offset.
    /// - For non-contiguous sequences: Uses O(N) linear scan.
    template<typename Seq> auto to_variant(int val, Seq /*seq*/) -> std::optional<variant_from_seq_t<Seq>> {
        if constexpr (is_contiguous_sequence<Seq>::value) {
            // Optimization for contiguous ranges (e.g., [0, 1, 2] or [5, 6, 7]).
            constexpr int first = sequence_first<Seq>::value;
            constexpr std::size_t len = sequence_size<Seq>::value;
            const int idx = val - first;

            // Branchless bounds check optimization:
            // Single unsigned comparison handles both negative and out-of-range positive indices.
            // - When idx < 0: Casting to std::size_t wraps around (two's complement) to a
            //   very large positive value (> len), so the check fails as desired.
            // - When idx >= len: The check naturally fails.
            // - When 0 <= idx < len: The check passes.
            // This reduces branch mispredictions and improves pipeline efficiency.
            if (static_cast<std::size_t>(idx) < len) {
                return variant_from_seq<Seq>::maker::make_by_index(static_cast<std::size_t>(idx));
            }
            return std::nullopt;
        } else {
            // Fallback for non-contiguous sequences.
            auto idx = sequence_runtime_index<Seq>::find(val);
            if (!idx) { return std::nullopt; }
            return variant_from_seq<Seq>::maker::make_by_index(*idx);
        }
    }

    template<typename ParamTuple, std::size_t... Idx>
    auto make_variants(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/) {
        using P = std::decay_t<ParamTuple>;
        return std::make_tuple(
          to_variant(std::get<Idx>(params).runtime_val, typename std::tuple_element_t<Idx, P>::seq_type{})...);
    }

    template<typename Functor, typename ArgumentTuple> struct VisitCaller {
      private:
        Functor *func;
        ArgumentTuple args;
        // Prefer value-argument form when viable; otherwise use template-parameter form.
        template<typename F, typename AT, typename... IC> struct has_value_call {
            template<std::size_t... Is>
            static auto test(std::index_sequence<Is...> /*idxs*/) -> decltype(
              std::declval<F&>()(IC{}..., std::get<Is>(std::declval<AT&&>())...), std::true_type{});
            static auto test(...) -> std::false_type;
            static constexpr bool value = decltype(test(
              std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<AT>>>{}))::value;
        };

      public:
        VisitCaller(Functor *functor, ArgumentTuple &&arguments) : func(functor), args(std::move(arguments)) {}

        /// \brief Invokes the stored functor with compile-time dispatch parameters.
        ///
        /// This operator is the core of the dispatch mechanism. It receives compile-time
        /// integral_constant values (IC...) from std::visit and forwards them along with
        /// the stored runtime arguments to the user's functor.
        ///
        /// **Performance optimization**: For common argument counts (0-2), we provide
        /// specialized fast paths that avoid std::apply overhead. This eliminates tuple
        /// indexing and lambda call overhead, improving inlining and reducing instruction
        /// count in hot dispatch paths.
        ///
        /// The optimization is most significant for:
        /// - Zero arguments: Pure template dispatching (e.g., matrix kernels)
        /// - One argument: Single runtime parameter (e.g., stride selection)
        /// - Two arguments: Common case for 2D operations
        ///
        /// For 3+ arguments, we fall back to std::apply which is still efficient but
        /// adds some indirection.
        ///
        /// \tparam IC... Pack of std::integral_constant types from std::visit
        /// \return Result of invoking the user's functor
        template<typename... IC>
        POET_HOT_LOOP auto operator()(IC... /*unused_args*/) -> decltype(auto) {
            constexpr std::size_t arg_count = std::tuple_size_v<std::remove_reference_t<ArgumentTuple>>;

            // Fast path specializations for common argument counts
            if constexpr (arg_count == 0) {
                // Zero arguments: Direct call without tuple overhead
                if constexpr (has_value_call<Functor, ArgumentTuple, IC...>::value) {
                    return (*this->func)(IC{}...);
                } else {
                    return this->func->template operator()<IC::value...>();
                }
            } else if constexpr (arg_count == 1) {
                // One argument: Direct extraction without std::apply
                if constexpr (has_value_call<Functor, ArgumentTuple, IC...>::value) {
                    return (*this->func)(IC{}..., std::move(std::get<0>(args)));
                } else {
                    return this->func->template operator()<IC::value...>(std::move(std::get<0>(args)));
                }
            } else if constexpr (arg_count == 2) {
                // Two arguments: Direct extraction without std::apply
                if constexpr (has_value_call<Functor, ArgumentTuple, IC...>::value) {
                    return (*this->func)(IC{}..., std::move(std::get<0>(args)), std::move(std::get<1>(args)));
                } else {
                    return this->func->template operator()<IC::value...>(
                      std::move(std::get<0>(args)), std::move(std::get<1>(args)));
                }
            } else {
                // General case (3+ arguments): Use std::apply for flexibility
                // The stored argument tuple is moved into the call to support move-only types
                return std::apply(
                  [this](auto &&...arg) -> decltype(auto) {
                      if constexpr (has_value_call<Functor, ArgumentTuple, IC...>::value) {
                          return (*this->func)(IC{}..., std::forward<decltype(arg)>(arg)...);
                      } else {
                          return this->func->template operator()<IC::value...>(std::forward<decltype(arg)>(arg)...);
                      }
                  },
                  std::move(args));
            }
        }
    };

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
    template<typename T> struct is_dispatch_set : std::false_type {};
    template<typename V, typename... Tuples> struct is_dispatch_set<DispatchSet<V, Tuples...>> : std::true_type {};
}// namespace detail

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_t{};

namespace detail {
    /// \brief Internal implementation for dispatch with compile-time error handling policy.
    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    auto dispatch_impl(Functor functor, ParamTuple const &params, Args... args) -> decltype(auto) {
        // 1. Convert the dispatch params (ranges) into sequences.
        auto sequences = extract_sequences(params);

        // 2. Package arguments and compute the return type of the functor when dispatched.
        using argument_tuple_type = std::tuple<std::decay_t<Args>...>;
        argument_tuple_type argument_tuple(std::move(args)...);
        using result_type = dispatch_result_t<Functor, argument_tuple_type, decltype(sequences)>;

        // 3. For each dispatch parameter, attempt to convert the runtime value into a
        // `std::variant<std::integral_constant<...>>`.
        //    Each variant contains the compile-time constant corresponding to the runtime value, or empty if out of range.
        constexpr std::size_t TupleSize = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        auto variants = make_variants(params, std::make_index_sequence<TupleSize>{});

        // 4. Check if all parameters were successfully mapped to a valid compile-time value.
        const bool success =
          std::apply([](auto const &...variant_opt) -> auto { return (variant_opt.has_value() && ...); }, variants);

        if (POET_LIKELY(success)) {
            // 5. Invoke std::visit with our custom VisitCaller.
            //    VisitCaller unwraps the integral_constants from the variants and calls the user functor:
            //    `functor.operator()<Values...>(args...)`
            using FunctorT = std::decay_t<Functor>;
            FunctorT functor_copy(std::forward<Functor>(functor));
            VisitCaller<FunctorT, decltype(argument_tuple)> caller{ &functor_copy, std::move(argument_tuple) };

            if constexpr (std::is_void_v<result_type>) {
                std::apply([&](auto &...variant_opt) -> auto { std::visit(caller, *variant_opt...); }, variants);
            } else {
                return std::apply(
                  [&](auto &...variant_opt) -> auto { return std::visit(caller, *variant_opt...); }, variants);
            }
        } else {
            if constexpr (ThrowOnNoMatch) {
                throw std::runtime_error("poet::dispatch: no matching compile-time combination for runtime inputs");
            } else {
                // Fail gracefully: return default value if none matched.
                if constexpr (!std::is_void_v<result_type>) { return result_type{}; }
            }
        }
    }
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
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<!detail::is_dispatch_set<std::decay_t<ParamTuple>>::value
                     && !std::is_same_v<std::decay_t<Functor>, throw_on_no_match_t>,
    int> = 0>
auto dispatch(Functor functor, ParamTuple const &params, Args... args) -> decltype(auto) {
    return detail::dispatch_impl<false>(std::forward<Functor>(functor), params, std::move(args)...);
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
auto dispatch(Functor &&functor, const DispatchSet<Tuples...> &dispatch_set, Args &&...args) -> decltype(auto) {
    return dispatch_tuples(std::forward<Functor>(functor),
      typename DispatchSet<Tuples...>::seq_type{},
      dispatch_set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `DispatchSet` dispatch.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/, Functor functor, const DispatchSet<Tuples...> &dispatch_set, Args... args)
  -> decltype(auto) {
    return dispatch_tuples(throw_on_no_match_t{},
      std::move(functor),
      typename DispatchSet<Tuples...>::seq_type{},
      dispatch_set.runtime_tuple(),
      std::move(args)...);
}

// Note: nothrow is the default behavior of `dispatch` (no explicit tag).

/// \brief Overload that accepts `throw_t` as a first argument.
///
/// When no match exists, this function throws `std::runtime_error`.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<!detail::is_dispatch_set<std::decay_t<ParamTuple>>::value, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/, Functor functor, ParamTuple const &params, Args... args) -> decltype(auto) {
    return detail::dispatch_impl<true>(std::forward<Functor>(functor), params, std::move(args)...);
}

}// namespace poet

// END_FILE: include/poet/core/static_dispatch.hpp
/* End inline (angle): include/poet/core/static_dispatch.hpp */
/* Begin inline (angle): include/poet/core/static_for.hpp */
// BEGIN_FILE: include/poet/core/static_for.hpp

/// \file static_for.hpp
/// \brief Provides a compile-time loop unrolling utility.
///
/// This header defines `static_for`, which unrolls loops at compile-time.
/// It is useful for iterating over template parameter packs, integer sequences,
/// or performing other meta-programming tasks where the iteration count is known
/// at compile-time.

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/for_utils.hpp */
// BEGIN_FILE: include/poet/core/for_utils.hpp

/// \file for_utils.hpp
/// \brief Internal utilities for compile-time loop unrolling and range computation.
///
/// This header provides core building blocks for `static_for` and `dynamic_for`.
/// It includes mechanisms for:
/// - Calculating iteration counts for compile-time ranges.
/// - Recursively emitting blocks of unrolled code to avoid compiler limits.
/// - Defining limits on unrolling depth (recursion depth).

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>

namespace poet {

namespace detail {

/// \brief Maximum chunk size for static loops to limit template instantiation depth.
/// Defines the maximum number of iterations or blocks processed in a single
/// recursive step. Reduced on MSVC to mitigate potential compiler stack overflows.
#if defined(_MSC_VER) && !defined(__clang__)
    inline constexpr std::size_t kMaxStaticLoopBlock = 128;
#else
    // Use a conservative upper bound for non-MSVC compilers to limit template
    // instantiation depth and keep compile times reasonable for tests.
    //
    // Capped at 256 to avoid excessive unrolling and long compile times.
    inline constexpr std::size_t kMaxStaticLoopBlock = 256;
#endif

    /// \brief Computes the number of iterations for a compile-time range.
    ///
    /// \tparam Begin Start of the range.
    /// \tparam End End of the range (exclusive).
    /// \tparam Step Iteration step (must be non-zero).
    /// \return The number of steps required to traverse from Begin to End.
    template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
    [[nodiscard]] constexpr auto compute_range_count() noexcept -> std::size_t {
        static_assert(Step != 0, "static_for requires a non-zero step");

        if constexpr (Step > 0) {
            static_assert(Begin <= End, "static_for with a positive step requires Begin <= End");
            if constexpr (Begin == End) { return 0; }
            const auto distance = End - Begin;
            return static_cast<std::size_t>((distance + Step - 1) / Step);
        } else {
            static_assert(Begin >= End, "static_for with a negative step requires Begin >= End");
            if constexpr (Begin == End) { return 0; }
            const auto distance = Begin - End;
            const auto magnitude = -Step;
            return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
        }
        // Unreachable: all cases covered by if constexpr branches above
        POET_UNREACHABLE();
    }

    /// \brief Executes a single block of unrolled loop iterations.
    ///
    /// Expands the provided indices pack into a sequence of function calls.
    /// Each call receives a `std::integral_constant` corresponding to the
    /// computed loop index.
    ///
    /// \tparam Func User callable type.
    /// \tparam Begin Range start value.
    /// \tparam Step Range step value.
    /// \tparam StartIndex The flat index offset for this block.
    /// \tparam Is Index sequence for unrolling (0, 1, ..., BlockSize-1).
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t StartIndex, std::size_t... Is>
    POET_FORCEINLINE constexpr void static_loop_impl_block(Func &func, std::index_sequence<Is...> /*indices*/) {
        // Fold expression over the index sequence Is...
        // For each compile-time index 'i' in Is:
        // 1. Compute the absolute iteration index: `StartIndex + i`
        // 2. Map to the actual value range: `Begin + (Step * absolute_index)`
        // 3. Construct an `std::integral_constant` for that value.
        // 4. Invoke `func` with that constant.
        // 5. The comma operator ... ensures sequential execution.
        // Optimization: Precompute Base = Begin + Step*StartIndex to reduce arithmetic per iteration.
        constexpr std::intmax_t Base = Begin + (Step * static_cast<std::intmax_t>(StartIndex));
        (func(std::integral_constant<std::intmax_t, Base + (Step * static_cast<std::intmax_t>(Is))>{}), ...);
    }

    /// \brief Processes a chunk of loop blocks.
    ///
    /// Recursively invokes `static_loop_impl_block` for a subset of the total blocks.
    /// Used to decompose very large loops into smaller compilation units.
    template<typename Func,
      std::intmax_t Begin,
      std::intmax_t Step,
      std::size_t BlockSize,
      std::size_t Offset,
      typename Tuple,
      std::size_t... Is>
    POET_FORCEINLINE constexpr void emit_block_chunk(Func &func, const Tuple & /*tuple*/, std::index_sequence<Is...> /*indices*/) {
        // This function processes a "chunk" of blocks to limit recursion depth.
        // It iterates over `Is...` (0 to ChunkSize-1).
        // For each `i` in `Is`:
        //   - `tuple` contains the block indices {0, 1, 2, ... FullBlocks-1}.
        //   - We access the block index at `Offset + i`.
        //   - We convert that block index into a global start index: `block_index * BlockSize`.
        //   - We call `static_loop_impl_block` to emit the iterations for that block.
        (static_loop_impl_block<Func, Begin, Step, std::tuple_element_t<Offset + Is, Tuple>::value * BlockSize>(
           func, std::make_index_sequence<BlockSize>{}),
          ...);
    }

    template<typename Func,
      std::intmax_t Begin,
      std::intmax_t Step,
      std::size_t BlockSize,
      typename Tuple,
      std::size_t Offset,
      std::size_t Remaining>
    POET_FORCEINLINE constexpr void emit_all_blocks_from_tuple(Func &func, const Tuple &tuple) {
        // Recursive function used to iterate over the tuple of block indices.
        // It consumes 'chunk_size' blocks at a time, where 'chunk_size' is capped
        // by kMaxStaticLoopBlock. This prevents generating a single massive fold
        // expression for loops with thousands of blocks (e.g., 10k iterations unrolled).
        if constexpr (Remaining > 0) {
            constexpr auto chunk_size = Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;

            // Emit the current chunk of blocks.
            emit_block_chunk<Func, Begin, Step, BlockSize, Offset>(func, tuple, std::make_index_sequence<chunk_size>{});

            // Recursively process the rest of the blocks.
            emit_all_blocks_from_tuple<Func,
              Begin,
              Step,
              BlockSize,
              Tuple,
              Offset + chunk_size,
              Remaining - chunk_size>(func, tuple);
        } else {
            // Base case: no blocks remaining.
        }
    }

    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, typename Tuple>
    POET_FORCEINLINE constexpr void emit_all_blocks(Func &func, const Tuple &tuple) {
        constexpr auto total_blocks = std::tuple_size_v<Tuple>;
        if constexpr (total_blocks > 0) {
            emit_all_blocks_from_tuple<Func, Begin, Step, BlockSize, Tuple, 0, total_blocks>(func, tuple);
        } else {
            // Zero blocks to emit.
        }
    }

    template<typename Functor> struct template_static_loop_invoker {
        Functor *functor;

        // Adapter operator:
        // Receives an std::integral_constant<int, Value> from implementation internals.
        // Unpacks 'Value' and calls the user's template operator<Value>().
        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*integral_constant*/) const {
            (*functor).template operator()<Value>();
        }
    };

}// namespace detail

// --- Public compatibility wrappers ---

inline constexpr std::size_t kMaxStaticLoopBlock = detail::kMaxStaticLoopBlock;

template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
[[nodiscard]] constexpr auto compute_range_count() noexcept -> std::size_t {
    return detail::compute_range_count<Begin, End, Step>();
}

}// namespace poet

// END_FILE: include/poet/core/for_utils.hpp
/* End inline (angle): include/poet/core/for_utils.hpp */

namespace poet {

namespace detail {

    /// \brief Helper: Emits a sequence of full blocks.
    ///
    /// This splits the loop body into manageable chunks (blocks) and delegates
    /// to `emit_all_blocks`. This partitioning prevents massive single fold
    /// expressions which can overwhelm the compiler.
    ///
    /// \tparam Func Callable type.
    /// \tparam Begin Range start.
    /// \tparam Step Range step.
    /// \tparam BlockSize Number of iterations per block.
    /// \tparam BlockIndices Indices for the blocks (0, 1, ...).
    template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t BlockSize, std::size_t... BlockIndices>
    POET_FORCEINLINE constexpr void static_loop_emit_all_blocks(Func &func, std::index_sequence<BlockIndices...> /*blocks*/) {
        // Wrap block indices into integral_constants in a tuple.
        // This tuple is then processed recursively by emit_all_blocks to avoid excessive
        // instantiation depth that a single fold expression over all blocks might cause.
        using blocks_tuple = std::tuple<std::integral_constant<std::size_t, BlockIndices>...>;
        emit_all_blocks<Func, Begin, Step, BlockSize>(func, blocks_tuple{});
    }

    /// \brief Computes a safe default block size for loop unrolling.
    ///
    /// - For small loops, returns the total iteration count.
    /// - For large loops, clamps the size to `kMaxStaticLoopBlock` to prevent
    ///   massive symbol names and excessive compile times.
    ///
    /// \tparam Begin Range start.
    /// \tparam End Range end.
    /// \tparam Step Range step.
    /// \return Optimized block size.
    template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
    constexpr auto compute_default_static_loop_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) { return 1; }
        if constexpr (count > detail::kMaxStaticLoopBlock) { return detail::kMaxStaticLoopBlock; }
        return count;
    }

    /// \brief Core loop driver.
    ///
    /// Splits the range `[0, count)` into `full_blocks` chunks of size `BlockSize`,
    /// followed by a single `remainder` block. This partitioning strategy reduces
    /// template instantiation depth.
    ///
    /// \tparam Begin Range start.
    /// \tparam End Range end.
    /// \tparam Step Range step.
    /// \tparam BlockSize Number of iterations per block.
    /// \tparam Func Callable type.
    /// \param func Callable to invoke.
    template<std::intmax_t Begin,
      std::intmax_t End,
      std::intmax_t Step = 1,
      std::size_t BlockSize = compute_default_static_loop_block_size<Begin, End, Step>(),
      typename Func>
    POET_FORCEINLINE constexpr void static_loop(Func &&func) {
        static_assert(BlockSize > 0, "static_loop requires BlockSize > 0");
        using Callable = std::remove_reference_t<Func>;
        // Create a local copy of the callable to ensure state persistence across block calls
        // if passed by rvalue, or bind a reference if passed by lvalue.
        // This is crucial for stateful functors.
        Callable callable(std::forward<Func>(func));

        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) { return; }

        // Calculate partitioning
        constexpr auto full_blocks = count / BlockSize;
        constexpr auto remainder = count % BlockSize;

        // 1. Process all full blocks
        // delegating to static_loop_emit_all_blocks which handles recursion limits
        if constexpr (full_blocks > 0) {
            static_loop_emit_all_blocks<Callable, Begin, Step, BlockSize>(
              callable, std::make_index_sequence<full_blocks>{});
        }

        // 2. Process remaining tail elements
        // The tail block is always smaller than BlockSize, so we can emit it directly
        // as a single block.
        if constexpr (remainder > 0) {
            static_loop_impl_block<Callable, Begin, Step, full_blocks * BlockSize>(
              callable, std::make_index_sequence<remainder>{});
        }
    }

}// namespace detail

/// \brief Adapts a callable to `static_loop` over a compile-time integer range.
///
/// This helper provides the unified `poet::static_for` API. It executes a
/// compile-time unrolled loop over the half-open range `[Begin, End)` using the
/// specified `Step`, where `Step` can be positive or negative. The iteration
/// space is partitioned into blocks of `BlockSize` elements, and each block is
/// emitted through `static_loop`.
///
/// Callables are supported in two forms:
/// - A callable that accepts a `std::integral_constant<std::intmax_t, I>`
///   argument. This form is constexpr-friendly and lets the implementation
///   forward the index as a compile-time value.
/// - A functor exposing `template <auto I>` call operators. In this case,
///   `static_for` internally adapts the functor to the integral-constant based
///   machinery.
///
/// The default `BlockSize` spans the entire range, clamped to
/// `poet::kMaxStaticLoopBlock` (currently `256`), to balance unrolling with
/// compile-time cost. Lvalue callables are preserved by reference, while
/// rvalues are copied into a local instance for the duration of the loop.
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
///                   total iteration count, clamped to `1` for empty ranges and
///                   to `poet::kMaxStaticLoopBlock` for large ranges).
/// \tparam Func Callable type.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t Begin,
  std::intmax_t End,
  std::intmax_t Step = 1,
  std::size_t BlockSize = detail::compute_default_static_loop_block_size<Begin, End, Step>(),
  typename Func>
POET_FORCEINLINE constexpr void static_for(Func &&func) {
    // Check if the user functor accepts an integral_constant index directly.
    if constexpr (std::is_invocable_v<Func, std::integral_constant<std::intmax_t, Begin>>) {
        // Direct invocation mode: simply forward to static_loop.
        detail::static_loop<Begin, End, Step, BlockSize>(std::forward<Func>(func));
    } else {
        // Template operator invocation mode (func.template operator()<I>()).
        // We wrap the functor in a helper (template_static_loop_invoker) that exposes
        // the integral_constant call operator expected by the backend.

        using Functor = std::remove_reference_t<Func>;

        if constexpr (std::is_lvalue_reference_v<Func>) {
            // If we got an lvalue reference, we must keep referring to the original object
            // to allow state mutation.
            const detail::template_static_loop_invoker<Functor> invoker{ &func };
            detail::static_loop<Begin, End, Step, BlockSize>(invoker);
        } else {
            // If we got an rvalue, we move-construct a local copy to keep it alive
            // during the loop execution.
            Functor functor(std::forward<Func>(func));
            const detail::template_static_loop_invoker<Functor> invoker{ &functor };
            detail::static_loop<Begin, End, Step, BlockSize>(invoker);
        }
    }
}

/// \brief Convenience overload for `static_for` iterating from 0 to `End`.
///
/// This is equivalent to `static_for<0, End>(func)`.
///
/// \tparam End Exclusive upper bound of the range.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}

}// namespace poet

// END_FILE: include/poet/core/static_for.hpp
/* End inline (angle): include/poet/core/static_for.hpp */

namespace poet {

namespace detail {

    /// \brief Helper functor that adapts a user lambda for use with static_for.
    ///
    /// This struct wraps a runtime function pointer/reference along with base and
    /// stride values. It exposes a compile-time call operator that calculates the
    /// actual runtime index based on the compile-time offset `I` and invokes the
    /// user function.
    ///
    /// \tparam Func Type of the user-provided callable.
    /// \tparam T Type of the loop counter (e.g., int, size_t).
    template<typename Func, typename T> struct dynamic_block_invoker {
        Func *func;
        T base;
        T stride;

        template<auto I> constexpr void operator()() const {
            // Invoke the user lambda with the current runtime index.
            // The runtime index is computed as: base + (compile_time_offset * stride).
            (*func)(base + (static_cast<T>(I) * stride));
        }
    };

    /// \brief Emits a block of unrolled iterations.
    ///
    /// Invokes `static_for` to generate `BlockSize` calls to the user function.
    /// This helper is used for both the main unrolled loop body and the
    /// tail handling (where the block size is determined at runtime via dispatch).
    ///
    /// \tparam Func User callable type.
    /// \tparam T Loop counter type.
    /// \tparam BlockSize Number of iterations to unroll in this block.
    template<typename Func, typename T, std::size_t BlockSize>
    POET_HOT_LOOP void execute_runtime_block([[maybe_unused]] Func &func, [[maybe_unused]] T base, [[maybe_unused]] T stride) {
        if constexpr (BlockSize > 0) {
            // Create an invoker that captures the user function and the current base index.
            dynamic_block_invoker<Func, T> invoker{ &func, base, stride };

            // Use static_for to generate exactly 'BlockSize' compile-time calls.
            // We pass BlockSize as the unrolling factor to ensure a single unrolled block is emitted.
            // The range [0, BlockSize) will be iterated.
            static_for<0, static_cast<std::intmax_t>(BlockSize), 1, BlockSize>(invoker);
        } else {
            // Nothing to do when the block size is zero.
        }
    }

    /// \brief Functor for dispatching the tail of a dynamic loop.
    ///
    /// When the stored `Tail` template parameter matches the runtime remainder,
    /// this operator invokes `execute_runtime_block` with the correct
    /// compile-time block size.
    template<typename Callable, typename T> struct tail_caller_for_dynamic_for {
        Callable *callable;
        T stride;

        template<int Tail> void operator()(T base) const {
            // This function is instantiated by the dispatcher for a specific 'Tail' value.
            // We call execute_runtime_block with 'Tail' as the compile-time block size.
            execute_runtime_block<Callable, T, static_cast<std::size_t>(Tail)>(*callable, base, stride);
        }
    };

    /// \brief Internal implementation of the dynamic loop.
    ///
    /// Calculates the total number of iterations required based on proper signed
    /// arithmetic and executes the loop in chunks of `Unroll`. Any remaining
    /// iterations are handled by `poet::dispatch`.
    ///
    /// \param begin Inclusive start of the iteration range.
    /// \param end Exclusive end of the iteration range.
    /// \param stride Step/increment value (can be negative for backward iteration).
    /// \param callable User-provided function to invoke for each index.
    template<typename T, typename Callable, std::size_t Unroll>
    POET_HOT_LOOP void dynamic_for_impl(T begin, T end, T stride, Callable &callable) {
        if (POET_UNLIKELY(stride == 0)) { return; }

        // Calculate iteration count.
        // We determine the number of steps to go from `begin` to `end` exclusively.
        std::size_t count = 0;

        // For unsigned types, detect wrapped negative values caused by implicit conversion.
        // When users pass negative literals (e.g., -1, -5) to unsigned parameters, C++ performs
        // implicit conversion via unsigned wrapping, resulting in very large positive values:
        //   - For uint32_t: -1 → 4294967295 (UINT_MAX), -2 → 4294967294, etc.
        //   - For uint64_t: -1 → 18446744073709551615 (ULLONG_MAX), -2 → 18446744073709551614, etc.
        //
        // Detection heuristic: Values > (max/2) are likely wrapped negatives.
        // This works because:
        //   1. Negative values -1 to -N map to [max, max-N+1] (all > max/2)
        //   2. Normal positive strides are typically small (< max/2)
        //   3. Edge case: Very large positive strides (> max/2) would be misidentified,
        //      but such strides are impractical (would cause integer overflow in loops)
        //
        // Examples for uint32_t (max = 4294967295, half_max = 2147483647):
        //   - stride = static_cast<uint32_t>(-1)  → 4294967295 > 2147483647 → detected as wrapped negative ✓
        //   - stride = static_cast<uint32_t>(-5)  → 4294967291 > 2147483647 → detected as wrapped negative ✓
        //   - stride = 100                        → 100 < 2147483647          → normal positive stride ✓
        //   - stride = 1000000                    → 1000000 < 2147483647      → normal positive stride ✓
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (POET_UNLIKELY(stride < 0 || is_wrapped_negative)) {
            // Backward iteration (negative stride or wrapped unsigned)
            if (POET_UNLIKELY(begin <= end)) {
                count = 0;
            } else {
                // Compute absolute stride value
                T abs_stride;
                if constexpr (std::is_signed_v<T>) {
                    abs_stride = static_cast<T>(-stride);  // Safe for signed types
                } else {
                    // For unsigned wrapped values: unsigned(-1) → abs is 1
                    abs_stride = static_cast<T>(0) - stride;  // Wrapping subtraction
                }
                auto dist = static_cast<std::size_t>(begin - end);
                auto ustride = static_cast<std::size_t>(abs_stride);
                count = (dist + ustride - 1) / ustride;
            }
        } else {
            // Forward iteration (stride > 0 and not wrapped) - COMMON CASE
            if (POET_UNLIKELY(begin >= end)) {
                count = 0;
            } else {
                // Logic for positive stride:
                // dist = end - begin
                // count = ceil(dist / stride) = (dist + stride - 1) / stride
                //
                // Optimization: For power-of-2 strides, replace expensive division with bit shift.
                // This is a common case (strides of 1, 2, 4, 8, 16 are typical in DSP/linear algebra).
                auto dist = static_cast<std::size_t>(end - begin);
                auto ustride = static_cast<std::size_t>(stride);

                // Check if stride is a power of 2: (x & (x-1)) == 0 for powers of 2.
                // Note: 0 is not a power of 2, but we already checked stride == 0 at the top.
                const bool is_power_of_2 = (ustride & (ustride - 1)) == 0;

                if (POET_LIKELY(is_power_of_2)) {
                    // Fast path: Use bit shift for power-of-2 division.
                    // count = ceil(dist / stride) = (dist + stride - 1) >> log2(stride)
                    const unsigned int shift = poet_count_trailing_zeros(ustride);
                    count = (dist + ustride - 1) >> shift;
                } else {
                    // General path: Use division for non-power-of-2 strides.
                    count = (dist + ustride - 1) / ustride;
                }
            }
        }

        T index = begin;
        std::size_t remaining = count;

        // Execute full blocks of size 'Unroll'.
        // We use a runtime while loop here, but the body (execute_runtime_block)
        // is fully unrolled at compile-time for 'Unroll' iterations.
        // Optimization: Hoist loop-invariant multiplication out of the loop.
        const T stride_times_unroll = static_cast<T>(Unroll) * stride;
        while (remaining >= Unroll) {
            detail::execute_runtime_block<Callable, T, Unroll>(callable, index, stride);
            index += stride_times_unroll;
            remaining -= Unroll;
        }

        // Handle remaining iterations (tail).
        if constexpr (Unroll > 1) {
            if (POET_UNLIKELY(remaining > 0)) {
                // Dispatch the runtime 'remaining' count to a compile-time template instantiation.
                // This ensures even the tail is unrolled, avoiding a runtime loop for the last few elements.
                const detail::tail_caller_for_dynamic_for<Callable, T> tail_caller{ &callable, stride };
                // Define the allowed range of tail sizes: [0, Unroll - 1].
                using TailRange = poet::make_range<0, (static_cast<int>(Unroll) - 1)>;
                auto params = std::make_tuple(poet::DispatchParam<TailRange>{ static_cast<int>(remaining) });
                // Invoke dispatch. This will find the matching CompileTimeTail in TailRange
                // and call tail_caller.operator()<CompileTimeTail>(index).
                poet::dispatch(tail_caller, params, index);
            }
        }
    }

}// namespace detail

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// The helper iterates over the half-open range `[begin, end)` with a given
/// `step`. Blocks of `Unroll` iterations are dispatched through `static_for`.
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \param begin Inclusive lower/start bound.
/// \param end Exclusive upper/end bound.
/// \param step Increment per iteration. Can be negative.
/// \param func Callable invoked for each iteration.
constexpr std::size_t kDefaultUnroll = 8;

template<std::size_t Unroll = kDefaultUnroll, typename T1, typename T2, typename T3, typename Func>
inline void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2, T3>;
    using Callable = std::remove_reference_t<Func>;

    [[maybe_unused]] Callable callable(std::forward<Func>(func));

    // Delegate to implementation
    detail::dynamic_for_impl<T, Callable, Unroll>(
      static_cast<T>(begin), static_cast<T>(end), static_cast<T>(step), callable);
}

/// \brief Executes a runtime-sized loop using compile-time unrolling with
/// automatic step detection.
///
/// If `begin <= end`, step is +1.
/// If `begin > end`, step is -1.
template<std::size_t Unroll = kDefaultUnroll, typename T1, typename T2, typename Func>
inline void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    T s_begin = static_cast<T>(begin);
    T s_end = static_cast<T>(end);
    T step = (s_begin <= s_end) ? static_cast<T>(1) : static_cast<T>(-1);

    // Call general overload
    dynamic_for<Unroll>(s_begin, s_end, step, std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop from zero using compile-time unrolling.
///
/// This overload iterates over the range `[0, count)`.
template<std::size_t Unroll = kDefaultUnroll, typename Func> inline void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll>(static_cast<std::size_t>(0), count, std::forward<Func>(func));
}

} // namespace poet


#if __cplusplus >= 202002L
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cstddef>

namespace poet {

// Adaptor holds the user callable.
// Template ordering: Func first (deduced), Unroll second (optional).
template<typename Func, std::size_t Unroll = poet::kDefaultUnroll>
struct dynamic_for_adaptor {
  Func func;
  dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

// Range overload: accept any std::ranges::range.
// Interprets the range as a sequence of consecutive indices starting at *begin(range).
// This implementation computes the distance by iterating the range (works even when not sized).
template<typename Func, std::size_t Unroll, typename Range>
  requires std::ranges::range<Range>
void operator|(Range &&r, dynamic_for_adaptor<Func, Unroll> const &ad) {
  auto it = std::ranges::begin(r);
  auto it_end = std::ranges::end(r);

  if (it == it_end) return; // empty range

  using ValT = std::remove_reference_t<decltype(*it)>;
  ValT start = *it;

  std::size_t count = 0;
  for (auto jt = it; jt != it_end; ++jt) ++count;

  // Call dynamic_for with [start, start+count) using step = +1
  poet::dynamic_for<Unroll>(start, static_cast<ValT>(start + static_cast<ValT>(count)), ad.func);
}

// Tuple overload: accept tuple-like (begin, end, step)
template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
  auto [b, e, s] = t;
  poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

// Helper to construct adaptor with type deduction
template<std::size_t U = poet::kDefaultUnroll, typename F>
dynamic_for_adaptor<std::decay_t<F>, U> make_dynamic_for(F &&f) {
  return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

} // namespace poet
#endif // __cplusplus >= 202002L

// END_FILE: include/poet/core/dynamic_for.hpp
/* End inline (angle): include/poet/core/dynamic_for.hpp */
/* Begin inline (angle): include/poet/core/static_dispatch.hpp */
/* Skipped already inlined: include/poet/core/static_dispatch.hpp */
/* End inline (angle): include/poet/core/static_dispatch.hpp */
/* Begin inline (angle): include/poet/core/static_for.hpp */
/* Skipped already inlined: include/poet/core/static_for.hpp */
/* End inline (angle): include/poet/core/static_for.hpp */
#include <poet/core/undef_macros.hpp>
// clang-format on
// END_FILE: include/poet/poet.hpp

#endif // POET_SINGLE_HEADER_GOLDBOT_HPP
