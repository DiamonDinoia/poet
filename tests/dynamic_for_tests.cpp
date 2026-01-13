#include <poet/core/dynamic_for.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

// NOLINTBEGIN(bugprone-throwing-static-initialization, boost-use-ranges)

using poet::detail::kMaxStaticLoopBlock;

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
    // 10 to 5 exclusive (backward) -> 10, 9, 8, 7, 6
    constexpr int kStart = 10;
    constexpr int kEnd = 5;
    poet::dynamic_for<4>(kStart, kEnd, [&visited](int index) { visited.push_back(index); });

    std::vector<int> expected = { 10, 9, 8, 7, 6 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports negative ranges", "[dynamic_for]") {
    std::vector<int> visited;
    // -5 to 0 -> -5, -4, -3, -2, -1
    constexpr int kStart = -5;
    constexpr int kEnd = 0;
    poet::dynamic_for<2>(kStart, kEnd, [&visited](int index) { visited.push_back(index); });

    std::vector<int> expected = { -5, -4, -3, -2, -1 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports negative backward ranges", "[dynamic_for]") {
    std::vector<int> visited;
    // -2 to -6 backward -> -2, -3, -4, -5
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
    // Common type should be long
    poet::dynamic_for<2>(start, end, [&visited](auto index) {
        static_assert(std::is_same_v<decltype(index), long>);
        visited.push_back(index);
    });

    std::vector<long> expected = { 0, 1, 2, 3, 4 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports step > 1", "[dynamic_for]") {
    std::vector<int> visited;
    // 0 to 10 step 2 -> 0, 2, 4, 6, 8
    poet::dynamic_for<4>(0, 10, 2, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 0, 2, 4, 6, 8 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports step < -1", "[dynamic_for]") {
    std::vector<int> visited;
    // 10 to 0 step -2 -> 10, 8, 6, 4, 2
    poet::dynamic_for<4>(10, 0, -2, [&visited](int i) { visited.push_back(i); });
    std::vector<int> expected = { 10, 8, 6, 4, 2 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for step with non-divisible range", "[dynamic_for]") {
    std::vector<int> visited;
    // 0 to 9 step 2 -> 0, 2, 4, 6, 8
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

TEST_CASE("dynamic_for supports the maximum unroll factor", "[dynamic_for]") {
    constexpr std::size_t kMaxUnroll = kMaxStaticLoopBlock;
    std::vector<std::size_t> visited;
    const std::size_t begin = 11U;
    const std::size_t end = begin + kMaxUnroll;

    poet::dynamic_for<kMaxUnroll>(begin, end, [&visited](std::size_t index) -> void { visited.push_back(index); });

    std::vector<std::size_t> expected(kMaxUnroll);
    std::iota(expected.begin(), expected.end(), begin);

    REQUIRE(visited == expected);
}

// NOLINTEND(bugprone-throwing-static-initialization, boost-use-ranges)

TEST_CASE("dynamic_for block start pattern with Unroll=4 step=2", "[dynamic_for]") {
    std::vector<int> visited;

    // With Unroll=4 and step=2, blocks should start at 0, 8, 16, ...
    // Emit indices for range [0, 24) with step 2 -> indices: 0,2,...,22 (12 elements = 3 blocks)
    poet::dynamic_for<4>(0, 24, 2, [&visited](int i) { visited.push_back(i); });

    // Verify overall sequence
    std::vector<int> expected;
    for (int k = 0; k < 12; ++k) expected.push_back(k * 2);
    REQUIRE(visited == expected);

    // Verify block starts: visited[b*Unroll] == b * Unroll * step
    constexpr int Unroll = 4;
    constexpr int Step = 2;
    const std::size_t num_blocks = visited.size() / Unroll;
    for (std::size_t b = 0; b < num_blocks; ++b) {
        REQUIRE(visited[b * Unroll] == static_cast<int>(b * Unroll * Step));
    }
}

#if __cplusplus >= 202002L
#include <ranges>
#include <algorithm>
#include <tuple>

TEST_CASE("dynamic_for vs ranges (C++20)", "[dynamic_for][ranges][cpp20]") {
    std::vector<int> via_ranges;
    auto indices = std::views::iota(0) | std::views::take(10);
    std::ranges::for_each(indices, [&via_ranges](int i){ via_ranges.push_back(i); });

    std::vector<int> via_dynamic;
    poet::dynamic_for<4>(0, 10, 1, [&via_dynamic](int i){ via_dynamic.push_back(i); });

    REQUIRE(via_ranges == via_dynamic);

    // Also test piping the range directly into dynamic_for adaptor
    std::vector<int> via_pipe;
    indices | poet::make_dynamic_for<4>([&via_pipe](int i){ via_pipe.push_back(i); });
    REQUIRE(via_pipe == via_ranges);
}

TEST_CASE("dynamic_for with transformed ranges (C++20)", "[dynamic_for][ranges][cpp20]") {
    std::vector<int> via_ranges;
    auto r = std::views::iota(0)
             | std::views::transform([](int i){ return i * 2; })
             | std::views::take(5);
    std::ranges::for_each(r, [&via_ranges](int v){ via_ranges.push_back(v); });

    std::vector<int> via_dynamic;
    // dynamic_for producing same transformed values: 0,2,4,6,8
    poet::dynamic_for<4>(0, 10, 2, [&via_dynamic](int i){ via_dynamic.push_back(i); });

    REQUIRE(via_ranges == via_dynamic);

    // tuple pipeline
    std::vector<int> via_tuple;
    std::tuple{0, 10, 2} | poet::make_dynamic_for<4>([&via_tuple](int i){ via_tuple.push_back(i); });
    REQUIRE(via_tuple == via_dynamic);
}
#endif // __cplusplus >= 202002L
