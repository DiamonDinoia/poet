#include <catch2/catch_test_macros.hpp>

#include "static_dispatch_test_support.hpp"

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
// Stateful functor preserves state across dispatch calls
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
