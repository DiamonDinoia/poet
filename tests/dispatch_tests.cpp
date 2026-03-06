// cppcheck-suppress-file unknownMacro
#include <poet/core/dispatch.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <memory>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

// ============================================================================
// Test support types (shared across all dispatch tests)
// ============================================================================

namespace {
using poet::DispatchParam;
using poet::DispatchSet;
using poet::dispatch;
using poet::make_range;
using poet::T;
using poet::throw_t;

static_assert(std::is_same_v<make_range<0, 0>, std::integer_sequence<int, 0>>,
  "make_range should include the end value");
static_assert(std::is_same_v<make_range<2, 4>, std::integer_sequence<int, 2, 3, 4>>,
  "make_range should produce inclusive ranges");
static_assert(std::is_same_v<make_range<-2, 1>, std::integer_sequence<int, -2, -1, 0, 1>>,
  "make_range should support negative bounds");

struct vector_dispatcher {
    std::vector<int> *values;

    template<int Width, int Height> void operator()(int scale) const { values->push_back(scale + Width * 10 + Height); }
};

struct sum_dispatcher {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X + Y + Z; }
};

struct guard_dispatcher {
    bool *invoked;

    template<int Value> int operator()(int base) const {
        *invoked = true;
        return base + Value;
    }
};

struct return_type_dispatcher {
    template<int X> double operator()(double multiplier) const { return X * multiplier; }
};

struct mover {
    template<int X> std::unique_ptr<int> operator()(int base) const { return std::make_unique<int>(base + X); }
};

struct receiver {
    template<int X> int operator()(std::unique_ptr<int> p) const { return X + *p; }
};

struct probe {
    int *out;
    template<int X, int Y> void operator()(int base) const { *out = base + X + Y; }
};

struct duplicate_reporter {
    int *out;
    template<int X> void operator()() const { *out = X; }
};

struct throwing_dispatcher {
    int *counter;

    template<int X> void operator()(int threshold) const {
        (*counter)++;
        if (X >= threshold) { throw std::runtime_error("dispatch exception"); }
    }
};

struct triple_dispatcher {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X * 100 + Y * 10 + Z; }
};

struct value_arg_functor {
    template<int X, int Y>
    int operator()(std::integral_constant<int, X>, std::integral_constant<int, Y>, int base) const {
        return base + X * 10 + Y;
    }
};

struct accumulating_dispatcher {
    int *total;
    template<int N> void operator()(int add) const { *total += N + add; }
};

// DispatchSet tuple functors
struct tuple_sum {
    template<int X, int Y> int operator()(int base) const { return base + X + Y; }
};

struct tuple_voider {
    int *out;
    template<int X, int Y> void operator()(int add) const { *out = add + X + Y; }
};

struct triple_sum {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X + Y + Z; }
};

struct quad_sum {
    template<int X, int Y, int Z, int W> int operator()(int base) const { return base + X + Y + Z + W; }
};

}// namespace

// ============================================================================
// Basic dispatch tests
// ============================================================================

TEST_CASE("dispatch routes to the matching template instantiation", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<-2, 1>>{ -1 });

    dispatch(vector_dispatcher{ &values }, params, 5);

    REQUIRE(values == std::vector<int>{ 24 });
}

TEST_CASE("dispatch forwards runtime arguments to the functor", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{ 5 },
      DispatchParam<make_range<1, 3>>{ 2 },
      DispatchParam<make_range<-1, 1>>{ 0 });

    const auto result = dispatch(sum_dispatcher{}, params, 10);

    REQUIRE(result == 17);
}

TEST_CASE("dispatch returns default values when no match exists", "[static_dispatch]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 3 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 8);

    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch handles single-element ranges", "[static_dispatch]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<5, 5>>{ 5 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 10);

    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch handles boundary values in ranges", "[static_dispatch]") {
    std::vector<int> values;

    SECTION("minimum boundary") {
        auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 0 }, DispatchParam<make_range<-2, 1>>{ -2 });
        dispatch(vector_dispatcher{ &values }, params, 5);
        REQUIRE(values == std::vector<int>{ 3 });
    }

    SECTION("maximum boundary") {
        auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 3 }, DispatchParam<make_range<-2, 1>>{ 1 });
        dispatch(vector_dispatcher{ &values }, params, 5);
        REQUIRE(values == std::vector<int>{ 36 });
    }
}

TEST_CASE("dispatch handles all negative ranges", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<-5, -2>>{ -3 },
      DispatchParam<make_range<-10, -8>>{ -9 },
      DispatchParam<make_range<-1, 0>>{ 0 });

    const auto result = dispatch(sum_dispatcher{}, params, 100);

    REQUIRE(result == 88);
}

