#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>

#include <tuple>
#include <type_traits>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;

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

}// namespace

// Helper functors placed at namespace scope because local classes cannot
// declare member templates.
namespace {
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

}// namespace

// Reporter for duplicate-value dispatch tests. Must live at namespace scope.
namespace {
struct duplicate_reporter {
    int *out;
    template<int X> void operator()() const { *out = X; }
};
}// namespace

struct throwing_dispatcher {
    int* counter;
    template<int X>
    void operator()(int threshold) const {
        (*counter)++;
        if (X >= threshold) { throw std::runtime_error("dispatch exception"); }
    }
};

struct triple_dispatcher {
    template<int X, int Y, int Z>
    int operator()(int base) const { return base + X * 100 + Y * 10 + Z; }
};

struct value_arg_functor {
    template<int X, int Y>
    int operator()(std::integral_constant<int, X>, std::integral_constant<int, Y>, int base) const {
        return base + X * 10 + Y;
    }
};

TEST_CASE("dispatch routes to the matching template instantiation", "[static_dispatch]") {
    std::vector<int> values;

    dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<-2, 1>>{ -1 }, 5);

    REQUIRE(values == std::vector<int>{ 24 });
}

TEST_CASE("dispatch forwards runtime arguments to the functor", "[static_dispatch]") {
    const auto result = dispatch(sum_dispatcher{}, DispatchParam<make_range<0, 5>>{ 5 },
      DispatchParam<make_range<1, 3>>{ 2 },
      DispatchParam<make_range<-1, 1>>{ 0 }, 10);

    REQUIRE(result == 17);
}

TEST_CASE("dispatch returns default values when no match exists", "[static_dispatch]") {
    bool invoked = false;

    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 2>>{ 3 }, 8);

    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch handles single-element ranges", "[static_dispatch]") {
    bool invoked = false;

    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<make_range<5, 5>>{ 5 }, 10);

    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch handles boundary values in ranges", "[static_dispatch]") {
    std::vector<int> values;

    SECTION("minimum boundary") {
        dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<0, 3>>{ 0 }, DispatchParam<make_range<-2, 1>>{ -2 }, 5);
        REQUIRE(values == std::vector<int>{ 3 });
    }

    SECTION("maximum boundary") {
        dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<0, 3>>{ 3 }, DispatchParam<make_range<-2, 1>>{ 1 }, 5);
        REQUIRE(values == std::vector<int>{ 36 });
    }
}

TEST_CASE("dispatch handles all negative ranges", "[static_dispatch]") {
    const auto result = dispatch(sum_dispatcher{}, DispatchParam<make_range<-5, -2>>{ -3 },
      DispatchParam<make_range<-10, -8>>{ -9 },
      DispatchParam<make_range<-1, 0>>{ 0 }, 100);

    REQUIRE(result == 88);// 100 + (-3) + (-9) + 0
}

TEST_CASE("dispatch handles ranges crossing zero", "[static_dispatch]") {
    std::vector<int> values;

    dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<-3, 3>>{ 0 }, DispatchParam<make_range<-1, 1>>{ 0 }, 7);

    REQUIRE(values == std::vector<int>{ 7 });
}

TEST_CASE("dispatch handles void return type explicitly", "[static_dispatch]") {
    std::vector<int> values;

    // This should compile and execute without errors
    dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<1, 2>>{ 1 }, DispatchParam<make_range<3, 4>>{ 4 }, 0);

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 14);
}

TEST_CASE("dispatch handles multiple out-of-range parameters", "[static_dispatch]") {
    std::vector<int> values;

    dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<0, 2>>{ 10 }, DispatchParam<make_range<5, 7>>{ 15 }, 8);

    REQUIRE(values.empty());// Should not invoke because both params are out of range
}

TEST_CASE("dispatch handles non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NonContiguousSeq = std::integer_sequence<int, 1, 3, 7, 12>;

    std::vector<int> values;

    SECTION("matches first element") {
        dispatch(vector_dispatcher{ &values }, DispatchParam<NonContiguousSeq>{ 1 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 2 }, 10);
        REQUIRE(values == std::vector<int>{ 22 });
    }

    SECTION("matches middle element") {
        dispatch(vector_dispatcher{ &values }, DispatchParam<NonContiguousSeq>{ 7 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 5 }, 10);
        REQUIRE(values == std::vector<int>{ 85 });
    }

    SECTION("matches last element") {
        dispatch(vector_dispatcher{ &values }, DispatchParam<NonContiguousSeq>{ 12 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 2 }, 10);
        REQUIRE(values == std::vector<int>{ 132 });
    }

    SECTION("value between sequence elements fails") {
        bool invoked = false;

        const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<NonContiguousSeq>{ 5 }, 10);
        REQUIRE(result == 0);
        REQUIRE_FALSE(invoked);
    }
}

