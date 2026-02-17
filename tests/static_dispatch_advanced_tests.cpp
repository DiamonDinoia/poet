#include <catch2/catch_test_macros.hpp>

#include "static_dispatch_test_support.hpp"

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