TEST_CASE("dispatch handles ranges crossing zero", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<-3, 3>>{ 0 }, DispatchParam<make_range<-1, 1>>{ 0 });

    dispatch(vector_dispatcher{ &values }, params, 7);

    REQUIRE(values == std::vector<int>{ 7 });
}

TEST_CASE("dispatch handles void return type explicitly", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<1, 2>>{ 1 }, DispatchParam<make_range<3, 4>>{ 4 });

    dispatch(vector_dispatcher{ &values }, params, 0);

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 14);
}

TEST_CASE("dispatch handles multiple out-of-range parameters", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 10 }, DispatchParam<make_range<5, 7>>{ 15 });

    dispatch(vector_dispatcher{ &values }, params, 8);

    REQUIRE(values.empty());
}

TEST_CASE("dispatch handles non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NonContiguousSeq = std::integer_sequence<int, 1, 3, 7, 12>;

    std::vector<int> values;

    SECTION("matches first element") {
        auto params =
          std::make_tuple(DispatchParam<NonContiguousSeq>{ 1 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 2 });

        dispatch(vector_dispatcher{ &values }, params, 10);
        REQUIRE(values == std::vector<int>{ 22 });
    }

    SECTION("matches middle element") {
        auto params =
          std::make_tuple(DispatchParam<NonContiguousSeq>{ 7 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 5 });

        dispatch(vector_dispatcher{ &values }, params, 10);
        REQUIRE(values == std::vector<int>{ 85 });
    }

    SECTION("matches last element") {
        auto params =
          std::make_tuple(DispatchParam<NonContiguousSeq>{ 12 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 2 });

        dispatch(vector_dispatcher{ &values }, params, 10);
        REQUIRE(values == std::vector<int>{ 132 });
    }

    SECTION("value between sequence elements fails") {
        bool invoked = false;
        auto params = std::make_tuple(DispatchParam<NonContiguousSeq>{ 5 });

        const auto result = dispatch(guard_dispatcher{ &invoked }, params, 10);
        REQUIRE(result == 0);
        REQUIRE_FALSE(invoked);
    }
}

TEST_CASE("dispatch handles mixed contiguous and non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NonContiguousSeq = std::integer_sequence<int, 0, 10, 20>;

    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<NonContiguousSeq>{ 10 }, DispatchParam<make_range<1, 3>>{ 2 });

    dispatch(vector_dispatcher{ &values }, params, 5);
    REQUIRE(values == std::vector<int>{ 107 });
}

TEST_CASE("dispatch deterministic with duplicate sequence values", "[static_dispatch][duplicates]") {
    int out = 0;
    using DupSeq = std::integer_sequence<int, 5, 7, 5>;
    auto params = std::make_tuple(DispatchParam<DupSeq>{ 5 });

    dispatch(duplicate_reporter{ &out }, params);

    REQUIRE(out == 5);
}

TEST_CASE("dispatch handles negative non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NegativeSeq = std::integer_sequence<int, -10, -5, 0, 7>;

    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<NegativeSeq>{ -5 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 20);
    REQUIRE(result == 15);
    REQUIRE(invoked);
}

// ============================================================================
// Variadic dispatch form (DispatchParam args directly, no std::make_tuple)
// ============================================================================

TEST_CASE("dispatch variadic form 1D", "[static_dispatch][variadic]") {
    bool invoked = false;
    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 5>>{ 3 }, 10);
    REQUIRE(result == 13);
    REQUIRE(invoked);
}

TEST_CASE("dispatch variadic form 2D", "[static_dispatch][variadic]") {
    std::vector<int> values;
    dispatch(
      vector_dispatcher{ &values }, DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<-2, 1>>{ -1 }, 5);
    REQUIRE(values == std::vector<int>{ 24 });
}

TEST_CASE("dispatch variadic form no match", "[static_dispatch][variadic]") {
    bool invoked = false;
    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 2>>{ 10 }, 5);
    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch variadic form with no extra args", "[static_dispatch][variadic]") {
    int out = 0;
    dispatch(duplicate_reporter{ &out }, DispatchParam<make_range<3, 7>>{ 5 });
    REQUIRE(out == 5);
}

// ============================================================================
// Sparse 1D dispatch (non-contiguous single dimension)
// ============================================================================

TEST_CASE("dispatch sparse 1D iterates all values", "[static_dispatch][sparse]") {
    using Sparse = std::integer_sequence<int, 1, 5, 10, 50>;
    for (int val : { 1, 5, 10, 50 }) {
        bool invoked = false;
        const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<Sparse>{ val }, 0);
        REQUIRE(result == val);
        REQUIRE(invoked);
    }
}

