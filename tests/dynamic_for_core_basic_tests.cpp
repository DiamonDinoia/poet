#include <catch2/catch_test_macros.hpp>

#include <numeric>
#include <type_traits>
#include <vector>


TEST_CASE("dynamic_for handles divisible counts", "[dynamic_for]") {
    std::vector<std::size_t> visited;
    constexpr std::size_t kCount = 16U;
    poet::dynamic_for<4>(0U, kCount, [&visited](std::size_t index) -> void { visited.push_back(index); });

    std::vector<std::size_t> expected(kCount);
    std::iota(expected.begin(), expected.end(), 0U);

    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for applies offsets and tails", "[dynamic_for]") {
    std::vector<std::size_t> visited;
    constexpr std::size_t kStart = 5U;
    constexpr std::size_t kEnd = 15U;
    poet::dynamic_for<4>(kStart, kEnd, [&visited](std::size_t index) -> void { visited.push_back(index); });

    std::vector<std::size_t> expected(10);
    std::iota(expected.begin(), expected.end(), kStart);

    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for skips zero-length ranges", "[dynamic_for]") {
    bool invoked = false;
    constexpr std::size_t kVal = 42U;
    poet::dynamic_for<8>(kVal, kVal, [&invoked](std::size_t) -> void { invoked = true; });

    REQUIRE_FALSE(invoked);
}

TEST_CASE("dynamic_for supports backward ranges", "[dynamic_for]") {
    std::vector<int> visited;
    constexpr int kStart = 10;
    constexpr int kEnd = 5;
    poet::dynamic_for<4>(kStart, kEnd, [&visited](int index) { visited.push_back(index); });

    std::vector<int> expected = { 10, 9, 8, 7, 6 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports negative ranges", "[dynamic_for]") {
    std::vector<int> visited;
    constexpr int kStart = -5;
    constexpr int kEnd = 0;
    poet::dynamic_for<2>(kStart, kEnd, [&visited](int index) { visited.push_back(index); });

    std::vector<int> expected = { -5, -4, -3, -2, -1 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports negative backward ranges", "[dynamic_for]") {
    std::vector<int> visited;
    constexpr int kStart = -2;
    constexpr int kEnd = -6;
    poet::dynamic_for<2>(kStart, kEnd, [&visited](int index) { visited.push_back(index); });

    std::vector<int> expected = { -2, -3, -4, -5 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for mixed types deduction", "[dynamic_for]") {
    std::vector<long> visited;
    int start = 0;
    long end = 5;
    poet::dynamic_for<2>(start, end, [&visited](auto index) {
        static_assert(std::is_same_v<decltype(index), long>);
        visited.push_back(index);
    });

    std::vector<long> expected = { 0, 1, 2, 3, 4 };
    REQUIRE(visited == expected);
}
