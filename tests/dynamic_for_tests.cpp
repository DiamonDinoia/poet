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

TEST_CASE("dynamic_for supports unsigned backward iteration with wrapped step", "[dynamic_for]") {
    std::vector<unsigned> visited;
    // Test unsigned backward iteration using wrapped negative step
    // unsigned(-1) wraps to UINT_MAX, representing step of -1
    constexpr unsigned kStart = 10U;
    constexpr unsigned kEnd = 5U;
    constexpr unsigned kStep = static_cast<unsigned>(-1);  // Wrapped negative step

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) {
        visited.push_back(index);
    });

    std::vector<unsigned> expected = { 10U, 9U, 8U, 7U, 6U };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports unsigned backward iteration with wrapped step -2", "[dynamic_for]") {
    std::vector<unsigned> visited;
    // Test unsigned backward iteration with step of -2 (wrapped)
    constexpr unsigned kStart = 20U;
    constexpr unsigned kEnd = 10U;
    constexpr unsigned kStep = static_cast<unsigned>(-2);  // Wrapped negative step

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) {
        visited.push_back(index);
    });

    std::vector<unsigned> expected = { 20U, 18U, 16U, 14U, 12U };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for unsigned backward iteration handles empty range", "[dynamic_for]") {
    std::vector<unsigned> visited;
    // When start <= end with negative step, should be empty
    constexpr unsigned kStart = 5U;
    constexpr unsigned kEnd = 10U;
    constexpr unsigned kStep = static_cast<unsigned>(-1);

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) {
        visited.push_back(index);
    });

    REQUIRE(visited.empty());
}

TEST_CASE("dynamic_for handles step==0 gracefully", "[dynamic_for][edge_case]") {
    std::vector<int> visited;
    // step==0 should result in no iterations
    poet::dynamic_for<4>(0, 10, 0, [&visited](int index) {
        visited.push_back(index);
    });

    REQUIRE(visited.empty());
}

TEST_CASE("dynamic_for handles step==0 with signed types", "[dynamic_for][edge_case]") {
    bool invoked = false;
    poet::dynamic_for<8>(-5, 5, 0, [&invoked](int) {
        invoked = true;
    });

    REQUIRE_FALSE(invoked);
}

TEST_CASE("dynamic_for handles step==0 with unsigned types", "[dynamic_for][edge_case]") {
    std::vector<std::size_t> visited;
    poet::dynamic_for<4>(0U, 100U, 0U, [&visited](std::size_t index) {
        visited.push_back(index);
    });

    REQUIRE(visited.empty());
}

TEST_CASE("dynamic_for handles large iteration counts", "[dynamic_for][stress]") {
    std::size_t count = 0;
    constexpr std::size_t kLargeCount = 1000000U;  // 1 million iterations

    poet::dynamic_for<8>(0U, kLargeCount, [&count](std::size_t) {
        ++count;
    });

    REQUIRE(count == kLargeCount);
}

TEST_CASE("dynamic_for handles large counts with custom step", "[dynamic_for][stress]") {
    std::size_t count = 0;
    constexpr std::size_t kStart = 0U;
    constexpr std::size_t kEnd = 500000U;
    constexpr std::size_t kStep = 7U;

    poet::dynamic_for<16>(kStart, kEnd, kStep, [&count](std::size_t) {
        ++count;
    });

    // Expected count: ceil(500000 / 7) = 71429
    constexpr std::size_t expected = (kEnd - kStart + kStep - 1) / kStep;
    REQUIRE(count == expected);
}

TEST_CASE("dynamic_for handles single iteration edge case", "[dynamic_for][edge_case]") {
    std::vector<int> visited;
    // Range that produces exactly 1 iteration
    poet::dynamic_for<8>(5, 6, [&visited](int index) {
        visited.push_back(index);
    });

    REQUIRE(visited.size() == 1);
    REQUIRE(visited[0] == 5);
}

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