TEST_CASE("dispatch sparse 1D miss between values", "[static_dispatch][sparse]") {
    using Sparse = std::integer_sequence<int, 1, 5, 10, 50>;
    for (int val : { 0, 2, 6, 11, 49, 51 }) {
        bool invoked = false;
        const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<Sparse>{ val }, 0);
        REQUIRE(result == 0);
        REQUIRE_FALSE(invoked);
    }
}

// ============================================================================
// Sparse 1D dispatch — strided (equal-gap) path
// ============================================================================

TEST_CASE("dispatch strided sparse 1D hits all values", "[static_dispatch][sparse][strided]") {
    // {0, 10, 20}: stride=10 → is_strided=true → O(1) lookup
    using StridedSeq = std::integer_sequence<int, 0, 10, 20>;
    for (int val : { 0, 10, 20 }) {
        bool invoked = false;
        const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<StridedSeq>{ val }, 0);
        REQUIRE(result == val);
        REQUIRE(invoked);
    }
}

TEST_CASE("dispatch strided sparse 1D miss cases", "[static_dispatch][sparse][strided]") {
    using StridedSeq = std::integer_sequence<int, 0, 10, 20>;
    // negative diff (below first)
    {
        bool inv = false;
        REQUIRE(dispatch(guard_dispatcher{ &inv }, DispatchParam<StridedSeq>{ -5 }, 0) == 0);
        REQUIRE_FALSE(inv);
    }
    // in range but not a multiple of stride
    {
        bool inv = false;
        REQUIRE(dispatch(guard_dispatcher{ &inv }, DispatchParam<StridedSeq>{ 5 }, 0) == 0);
        REQUIRE_FALSE(inv);
    }
    // beyond last element (idx >= unique_count)
    {
        bool inv = false;
        REQUIRE(dispatch(guard_dispatcher{ &inv }, DispatchParam<StridedSeq>{ 30 }, 0) == 0);
        REQUIRE_FALSE(inv);
    }
}

// ============================================================================
// Sparse 1D dispatch — non-strided (unequal-gap) path
// ============================================================================

TEST_CASE("dispatch non-strided sparse 1D hits all values", "[static_dispatch][sparse][non-strided]") {
    // {1, 3, 7}: stride0=2 but 7-3=4 ≠ 2 → is_strided=false → binary search
    using UnequalSeq = std::integer_sequence<int, 1, 3, 7>;
    for (int val : { 1, 3, 7 }) {
        bool invoked = false;
        const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<UnequalSeq>{ val }, 0);
        REQUIRE(result == val);
        REQUIRE(invoked);
    }
}

TEST_CASE("dispatch non-strided sparse 1D miss cases", "[static_dispatch][sparse][non-strided]") {
    using UnequalSeq = std::integer_sequence<int, 1, 3, 7>;
    for (int val : { 0, 2, 4, 5, 6, 8 }) {
        bool invoked = false;
        const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<UnequalSeq>{ val }, 0);
        REQUIRE(result == 0);
        REQUIRE_FALSE(invoked);
    }
}

// ============================================================================
// Stateful functor
// ============================================================================

TEST_CASE("dispatch with stateful functor", "[static_dispatch][stateful]") {
    int total = 0;
    accumulating_dispatcher func{ &total };

    auto p1 = std::make_tuple(DispatchParam<make_range<0, 5>>{ 2 });
    dispatch(func, p1, 10);
    REQUIRE(total == 12);// 2 + 10

    auto p2 = std::make_tuple(DispatchParam<make_range<0, 5>>{ 4 });
    dispatch(func, p2, 100);
    REQUIRE(total == 116);// 12 + 4 + 100
}

// ============================================================================
// Throw tests
// ============================================================================

TEST_CASE("dispatch throws when no match exists with throw_t (non-void)", "[static_dispatch][throw]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 3 });

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 8), std::runtime_error);

    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch throws when no match exists with throw_t (void)", "[static_dispatch][throw]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<1, 2>>{ 3 }, DispatchParam<make_range<3, 4>>{ 4 });

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, vector_dispatcher{ &values }, params, 0), std::runtime_error);

    REQUIRE(values.empty());
}

TEST_CASE("dispatch with throw_t succeeds on valid match", "[static_dispatch][throw]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{ 3 });

    const auto result = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 100);

    REQUIRE(invoked);
    REQUIRE(result == 103);
}

TEST_CASE("dispatch with throw_t handles multiple parameters correctly", "[static_dispatch][throw]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 3>>{ 2 },
      DispatchParam<make_range<5, 7>>{ 6 },
      DispatchParam<make_range<10, 12>>{ 11 });

    const auto result = dispatch(poet::throw_t, sum_dispatcher{}, params, 100);
    REQUIRE(result == 119);

    auto bad_params1 = std::make_tuple(DispatchParam<make_range<1, 3>>{ 0 },
      DispatchParam<make_range<5, 7>>{ 6 },
      DispatchParam<make_range<10, 12>>{ 11 });
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, sum_dispatcher{}, bad_params1, 100), std::runtime_error);
}

