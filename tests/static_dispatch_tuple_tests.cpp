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

struct triple_sum {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X + Y + Z; }
};

struct quad_sum {
    template<int X, int Y, int Z, int W> int operator()(int base) const { return base + X + Y + Z + W; }
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

TEST_CASE("DispatchSet with throw_t succeeds on valid match", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 2>, T<3, 4>, T<5, 6>>;
    auto ds = DS(3, 4);

    const auto result = dispatch(throw_t, tuple_sum{}, ds, 10);
    REQUIRE(result == 17);  // 10 + 3 + 4
}

TEST_CASE("DispatchSet with throw_t handles multiple valid tuples", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 2>, T<3, 4>, T<5, 6>>;

    // Test each valid combination
    auto ds1 = DS(1, 2);
    REQUIRE(dispatch(throw_t, tuple_sum{}, ds1, 0) == 3);  // 0 + 1 + 2

    auto ds2 = DS(3, 4);
    REQUIRE(dispatch(throw_t, tuple_sum{}, ds2, 0) == 7);  // 0 + 3 + 4

    auto ds3 = DS(5, 6);
    REQUIRE(dispatch(throw_t, tuple_sum{}, ds3, 0) == 11);  // 0 + 5 + 6

    // Invalid combination
    auto ds_invalid = DS(2, 3);
    REQUIRE_THROWS_AS(dispatch(throw_t, tuple_sum{}, ds_invalid, 0), std::runtime_error);
}

TEST_CASE("DispatchSet with throw_t handles void return type", "[static_dispatch][tuples][throw]") {
    using DS = DispatchSet<int, T<1, 2>, T<3, 4>>;

    int result = 0;
    auto ds1 = DS(1, 2);
    dispatch(throw_t, tuple_voider{&result}, ds1, 100);
    REQUIRE(result == 103);  // 100 + 1 + 2

    auto ds2 = DS(3, 4);
    dispatch(throw_t, tuple_voider{&result}, ds2, 50);
    REQUIRE(result == 57);  // 50 + 3 + 4

    // Invalid
    auto ds_invalid = DS(2, 3);
    REQUIRE_THROWS_AS(dispatch(throw_t, tuple_voider{&result}, ds_invalid, 100), std::runtime_error);
}

TEST_CASE("DispatchSet with 3-tuples", "[static_dispatch][tuples][arity-3]") {
    using DS = DispatchSet<int, T<1, 2, 3>, T<4, 5, 6>, T<7, 8, 9>>;

    auto ds1 = DS(1, 2, 3);
    REQUIRE(dispatch(::triple_sum{}, ds1, 10) == 16);  // 10 + 1 + 2 + 3

    auto ds2 = DS(4, 5, 6);
    REQUIRE(dispatch(::triple_sum{}, ds2, 10) == 25);  // 10 + 4 + 5 + 6

    auto ds3 = DS(7, 8, 9);
    REQUIRE(dispatch(::triple_sum{}, ds3, 10) == 34);  // 10 + 7 + 8 + 9

    // Invalid combination
    auto ds_invalid = DS(1, 2, 4);
    REQUIRE(dispatch(::triple_sum{}, ds_invalid, 10) == 0);
}

TEST_CASE("DispatchSet with 4-tuples", "[static_dispatch][tuples][arity-4]") {
    using DS = DispatchSet<int, T<1, 2, 3, 4>, T<5, 6, 7, 8>>;

    auto ds1 = DS(1, 2, 3, 4);
    REQUIRE(dispatch(::quad_sum{}, ds1, 100) == 110);  // 100 + 1 + 2 + 3 + 4

    auto ds2 = DS(5, 6, 7, 8);
    REQUIRE(dispatch(::quad_sum{}, ds2, 100) == 126);  // 100 + 5 + 6 + 7 + 8
}

TEST_CASE("DispatchSet with negative values", "[static_dispatch][tuples][negative]") {
    using DS = DispatchSet<int, T<-1, -2>, T<-5, -10>, T<0, 0>>;

    auto ds1 = DS(-1, -2);
    REQUIRE(dispatch(tuple_sum{}, ds1, 10) == 7);  // 10 + (-1) + (-2)

    auto ds2 = DS(-5, -10);
    REQUIRE(dispatch(tuple_sum{}, ds2, 20) == 5);  // 20 + (-5) + (-10)

    auto ds3 = DS(0, 0);
    REQUIRE(dispatch(tuple_sum{}, ds3, 5) == 5);  // 5 + 0 + 0
}

TEST_CASE("DispatchSet with mixed positive and negative values", "[static_dispatch][tuples][mixed]") {
    using DS = DispatchSet<int, T<-5, 10>, T<3, -7>, T<0, 0>>;

    auto ds1 = DS(-5, 10);
    REQUIRE(dispatch(tuple_sum{}, ds1, 100) == 105);  // 100 + (-5) + 10

    auto ds2 = DS(3, -7);
    REQUIRE(dispatch(tuple_sum{}, ds2, 50) == 46);  // 50 + 3 + (-7)

    auto ds3 = DS(0, 0);
    REQUIRE(dispatch(tuple_sum{}, ds3, 10) == 10);
}

TEST_CASE("DispatchSet with single allowed tuple", "[static_dispatch][tuples][single]") {
    using DS = DispatchSet<int, T<42, 84>>;

    auto ds_valid = DS(42, 84);
    REQUIRE(dispatch(tuple_sum{}, ds_valid, 100) == 226);  // 100 + 42 + 84

    // Any other combination should fail
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
