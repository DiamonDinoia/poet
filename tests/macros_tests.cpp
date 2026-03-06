/// \file macros_tests.cpp
/// \brief Tests for POET compiler macros — compile-time validation and runtime smoke tests.

#include <poet/core/dynamic_for.hpp>
#include <poet/core/macros.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>

// ============================================================================
// POET_ALWAYS_INLINE_LAMBDA — compilation and runtime correctness
// ============================================================================

TEST_CASE("POET_ALWAYS_INLINE_LAMBDA compiles on value-returning lambda", "[macros]") {
    auto square = [](int x) POET_ALWAYS_INLINE_LAMBDA { return x * x; };
    REQUIRE(square(5) == 25);
}

TEST_CASE("POET_ALWAYS_INLINE_LAMBDA compiles on generic (auto) lambda", "[macros]") {
    auto identity = [](auto x) POET_ALWAYS_INLINE_LAMBDA { return x; };
    REQUIRE(identity(7) == 7);
    REQUIRE(identity(3.14) == 3.14);
}

TEST_CASE("POET_ALWAYS_INLINE_LAMBDA compiles on mutable noexcept lambda", "[macros]") {
    int counter = 0;
    auto inc = [counter]() mutable noexcept POET_ALWAYS_INLINE_LAMBDA {
        ++counter;
        return counter;
    };
    REQUIRE(inc() == 1);
    REQUIRE(inc() == 2);
}

TEST_CASE("POET_ALWAYS_INLINE_LAMBDA works with dynamic_for lane callback", "[macros]") {
    constexpr std::size_t N = 16;
    std::array<int, 4> accs{};
    auto kernel = [&accs](auto lane_c, std::size_t i) POET_ALWAYS_INLINE_LAMBDA {
        constexpr auto lane = decltype(lane_c)::value;
        accs[lane] += static_cast<int>(i);
    };
    poet::dynamic_for<4>(std::size_t{ 0 }, N, kernel);

    int total = 0;
    for (int a : accs) total += a;
    REQUIRE(total == 120);
}

// ============================================================================
// POET_ALWAYS_INLINE_LAMBDA — macro is defined
// ============================================================================

TEST_CASE("POET_ALWAYS_INLINE_LAMBDA is defined", "[macros]") {
#ifdef POET_ALWAYS_INLINE_LAMBDA
    constexpr bool defined = true;
#else
    constexpr bool defined = false;
#endif
    REQUIRE(defined);
}