TEST_CASE("dispatch with throw_t handles boundary values", "[static_dispatch][throw]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -10 });

    const auto result1 = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 50);
    REQUIRE(invoked);
    REQUIRE(result1 == 40);

    invoked = false;
    auto params2 = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -5 });

    const auto result2 = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params2, 50);
    REQUIRE(invoked);
    REQUIRE(result2 == 45);

    auto params3 = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -11 });
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params3, 50), std::runtime_error);

    auto params4 = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -4 });
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params4, 50), std::runtime_error);
}

TEST_CASE("dispatch with throw_t preserves return type deduction", "[static_dispatch][throw]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });

    const auto result = dispatch(poet::throw_t, return_type_dispatcher{}, params, 2.5);

    static_assert(std::is_same_v<decltype(result), const double>, "Return type should be double");
    REQUIRE(result == 7.5);
}

TEST_CASE("dispatch with throw_t variadic form", "[static_dispatch][throw][variadic]") {
    bool invoked = false;

    SECTION("throws on miss") {
        REQUIRE_THROWS_AS(
          dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 2>>{ 10 }, 5),
          std::runtime_error);
        REQUIRE_FALSE(invoked);
    }

    SECTION("succeeds on hit") {
        const auto result =
          dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 5>>{ 3 }, 10);
        REQUIRE(invoked);
        REQUIRE(result == 13);
    }
}

TEST_CASE("dispatch with throw_t tuple form", "[static_dispatch][throw][tuple]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 2 });

    SECTION("succeeds on hit") {
        bool invoked = false;
        const auto result = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 10);
        REQUIRE(result == 12);
        REQUIRE(invoked);
    }

    SECTION("throws on miss") {
        auto bad = std::make_tuple(DispatchParam<make_range<0, 3>>{ 10 });
        bool invoked = false;
        REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, bad, 5), std::runtime_error);
        REQUIRE_FALSE(invoked);
    }
}

// ============================================================================
// Advanced tests
// ============================================================================

TEST_CASE("dispatch preserves return value types correctly", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });
    const auto result = dispatch(return_type_dispatcher{}, params, 2.5);

    REQUIRE(result == 7.5);
    REQUIRE(std::is_same_v<decltype(result), const double>);
}

TEST_CASE("dispatch return type is double at runtime", "[static_dispatch][type]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });
    auto result = dispatch(return_type_dispatcher{}, params, 2.5);
    REQUIRE(typeid(result) == typeid(double));
}

TEST_CASE("dispatch handles move-only return types", "[static_dispatch][move-only]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 1 });
    auto result = dispatch(mover{}, params, 10);
    REQUIRE(result);
    REQUIRE(*result == 11);
}

TEST_CASE("dispatch forwards move-only arguments", "[static_dispatch][forwarding]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 1>>{ 1 });

    std::unique_ptr<int> p = std::make_unique<int>(7);
    auto result = dispatch(receiver{}, params, std::move(p));
    REQUIRE(result == 8);
}

TEST_CASE("dispatch moderate table stress", "[static_dispatch][stress]") {
    using A = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;
    using B = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;

    int out = 0;

    auto params = std::make_tuple(DispatchParam<A>{ 3 }, DispatchParam<B>{ 4 });
    dispatch(probe{ &out }, params, 100);
    REQUIRE(out == 107);
}

TEST_CASE("dispatch handles contiguous descending sequence", "[static_dispatch][contiguous-descending]") {
    std::vector<int> values;
    using Desc = std::integer_sequence<int, 6, 5, 4, 3, 2, 1, 0>;
    for (int v : { 6, 5, 4, 3, 2, 1, 0 }) {
        auto params = std::make_tuple(DispatchParam<Desc>{ v }, DispatchParam<make_range<0, 0>>{ 0 });
        dispatch(vector_dispatcher{ &values }, params, 5);
    }
    REQUIRE(values == std::vector<int>{ 65, 55, 45, 35, 25, 15, 5 });
}

TEST_CASE("dispatch 1D descending contiguous sequence (seq_lookup path)", "[static_dispatch][contiguous-descending]") {
    using Desc = std::integer_sequence<int, 5, 4, 3, 2, 1>;
    int total = 0;
    for (int v : { 5, 4, 3, 2, 1 }) { dispatch(accumulating_dispatcher{ &total }, DispatchParam<Desc>{ v }, 0); }
    REQUIRE(total == 1 + 2 + 3 + 4 + 5);
}

