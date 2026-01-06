// cppcheck-suppress-file unknownMacro
#include <poet/core/static_dispatch.hpp>


#include <catch2/catch_test_macros.hpp>

#include <tuple>

namespace {
using poet::DispatchSet;
using poet::dispatch;
using poet::throw_t;
using poet::T;

struct tuple_sum {
    template<int X, int Y> int operator()(int base) const { return base + X + Y; }
};

struct tuple_voider {
    int *out;
    template<int X, int Y> void operator()(int add) const { *out = add + X + Y; }
};

}// namespace

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
