#include <catch2/catch_test_macros.hpp>

#include <numeric>
#include <vector>


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

// ============================================================================
// Compile-time step overload: dynamic_for<Unroll, Step>(begin, end, func)
// ============================================================================

TEST_CASE("dynamic_for with compile-time step +2", "[dynamic_for][ct-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4, 2>(0, 20, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for with compile-time step -1", "[dynamic_for][ct-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4, -1>(10, 0, [&visited](int i) { visited.push_back(i); });
    REQUIRE(visited == std::vector<int>{ 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 });
}

TEST_CASE("dynamic_for with compile-time step +3 non-divisible", "[dynamic_for][ct-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4, 3>(0, 15, [&visited](int i) { visited.push_back(i); });
    REQUIRE(visited == std::vector<int>{ 0, 3, 6, 9, 12 });
}

TEST_CASE("dynamic_for with compile-time step Unroll=1", "[dynamic_for][ct-step]") {
    std::vector<int> visited;
    poet::dynamic_for<1, 5>(0, 25, [&visited](int i) { visited.push_back(i); });
    REQUIRE(visited == std::vector<int>{ 0, 5, 10, 15, 20 });
}

TEST_CASE("dynamic_for with compile-time step lane form", "[dynamic_for][ct-step]") {
    std::vector<std::pair<std::size_t, int>> visited;
    poet::dynamic_for<4, 2>(
      0, 12, [&visited](auto lane_c, int i) { visited.emplace_back(decltype(lane_c)::value, i); });
    // 6 iterations: 0,2,4,6,8,10 — one full block of 4 + tail of 2
    REQUIRE(visited.size() == 6);
    // Verify all indices are correct
    for (std::size_t j = 0; j < visited.size(); ++j) { REQUIRE(visited[j].second == static_cast<int>(j * 2)); }
}

TEST_CASE("dynamic_for with compile-time step -2", "[dynamic_for][ct-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4, -2>(20, 0, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 20, 18, 16, 14, 12, 10, 8, 6, 4, 2 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for with compile-time step -3 non-divisible", "[dynamic_for][ct-step]") {
    std::vector<int> visited;
    poet::dynamic_for<4, -3>(15, 0, [&visited](int i) { visited.push_back(i); });
    REQUIRE(visited == std::vector<int>{ 15, 12, 9, 6, 3 });
}

TEST_CASE("dynamic_for tail dispatch completeness for Unroll=4", "[dynamic_for][tail]") {
    constexpr std::size_t kUnroll = 4;
    for (std::size_t remainder = 0; remainder < kUnroll; ++remainder) {
        std::vector<std::size_t> visited;
        std::size_t total = kUnroll + remainder;
        poet::dynamic_for<kUnroll>(std::size_t{ 0 }, total, [&visited](std::size_t i) { visited.push_back(i); });
        REQUIRE(visited.size() == total);
        for (std::size_t i = 0; i < total; ++i) { REQUIRE(visited[i] == i); }
    }
}

TEST_CASE("dynamic_for tail dispatch completeness for Unroll=16", "[dynamic_for][tail]") {
    constexpr std::size_t kUnroll = 16;
    for (std::size_t remainder = 0; remainder < kUnroll; ++remainder) {
        std::vector<std::size_t> visited;
        std::size_t total = kUnroll + remainder;
        poet::dynamic_for<kUnroll>(std::size_t{ 0 }, total, [&visited](std::size_t i) { visited.push_back(i); });
        REQUIRE(visited.size() == total);
        for (std::size_t i = 0; i < total; ++i) { REQUIRE(visited[i] == i); }
    }
}