TEST_CASE("dispatch 1D descending out-of-range returns default", "[static_dispatch][contiguous-descending]") {
    using Desc = std::integer_sequence<int, 5, 4, 3, 2, 1>;
    bool invoked = false;
    dispatch(guard_dispatcher{ &invoked }, DispatchParam<Desc>{ 0 }, 0);
    REQUIRE_FALSE(invoked);
    dispatch(guard_dispatcher{ &invoked }, DispatchParam<Desc>{ 6 }, 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch 2D both-descending contiguous sequences (fused path)", "[static_dispatch][contiguous-descending]") {
    std::vector<int> values;
    using D1 = std::integer_sequence<int, 3, 2, 1>;
    using D2 = std::integer_sequence<int, 2, 1, 0>;
    auto params = std::make_tuple(DispatchParam<D1>{ 2 }, DispatchParam<D2>{ 1 });
    dispatch(vector_dispatcher{ &values }, params, 0);
    // Width=2, Height=1, scale=0: 0 + 2*10 + 1 = 21
    REQUIRE(values == std::vector<int>{ 21 });
}

TEST_CASE("dispatch preserved non-throwing behavior with nothrow_on_no_match (non-void)",
  "[static_dispatch][nothrow]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 3 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 8);

    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch preserved non-throwing behavior with nothrow_on_no_match (void)", "[static_dispatch][nothrow]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<1, 2>>{ 3 }, DispatchParam<make_range<3, 4>>{ 4 });

    REQUIRE_NOTHROW(dispatch(vector_dispatcher{ &values }, params, 0));

    REQUIRE(values.empty());
}

TEST_CASE("dispatch with single-value repeated sequence", "[static_dispatch][edge_case]") {
    using RepeatedSeq = std::integer_sequence<int, 5, 5, 5, 5>;
    bool invoked = false;

    auto params = std::make_tuple(DispatchParam<RepeatedSeq>{ 5 });
    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 10);

    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch with value-argument form (integral_constant parameters)", "[static_dispatch][value-args]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<0, 3>>{ 1 });
    const auto result = dispatch(value_arg_functor{}, params, 5);

    REQUIRE(result == 26);
}

TEST_CASE("dispatch 3D (triple dispatch)", "[static_dispatch][3d]") {
    auto params = std::make_tuple(
      DispatchParam<make_range<0, 2>>{ 1 }, DispatchParam<make_range<0, 2>>{ 2 }, DispatchParam<make_range<0, 2>>{ 0 });

    const auto result = dispatch(triple_dispatcher{}, params, 5);
    REQUIRE(result == 125);
}

TEST_CASE("dispatch exception safety", "[static_dispatch][exception]") {
    int count = 0;
    auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{ 3 });

    try {
        dispatch(throwing_dispatcher{ &count }, params, 2);
        FAIL("Expected exception was not thrown");
    } catch (const std::runtime_error &) { REQUIRE(count == 1); }
}

TEST_CASE("dispatch with single-element non-contiguous sequence", "[static_dispatch][edge_case]") {
    using SingleSeq = std::integer_sequence<int, 42>;
    bool invoked = false;

    auto params = std::make_tuple(DispatchParam<SingleSeq>{ 42 });
    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 100);

    REQUIRE(result == 142);
    REQUIRE(invoked);

    invoked = false;
    auto params2 = std::make_tuple(DispatchParam<SingleSeq>{ 41 });
    const auto result2 = dispatch(guard_dispatcher{ &invoked }, params2, 100);

    REQUIRE(result2 == 0);
    REQUIRE_FALSE(invoked);
}

// ============================================================================
// DispatchSet / tuple tests
// ============================================================================

TEST_CASE("DispatchSet matches exact allowed tuples", "[static_dispatch][tuples]") {
    using DS = DispatchSet<int, T<1, 2>, T<2, 4>>;
    auto ds = DS(2, 4);

    const auto result = dispatch(tuple_sum{}, ds, 10);
    REQUIRE(result == 16);
}

TEST_CASE("DispatchSet returns default when no match", "[static_dispatch][tuples]") {
    using DS = DispatchSet<int, T<1, 2>, T<2, 4>>;
    auto ds = DS(3, 3);

    const auto result = dispatch(tuple_sum{}, ds, 5);
    REQUIRE(result == 0);
}

TEST_CASE("DispatchSet supports void return and side-effects", "[static_dispatch][tuples]") {
    using DS = DispatchSet<int, T<0, 0>, T<5, 7>>;
    auto ds = DS(5, 7);
    int out = 0;

    dispatch(tuple_voider{ &out }, ds, 3);
    REQUIRE(out == 15);
}

TEST_CASE("DispatchSet throws when requested and no match", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 1>>;
    auto ds = DS(9, 9);

    // cppcheck-suppress unknownMacro
    REQUIRE_THROWS_AS(dispatch(throw_t, tuple_sum{}, ds, 0), std::runtime_error);
}

