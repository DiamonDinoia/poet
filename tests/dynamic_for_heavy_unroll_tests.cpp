#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

#include "dynamic_for_test_support.hpp"

TEST_CASE("dynamic_for supports the maximum unroll factor", "[dynamic_for][limits][heavy]") {
    constexpr std::size_t kMaxUnroll = kMaxDynamicForTestUnroll;
    std::vector<std::size_t> visited;
    const std::size_t begin = 11U;
    const std::size_t end = begin + kMaxUnroll;

    poet::dynamic_for<kMaxUnroll>(begin, end, [&visited](std::size_t index) -> void { visited.push_back(index); });

    std::vector<std::size_t> expected(kMaxUnroll);
    std::iota(expected.begin(), expected.end(), begin);

    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for with maximum safe unroll factor", "[dynamic_for][limits][heavy]") {
    std::size_t count = 0;
    constexpr std::size_t kMaxUnroll = kMaxDynamicForTestUnroll;
    poet::dynamic_for<kMaxUnroll>(0U, kMaxUnroll * 2, [&count](std::size_t) { ++count; });

    REQUIRE(count == kMaxUnroll * 2);
}
