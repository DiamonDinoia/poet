#include <catch2/catch_test_macros.hpp>

#include "static_dispatch_test_support.hpp"

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