TEST_CASE("DispatchSet with throw_t succeeds on valid match", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 2>, T<3, 4>, T<5, 6>>;
    auto ds = DS(3, 4);

    const auto result = dispatch(throw_t, tuple_sum{}, ds, 10);
    REQUIRE(result == 17);// 10 + 3 + 4
}

TEST_CASE("DispatchSet with throw_t handles multiple valid tuples", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 2>, T<3, 4>, T<5, 6>>;

    auto ds1 = DS(1, 2);
    REQUIRE(dispatch(throw_t, tuple_sum{}, ds1, 0) == 3);

    auto ds2 = DS(3, 4);
    REQUIRE(dispatch(throw_t, tuple_sum{}, ds2, 0) == 7);

    auto ds3 = DS(5, 6);
    REQUIRE(dispatch(throw_t, tuple_sum{}, ds3, 0) == 11);

    auto ds_invalid = DS(2, 3);
    REQUIRE_THROWS_AS(dispatch(throw_t, tuple_sum{}, ds_invalid, 0), std::runtime_error);
}

TEST_CASE("DispatchSet with throw_t handles void return type", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 2>, T<3, 4>>;

    int result = 0;
    auto ds1 = DS(1, 2);
    dispatch(throw_t, tuple_voider{ &result }, ds1, 100);
    REQUIRE(result == 103);// 100 + 1 + 2

    auto ds2 = DS(3, 4);
    dispatch(throw_t, tuple_voider{ &result }, ds2, 50);
    REQUIRE(result == 57);// 50 + 3 + 4

    auto ds_invalid = DS(2, 3);
    REQUIRE_THROWS_AS(dispatch(throw_t, tuple_voider{ &result }, ds_invalid, 100), std::runtime_error);
}

TEST_CASE("DispatchSet with 3-tuples", "[static_dispatch][tuples][arity-3]") {
    using DS = DispatchSet<int, T<1, 2, 3>, T<4, 5, 6>, T<7, 8, 9>>;

    auto ds1 = DS(1, 2, 3);
    REQUIRE(dispatch(::triple_sum{}, ds1, 10) == 16);

    auto ds2 = DS(4, 5, 6);
    REQUIRE(dispatch(::triple_sum{}, ds2, 10) == 25);

    auto ds3 = DS(7, 8, 9);
    REQUIRE(dispatch(::triple_sum{}, ds3, 10) == 34);

    auto ds_invalid = DS(1, 2, 4);
    REQUIRE(dispatch(::triple_sum{}, ds_invalid, 10) == 0);
}

TEST_CASE("DispatchSet with 4-tuples", "[static_dispatch][tuples][arity-4]") {
    using DS = DispatchSet<int, T<1, 2, 3, 4>, T<5, 6, 7, 8>>;

    auto ds1 = DS(1, 2, 3, 4);
    REQUIRE(dispatch(::quad_sum{}, ds1, 100) == 110);

    auto ds2 = DS(5, 6, 7, 8);
    REQUIRE(dispatch(::quad_sum{}, ds2, 100) == 126);
}

TEST_CASE("DispatchSet with negative values", "[static_dispatch][tuples][negative]") {
    using DS = DispatchSet<int, T<-1, -2>, T<-5, -10>, T<0, 0>>;

    auto ds1 = DS(-1, -2);
    REQUIRE(dispatch(tuple_sum{}, ds1, 10) == 7);

    auto ds2 = DS(-5, -10);
    REQUIRE(dispatch(tuple_sum{}, ds2, 20) == 5);

    auto ds3 = DS(0, 0);
    REQUIRE(dispatch(tuple_sum{}, ds3, 5) == 5);
}

TEST_CASE("DispatchSet with mixed positive and negative values", "[static_dispatch][tuples][mixed]") {
    using DS = DispatchSet<int, T<-5, 10>, T<3, -7>, T<0, 0>>;

    auto ds1 = DS(-5, 10);
    REQUIRE(dispatch(tuple_sum{}, ds1, 100) == 105);

    auto ds2 = DS(3, -7);
    REQUIRE(dispatch(tuple_sum{}, ds2, 50) == 46);

    auto ds3 = DS(0, 0);
    REQUIRE(dispatch(tuple_sum{}, ds3, 10) == 10);
}

TEST_CASE("DispatchSet with single allowed tuple", "[static_dispatch][tuples][single]") {
    using DS = DispatchSet<int, T<42, 84>>;

    auto ds_valid = DS(42, 84);
    REQUIRE(dispatch(tuple_sum{}, ds_valid, 100) == 226);

    auto ds_invalid = DS(42, 85);
    REQUIRE(dispatch(tuple_sum{}, ds_invalid, 100) == 0);
}