TEST_CASE("dynamic_for with Unroll=1 comprehensive", "[dynamic_for][unroll-1]") {
    std::vector<int> visited;
    // Unroll=1 should still work correctly
    poet::dynamic_for<1>(0, 10, [&visited](int i) { visited.push_back(i); });

    std::vector<int> expected(10);
    std::iota(expected.begin(), expected.end(), 0);
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for with Unroll=1 and step > 1", "[dynamic_for][unroll-1]") {
    std::vector<int> visited;
    poet::dynamic_for<1>(0, 20, 3, [&visited](int i) { visited.push_back(i); });

    std::vector<int> expected = { 0, 3, 6, 9, 12, 15, 18 };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for tail dispatch completeness", "[dynamic_for][tail]") {
    // Test all possible remainders for Unroll=8
    constexpr std::size_t kUnroll = 8;
    for (std::size_t remainder = 0; remainder < kUnroll; ++remainder) {
        std::vector<std::size_t> visited;
        // Create a range that produces exactly 'remainder' tail iterations
        std::size_t total = kUnroll * 2 + remainder;  // 2 full blocks + remainder
        poet::dynamic_for<kUnroll>(0U, total, [&visited](std::size_t i) { visited.push_back(i); });

        REQUIRE(visited.size() == total);
        for (std::size_t i = 0; i < total; ++i) {
            REQUIRE(visited[i] == i);
        }
    }
}

TEST_CASE("dynamic_for nested loops", "[dynamic_for][nested]") {
    std::vector<std::pair<int, int>> pairs;
    poet::dynamic_for<4>(0, 5, [&pairs](int i) {
        poet::dynamic_for<4>(0, 5, [&pairs, i](int j) {
            pairs.push_back({ i, j });
        });
    });

    REQUIRE(pairs.size() == 25);
    REQUIRE(pairs[0] == std::make_pair(0, 0));
    REQUIRE(pairs[12] == std::make_pair(2, 2));
    REQUIRE(pairs[24] == std::make_pair(4, 4));
}

TEST_CASE("dynamic_for with stateful functor", "[dynamic_for][stateful]") {
    struct accumulator {
        int sum = 0;
        void operator()(int i) { sum += i; }
    };

    accumulator acc;
    poet::dynamic_for<4>(0, 10, std::ref(acc));

    // Sum of 0..9 = 45
    REQUIRE(acc.sum == 45);
}

TEST_CASE("dynamic_for exception safety", "[dynamic_for][exception]") {
    int count = 0;
    auto throwing_func = [&count](int i) {
        count++;
        if (i == 5) { throw std::runtime_error("test exception"); }
    };

    REQUIRE_THROWS_AS(poet::dynamic_for<4>(0, 10, throwing_func), std::runtime_error);
    // Should have executed iterations 0-5 before throwing
    REQUIRE(count == 6);
}

TEST_CASE("dynamic_for with maximum safe unroll factor", "[dynamic_for][limits]") {
    std::size_t count = 0;
    // Test at the maximum allowed unroll factor
    constexpr std::size_t kMaxUnroll = poet::detail::kMaxStaticLoopBlock;
    poet::dynamic_for<kMaxUnroll>(0U, kMaxUnroll * 2, [&count](std::size_t) { ++count; });

    REQUIRE(count == kMaxUnroll * 2);
}

TEST_CASE("dynamic_for auto-detects forward direction", "[dynamic_for][auto-step]") {
    std::vector<int> visited;
    // When begin < end, step should be +1
    poet::dynamic_for<4>(5, 10, [&visited](int i) { visited.push_back(i); });

    REQUIRE(visited == std::vector<int>{ 5, 6, 7, 8, 9 });
}

TEST_CASE("dynamic_for auto-detects backward direction", "[dynamic_for][auto-step]") {
    std::vector<int> visited;
    // When begin > end, step should be -1
    poet::dynamic_for<4>(10, 5, [&visited](int i) { visited.push_back(i); });

    REQUIRE(visited == std::vector<int>{ 10, 9, 8, 7, 6 });
}
