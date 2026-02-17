#include <catch2/catch_test_macros.hpp>

#include <numeric>
#include <vector>

#include "dynamic_for_test_support.hpp"

TEST_CASE("dynamic_for supports step > 1", "[dynamic_for]") {
    std::vector<int> visited;
    poet::dynamic_for<4>(0, 10, 2, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 0, 2, 4, 6, 8 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports step < -1", "[dynamic_for]") {
    std::vector<int> visited;
    poet::dynamic_for<4>(10, 0, -2, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 10, 8, 6, 4, 2 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for step with non-divisible range", "[dynamic_for]") {
    std::vector<int> visited;
    poet::dynamic_for<4>(0, 9, 2, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 0, 2, 4, 6, 8 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for executes single-step loops", "[dynamic_for]") {
    std::vector<std::size_t> visited;

    const std::size_t begin = 3U;
    const std::size_t end = 9U;

    poet::dynamic_for<1>(begin, end, [&visited](std::size_t index) -> void { visited.push_back(index); });

    std::vector<std::size_t> expected(end - begin);
    std::iota(expected.begin(), expected.end(), begin);

    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for honours custom unroll factors", "[dynamic_for]") {
    std::vector<std::size_t> visited;
    constexpr std::size_t kCount = 12U;
    poet::dynamic_for<5>(kCount, [&visited](std::size_t index) -> void { visited.push_back(index * index); });

    std::vector<std::size_t> expected(kCount);
    for (std::size_t i = 0; i < expected.size(); ++i) { expected.at(i) = i * i; }

    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for auto-detects forward direction", "[dynamic_for][auto-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4>(5, 10, [&visited](int i) { visited.push_back(i); });

    REQUIRE(visited == std::vector<int>{ 5, 6, 7, 8, 9 });
}

TEST_CASE("dynamic_for auto-detects backward direction", "[dynamic_for][auto-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4>(10, 5, [&visited](int i) { visited.push_back(i); });

    REQUIRE(visited == std::vector<int>{ 10, 9, 8, 7, 6 });
}