TEST_CASE("dispatch handles mixed contiguous and non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NonContiguousSeq = std::integer_sequence<int, 0, 10, 20>;

    std::vector<int> values;

    dispatch(vector_dispatcher{ &values }, DispatchParam<NonContiguousSeq>{ 10 }, DispatchParam<make_range<1, 3>>{ 2 }, 5);
    REQUIRE(values == std::vector<int>{ 107 });
}

TEST_CASE("dispatch deterministic with duplicate sequence values", "[static_dispatch][duplicates]") {
    int out = 0;
    using DupSeq = std::integer_sequence<int, 5, 7, 5>;

    dispatch(duplicate_reporter{ &out }, DispatchParam<DupSeq>{ 5 });

    // Expect deterministic mapping (implementation uses first-match index)
    REQUIRE(out == 5);
}

TEST_CASE("dispatch handles negative non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NegativeSeq = std::integer_sequence<int, -10, -5, 0, 7>;

    bool invoked = false;

    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<NegativeSeq>{ -5 }, 20);
    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch preserves return value types correctly", "[static_dispatch]") {
    const auto result = dispatch(return_type_dispatcher{}, DispatchParam<make_range<1, 5>>{ 3 }, 2.5);

    REQUIRE(result == 7.5);
    REQUIRE(std::is_same_v<decltype(result), const double>);
}

TEST_CASE("dispatch return type is double at runtime", "[static_dispatch][type]") {
    auto result = dispatch(return_type_dispatcher{}, DispatchParam<make_range<1, 5>>{ 3 }, 2.5);
    REQUIRE(typeid(result) == typeid(double));
}

// Move-only return type: ensure dispatch can return move-only results
TEST_CASE("dispatch handles move-only return types", "[static_dispatch][move-only]") {
    auto result = dispatch(::mover{}, DispatchParam<make_range<0, 2>>{ 1 }, 10);
    REQUIRE(result);
    REQUIRE(*result == 11);
}

// Forwarding test: pass a move-only argument through dispatch to the functor
TEST_CASE("dispatch forwards move-only arguments", "[static_dispatch][forwarding]") {
    std::unique_ptr<int> p = std::make_unique<int>(7);
    auto result = dispatch(::receiver{}, DispatchParam<make_range<1, 1>>{ 1 }, std::move(p));
    REQUIRE(result == 8);
}

// Stress test: moderate-sized table to ensure dispatch table generation works
TEST_CASE("dispatch moderate table stress", "[static_dispatch][stress]") {
    // smallish product (8*8 = 64) to avoid template explosion in CI
    using A = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;
    using B = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;

    int out = 0;

    dispatch(::probe{ &out }, DispatchParam<A>{ 3 }, DispatchParam<B>{ 4 }, 100);
    REQUIRE(out == 107);
}

TEST_CASE("dispatch throws when no match exists with throw_t (non-void)", "[static_dispatch][throw]") {
    bool invoked = false;

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 2>>{ 3 }, 8), std::runtime_error);

    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch throws when no match exists with throw_t (void)", "[static_dispatch][throw]") {
    std::vector<int> values;

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, vector_dispatcher{ &values }, DispatchParam<make_range<1, 2>>{ 3 }, DispatchParam<make_range<3, 4>>{ 4 }, 0), std::runtime_error);

    REQUIRE(values.empty());
}

TEST_CASE("dispatch with throw_t succeeds on valid match", "[static_dispatch][throw]") {
    bool invoked = false;

    // Should NOT throw when match exists
    const auto result = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 5>>{ 3 }, 100);

    REQUIRE(invoked);
    REQUIRE(result == 103);  // 100 + 3
}

TEST_CASE("dispatch with throw_t handles multiple parameters correctly", "[static_dispatch][throw]") {
    // Valid match
    const auto result = dispatch(poet::throw_t, sum_dispatcher{},
        DispatchParam<make_range<1, 3>>{ 2 },
        DispatchParam<make_range<5, 7>>{ 6 },
        DispatchParam<make_range<10, 12>>{ 11 }, 100);
    REQUIRE(result == 119);  // 100 + 2 + 6 + 11

    // Invalid match - first param out of range
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, sum_dispatcher{},
        DispatchParam<make_range<1, 3>>{ 0 },
        DispatchParam<make_range<5, 7>>{ 6 },
        DispatchParam<make_range<10, 12>>{ 11 }, 100), std::runtime_error);
}

