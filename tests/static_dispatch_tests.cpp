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

    REQUIRE(result == 88);// 100 + (-3) + (-9) + 0
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

    // This should compile and execute without errors
    dispatch(vector_dispatcher{ &values }, params, 0);

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 14);
}

TEST_CASE("dispatch handles multiple out-of-range parameters", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 10 }, DispatchParam<make_range<5, 7>>{ 15 });

    dispatch(vector_dispatcher{ &values }, params, 8);

    REQUIRE(values.empty());// Should not invoke because both params are out of range
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

    // Expect deterministic mapping (implementation uses first-match index)
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

TEST_CASE("dispatch preserves return value types correctly", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });
    const auto result = dispatch(return_type_dispatcher{}, params, 2.5);

    REQUIRE(result == 7.5);
    REQUIRE(std::is_same_v<decltype(result), const double>);
}

// Move-only return type: ensure dispatch can return move-only results
TEST_CASE("dispatch handles move-only return types", "[static_dispatch][move-only]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 1 });
    auto result = dispatch(::mover{}, params, 10);
    REQUIRE(result);
    REQUIRE(*result == 11);
}

// Forwarding test: pass a move-only argument through dispatch to the functor
TEST_CASE("dispatch forwards move-only arguments", "[static_dispatch][forwarding]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 1>>{ 1 });

    std::unique_ptr<int> p = std::make_unique<int>(7);
    auto result = dispatch(::receiver{}, params, std::move(p));
    REQUIRE(result == 8);
}

// Stress test: moderate-sized table to ensure dispatch table generation works
TEST_CASE("dispatch moderate table stress", "[static_dispatch][stress]") {
    // smallish product (8*8 = 64) to avoid template explosion in CI
    using A = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;
    using B = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;

    int out = 0;

    auto params = std::make_tuple(DispatchParam<A>{ 3 }, DispatchParam<B>{ 4 });
    dispatch(::probe{ &out }, params, 100);
    REQUIRE(out == 107);
}

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

TEST_CASE("dispatch handles contiguous descending sequence", "[static_dispatch][contiguous-descending]") {
    std::vector<int> values;
    using Desc = std::integer_sequence<int, 6, 5, 4, 3, 2, 1, 0>;
    for (int v : { 6, 5, 4, 3, 2, 1, 0 }) {
        auto params = std::make_tuple(DispatchParam<Desc>{ v }, DispatchParam<make_range<0, 0>>{ 0 });
        dispatch(vector_dispatcher{ &values }, params, 5);
    }
    REQUIRE(values == std::vector<int>{ 65, 55, 45, 35, 25, 15, 5 });
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