TEST_CASE("DispatchSet with 3-tuples and throw_t", "[static_dispatch][tuples][arity-3][throw]") {
    using DS = DispatchSet<int, T<1, 2, 3>, T<4, 5, 6>>;

    auto ds_valid = DS(1, 2, 3);
    REQUIRE(dispatch(throw_t, ::triple_sum{}, ds_valid, 10) == 16);

    auto ds_invalid = DS(1, 2, 4);
    REQUIRE_THROWS_AS(dispatch(throw_t, ::triple_sum{}, ds_invalid, 10), std::runtime_error);
}

TEST_CASE("dispatch_tuples_impl matches correct tuple", "[static_dispatch][tuples][internal]") {
    using TL = std::tuple<std::integer_sequence<int, 1, 2>, std::integer_sequence<int, 3, 4>>;
    auto rt = std::make_tuple(3, 4);
    auto result = poet::detail::dispatch_tuples_impl<false>(tuple_sum{}, TL{}, rt, 10);
    REQUIRE(result == 17);
}

TEST_CASE("dispatch_tuples_impl returns default on no match", "[static_dispatch][tuples][internal]") {
    using TL = std::tuple<std::integer_sequence<int, 1, 2>>;
    auto rt = std::make_tuple(9, 9);
    auto result = poet::detail::dispatch_tuples_impl<false>(tuple_sum{}, TL{}, rt, 5);
    REQUIRE(result == 0);
}

TEST_CASE("dispatch_tuples_impl throws on no match with ThrowOnNoMatch", "[static_dispatch][tuples][internal]") {
    using TL = std::tuple<std::integer_sequence<int, 1, 2>>;
    auto rt = std::make_tuple(9, 9);
    REQUIRE_THROWS_AS(poet::detail::dispatch_tuples_impl<true>(tuple_sum{}, TL{}, rt, 5), std::runtime_error);
}

// ============================================================================
// Heavy tests: 1D array dispatch
// ============================================================================

TEST_CASE("dispatch fills array using runtime index (lambda)", "[static_dispatch][array]") {
    constexpr int N = 8;
    int arr[N] = {};
    using Seq = make_range<0, N - 1>;

    for (int i = 0; i < N; ++i) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ i });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) REQUIRE(arr[i] == i);
}

TEST_CASE("dispatch sets selected random indexes only", "[static_dispatch][array][random]") {
    constexpr int N = 16;
    constexpr int M = 5;
    int arr[N] = {};
    using Seq = make_range<0, N - 1>;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, N - 1);
    std::unordered_set<int> picks;
    while (picks.size() < static_cast<std::size_t>(M)) picks.insert(dist(rng));

    for (int idx : picks) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) {
        if (picks.count(i))
            REQUIRE(arr[i] == i);
        else
            REQUIRE(arr[i] == 0);
    }
}

TEST_CASE("dispatch loop over non-contiguous sequence", "[static_dispatch][array][non-contiguous]") {
    constexpr int N = 16;
    int arr[N] = {};
    using Seq = std::integer_sequence<int, 1, 3, 5, 12>;
    std::vector<int> indices = { 1, 3, 5, 12 };
    std::unordered_set<int> index_set(indices.begin(), indices.end());

    for (int idx : indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) {
        if (index_set.count(i))
            REQUIRE(arr[i] == i);
        else
            REQUIRE(arr[i] == 0);
    }
}

TEST_CASE("dispatch non-contiguous subset set", "[static_dispatch][array][non-contiguous][subset]") {
    constexpr int N = 16;
    int arr[N] = {};
    using Seq = std::integer_sequence<int, 1, 3, 5, 12>;
    std::vector<int> set_indices = { 3, 12 };
    std::unordered_set<int> set_index_set(set_indices.begin(), set_indices.end());

    for (int idx : set_indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) {
        if (set_index_set.count(i))
            REQUIRE(arr[i] == i);
        else
            REQUIRE(arr[i] == 0);
    }
}

// ============================================================================
// Heavy tests: N-D array dispatch
// ============================================================================