TEST_CASE("dispatch with throw_t handles boundary values", "[static_dispatch][throw]") {
    bool invoked = false;

    // Boundary: minimum value in range
    const auto result1 = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<-10, -5>>{ -10 }, 50);
    REQUIRE(invoked);
    REQUIRE(result1 == 40);  // 50 + (-10)

    invoked = false;

    // Boundary: maximum value in range
    const auto result2 = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<-10, -5>>{ -5 }, 50);
    REQUIRE(invoked);
    REQUIRE(result2 == 45);  // 50 + (-5)

    // Boundary: just outside range
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<-10, -5>>{ -11 }, 50), std::runtime_error);

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, DispatchParam<make_range<-10, -5>>{ -4 }, 50), std::runtime_error);
}

TEST_CASE("dispatch with throw_t preserves return type deduction", "[static_dispatch][throw]") {
    const auto result = dispatch(poet::throw_t, return_type_dispatcher{}, DispatchParam<make_range<1, 5>>{ 3 }, 2.5);

    static_assert(std::is_same_v<decltype(result), const double>, "Return type should be double");
    REQUIRE(result == 7.5);  // 3 * 2.5
}

TEST_CASE("dispatch handles contiguous descending sequence", "[static_dispatch][contiguous-descending]") {
    std::vector<int> values;
    using Desc = std::integer_sequence<int, 6, 5, 4, 3, 2, 1, 0>;
    for (int v : { 6, 5, 4, 3, 2, 1, 0 }) {
        dispatch(vector_dispatcher{ &values }, DispatchParam<Desc>{ v }, DispatchParam<make_range<0, 0>>{ 0 }, 5);
    }
    REQUIRE(values == std::vector<int>{ 65, 55, 45, 35, 25, 15, 5 });
}

TEST_CASE("dispatch preserved non-throwing behavior with nothrow_on_no_match (non-void)",
  "[static_dispatch][nothrow]") {
    bool invoked = false;

    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<make_range<0, 2>>{ 3 }, 8);

    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch preserved non-throwing behavior with nothrow_on_no_match (void)", "[static_dispatch][nothrow]") {
    std::vector<int> values;

    REQUIRE_NOTHROW(dispatch(vector_dispatcher{ &values }, DispatchParam<make_range<1, 2>>{ 3 }, DispatchParam<make_range<3, 4>>{ 4 }, 0));

    REQUIRE(values.empty());
}
TEST_CASE("dispatch with single-value repeated sequence", "[static_dispatch][edge_case]") {
    // All values are the same - should still work deterministically
    using RepeatedSeq = std::integer_sequence<int, 5, 5, 5, 5>;
    bool invoked = false;

    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<RepeatedSeq>{ 5 }, 10);

    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch with value-argument form (integral_constant parameters)", "[static_dispatch][value-args]") {
    // Test functor that accepts integral_constant values as parameters
    const auto result = dispatch(::value_arg_functor{}, DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<0, 3>>{ 1 }, 5);

    REQUIRE(result == 26);  // 5 + 2*10 + 1
}

TEST_CASE("dispatch 3D (triple dispatch)", "[static_dispatch][3d]") {
    // Small ranges to avoid template explosion
    const auto result = dispatch(::triple_dispatcher{},
        DispatchParam<make_range<0, 2>>{ 1 },
        DispatchParam<make_range<0, 2>>{ 2 },
        DispatchParam<make_range<0, 2>>{ 0 }, 5);
    REQUIRE(result == 125);  // 5 + 1*100 + 2*10 + 0
}

TEST_CASE("dispatch exception safety", "[static_dispatch][exception]") {
    int count = 0;

    try {
        dispatch(::throwing_dispatcher{ &count }, DispatchParam<make_range<0, 5>>{ 3 }, 2);
        FAIL("Expected exception was not thrown");
    } catch (const std::runtime_error&) {
        // Expected
        REQUIRE(count == 1);  // Should have executed once before throwing
    }
}

TEST_CASE("dispatch with single-element non-contiguous sequence", "[static_dispatch][edge_case]") {
    using SingleSeq = std::integer_sequence<int, 42>;
    bool invoked = false;

    const auto result = dispatch(guard_dispatcher{ &invoked }, DispatchParam<SingleSeq>{ 42 }, 100);

    REQUIRE(result == 142);
    REQUIRE(invoked);

    // Test non-matching value
    invoked = false;
    const auto result2 = dispatch(guard_dispatcher{ &invoked }, DispatchParam<SingleSeq>{ 41 }, 100);

    REQUIRE(result2 == 0);
    REQUIRE_FALSE(invoked);
}