TEST_CASE("dispatch 2D sets selected random indexes only (lambda ND)", "[static_dispatch][array][random][nd]") {
    constexpr int N1 = 4;
    constexpr int N2 = 4;
    constexpr int M = 5;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = make_range<0, N1 - 1>;
    using Seq2 = make_range<0, N2 - 1>;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist1(0, N1 - 1);
    std::uniform_int_distribution<int> dist2(0, N2 - 1);
    std::unordered_set<int> picks;
    while (picks.size() < static_cast<std::size_t>(M)) picks.insert(dist1(rng) * 16 + dist2(rng));

    for (int key : picks) {
        const int x = key / 16;
        const int y = key % 16;
        auto setter = [&arr](auto I, auto J) {
            arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 100 + J;
        };
        auto params = std::make_tuple(DispatchParam<Seq1>{ x }, DispatchParam<Seq2>{ y });
        dispatch(setter, params);
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 16 + j;
            if (picks.count(key))
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
            else
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}

TEST_CASE("dispatch loop over non-contiguous sequences (lambda ND)", "[static_dispatch][array][non-contiguous][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    std::vector<int> indices1 = { 1, 3, 5, 12 };
    std::vector<int> indices2 = { 0, 2, 7 };
    std::unordered_set<int> index_set;
    for (int a : indices1)
        for (int b : indices2) index_set.insert(a * 256 + b);

    for (int a : indices1) {
        for (int b : indices2) {
            auto setter = [&arr](auto I, auto J) {
                arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J;
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ a }, DispatchParam<Seq2>{ b });
            dispatch(setter, params);
        }
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 256 + j;
            if (index_set.count(key))
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}

TEST_CASE("dispatch non-contiguous subset set (lambda ND)", "[static_dispatch][array][non-contiguous][subset][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    std::vector<std::pair<int, int>> set_pairs = { { 3, 2 }, { 12, 7 } };
    std::unordered_set<int> set_keys;
    for (const auto &p : set_pairs) set_keys.insert(p.first * 256 + p.second);

    for (const auto &p : set_pairs) {
        auto setter = [&arr](
                        auto I, auto J) { arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J; };
        auto params = std::make_tuple(DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
        dispatch(setter, params);
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 256 + j;
            if (set_keys.count(key))
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}

// ============================================================================
// Heavy tests: return value handling
// ============================================================================

TEST_CASE("dispatch ND lambda returns std::vector", "[static_dispatch][return][nd][vector]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> std::vector<int> { return std::vector<int>{ I, J }; };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            auto vec = dispatch(setter, params);
            REQUIRE(vec == std::vector<int>{ i, j });
        }
    }
}

TEST_CASE("dispatch ND lambda sets separate arrays and returns std::vector",
  "[static_dispatch][array][nd][vector][side-effect]") {
    constexpr int N1 = 4;
    constexpr int N2 = 4;
    std::array<std::array<int, N2>, N1> arrI{};
    std::array<std::array<int, N2>, N1> arrJ{};
    using Seq1 = make_range<0, N1 - 1>;
    using Seq2 = make_range<0, N2 - 1>;

    std::vector<std::pair<int, int>> picks = { { 1, 2 }, { 3, 0 } };
    std::unordered_set<int> pick_set;
    for (const auto &p : picks) pick_set.insert(p.first * 16 + p.second);

    for (const auto &p : picks) {
        auto setter = [&arrI, &arrJ](auto I, auto J) -> std::vector<int> {
            arrI[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I;
            arrJ[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = J;
            return std::vector<int>{ I, J };
        };
        auto params = std::make_tuple(DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
        auto vec = dispatch(setter, params);
        REQUIRE(vec == std::vector<int>{ p.first, p.second });
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 16 + j;
            if (pick_set.count(key)) {
                REQUIRE(arrI[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i);
                REQUIRE(arrJ[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == j);
            } else {
                REQUIRE(arrI[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
                REQUIRE(arrJ[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
            }
        }
    }
}

TEST_CASE("dispatch ND lambda returns NonTrivial (non-trivially copyable)",
  "[static_dispatch][return][nd][nontrivial]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    struct NonTrivial {
        std::vector<int> v;
        NonTrivial() = default;
        NonTrivial(int a, int b) : v{ a, b } {}
        NonTrivial(const NonTrivial &other) : v(other.v) {}
        bool operator==(const NonTrivial &o) const { return v == o.v; }
    };

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> NonTrivial { return NonTrivial(I, J); };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            auto nt = dispatch(setter, params);
            REQUIRE(nt == NonTrivial(i, j));
        }
    }
}

TEST_CASE("dispatch ND lambda returns move-only unique_ptr<vector>", "[static_dispatch][return][nd][move-only]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> std::unique_ptr<std::vector<int>> {
                return std::make_unique<std::vector<int>>(std::initializer_list<int>{ I, J });
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            auto p = dispatch(setter, params);
            REQUIRE(*p == std::vector<int>{ i, j });
        }
    }
}

TEST_CASE("dispatch ND lambda returns pointer (lvalue)", "[static_dispatch][return][nd][ptr]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    constexpr int N1 = 3;
    constexpr int N2 = 3;
    std::array<std::array<int, N2>, N1> arr{};
    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) { arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = 0; }
    }

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [&arr](auto I, auto J) -> int * {
                return &arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)];
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            int *p = dispatch(setter, params);
            *p = i * 100 + j;
            REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
        }
    }
}
