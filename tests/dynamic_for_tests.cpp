#include <poet/poet.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// ============================================================================
// Core basic tests
// ============================================================================

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

// ============================================================================
// Step tests
// ============================================================================

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

// ============================================================================
// Edge and stress tests
// ============================================================================

TEST_CASE("dynamic_for supports unsigned backward iteration with wrapped step", "[dynamic_for]") {
    std::vector<unsigned> visited;
    constexpr unsigned kStart = 10U;
    constexpr unsigned kEnd = 5U;
    constexpr unsigned kStep = static_cast<unsigned>(-1);

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) { visited.push_back(index); });

    std::vector<unsigned> expected = { 10U, 9U, 8U, 7U, 6U };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports unsigned backward iteration with wrapped step -2", "[dynamic_for]") {
    std::vector<unsigned> visited;
    constexpr unsigned kStart = 20U;
    constexpr unsigned kEnd = 10U;
    constexpr unsigned kStep = static_cast<unsigned>(-2);

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) { visited.push_back(index); });

    std::vector<unsigned> expected = { 20U, 18U, 16U, 14U, 12U };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for unsigned backward iteration handles empty range", "[dynamic_for]") {
    std::vector<unsigned> visited;
    constexpr unsigned kStart = 5U;
    constexpr unsigned kEnd = 10U;
    constexpr unsigned kStep = static_cast<unsigned>(-1);

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) { visited.push_back(index); });

    REQUIRE(visited.empty());
}

TEST_CASE("dynamic_for handles step==0 gracefully", "[dynamic_for][edge_case]") {
    std::vector<int> visited;
    poet::dynamic_for<4>(0, 10, 0, [&visited](int index) { visited.push_back(index); });

    REQUIRE(visited.empty());
}

TEST_CASE("dynamic_for handles step==0 with signed types", "[dynamic_for][edge_case]") {
    bool invoked = false;
    poet::dynamic_for<8>(-5, 5, 0, [&invoked](int) { invoked = true; });

    REQUIRE_FALSE(invoked);
}

TEST_CASE("dynamic_for handles step==0 with unsigned types", "[dynamic_for][edge_case]") {
    std::vector<std::size_t> visited;
    poet::dynamic_for<4>(0U, 100U, 0U, [&visited](std::size_t index) { visited.push_back(index); });

    REQUIRE(visited.empty());
}

TEST_CASE("dynamic_for handles large iteration counts", "[dynamic_for][stress]") {
    std::size_t count = 0;
    constexpr std::size_t kLargeCount = 1000000U;

    poet::dynamic_for<8>(0U, kLargeCount, [&count](std::size_t) { ++count; });

    REQUIRE(count == kLargeCount);
}

TEST_CASE("dynamic_for handles large counts with custom step", "[dynamic_for][stress]") {
    std::size_t count = 0;
    constexpr std::size_t kStart = 0U;
    constexpr std::size_t kEnd = 500000U;
    constexpr std::size_t kStep = 7U;

    poet::dynamic_for<16>(kStart, kEnd, kStep, [&count](std::size_t) { ++count; });

    constexpr std::size_t expected = (kEnd - kStart + kStep - 1) / kStep;
    REQUIRE(count == expected);
}

TEST_CASE("dynamic_for handles single iteration edge case", "[dynamic_for][edge_case]") {
    std::vector<int> visited;
    poet::dynamic_for<8>(5, 6, [&visited](int index) { visited.push_back(index); });

    REQUIRE(visited.size() == 1);
    REQUIRE(visited[0] == 5);
}

TEST_CASE("dynamic_for block start pattern with Unroll=4 step=2", "[dynamic_for]") {
    std::vector<int> visited;

    poet::dynamic_for<4>(0, 24, 2, [&visited](int i) { visited.push_back(i); });

    std::vector<int> expected;
    for (int k = 0; k < 12; ++k) expected.push_back(k * 2);
    REQUIRE(visited == expected);

    constexpr int Unroll = 4;
    constexpr int Step = 2;
    const std::size_t num_blocks = visited.size() / Unroll;
    for (std::size_t b = 0; b < num_blocks; ++b) {
        REQUIRE(visited[b * Unroll] == static_cast<int>(b * Unroll * Step));
    }
}

TEST_CASE("dynamic_for power-of-2 stride uses shift optimization", "[dynamic_for][stride]") {
    for (std::size_t stride :
      { std::size_t{ 1 }, std::size_t{ 2 }, std::size_t{ 4 }, std::size_t{ 8 }, std::size_t{ 16 } }) {
        std::size_t count = 0;
        constexpr std::size_t kEnd = 128U;
        poet::dynamic_for<4>(std::size_t{ 0 }, kEnd, stride, [&count](std::size_t) { ++count; });
        REQUIRE(count == (kEnd + stride - 1) / stride);
    }
}

TEST_CASE("dynamic_for non-power-of-2 strides", "[dynamic_for][stride]") {
    for (std::size_t stride : { std::size_t{ 3 }, std::size_t{ 5 }, std::size_t{ 7 }, std::size_t{ 11 } }) {
        std::vector<std::size_t> visited;
        constexpr std::size_t kEnd = 50U;
        poet::dynamic_for<4>(std::size_t{ 0 }, kEnd, stride, [&visited](std::size_t i) { visited.push_back(i); });

        std::vector<std::size_t> expected;
        for (std::size_t i = 0; i < kEnd; i += stride) expected.push_back(i);
        REQUIRE(visited == expected);
    }
}

TEST_CASE("dynamic_for tiny range bypasses main loop", "[dynamic_for][tiny]") {
    // count < Unroll should go straight to tail dispatch
    constexpr std::size_t kUnroll = 8;
    for (std::size_t total = 1; total < kUnroll; ++total) {
        std::vector<std::size_t> visited;
        poet::dynamic_for<kUnroll>(std::size_t{ 0 }, total, [&visited](std::size_t i) { visited.push_back(i); });
        REQUIRE(visited.size() == total);
        for (std::size_t i = 0; i < total; ++i) { REQUIRE(visited[i] == i); }
    }
}

// ============================================================================
// Advanced tests
// ============================================================================

TEST_CASE("dynamic_for with Unroll=1 comprehensive", "[dynamic_for][unroll-1]") {
    std::vector<int> visited;
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
    constexpr std::size_t kUnroll = 8;
    for (std::size_t remainder = 0; remainder < kUnroll; ++remainder) {
        std::vector<std::size_t> visited;
        std::size_t total = kUnroll * 2 + remainder;
        poet::dynamic_for<kUnroll>(0U, total, [&visited](std::size_t i) { visited.push_back(i); });

        REQUIRE(visited.size() == total);
        for (std::size_t i = 0; i < total; ++i) { REQUIRE(visited[i] == i); }
    }
}

TEST_CASE("dynamic_for passes compile-time lane in tiny and tail ranges", "[dynamic_for][lane][tail]") {
    constexpr std::size_t kUnroll = 8;
    std::vector<std::pair<std::size_t, std::size_t>> tiny;
    poet::dynamic_for<kUnroll>(
      5U, 8U, [&tiny](auto lane_c, std::size_t i) { tiny.emplace_back(i, decltype(lane_c)::value); });

    REQUIRE(tiny == std::vector<std::pair<std::size_t, std::size_t>>{ { 5U, 0U }, { 6U, 1U }, { 7U, 2U } });

    std::vector<std::pair<std::size_t, std::size_t>> with_tail;
    poet::dynamic_for<kUnroll>(
      5U, 16U, [&with_tail](auto lane_c, std::size_t i) { with_tail.emplace_back(i, decltype(lane_c)::value); });

    REQUIRE(with_tail.size() == 11);
    for (std::size_t iter = 0; iter < with_tail.size(); ++iter) {
        REQUIRE(with_tail[iter].first == 5U + iter);
        REQUIRE(with_tail[iter].second == iter % kUnroll);
    }
}

TEST_CASE("dynamic_for lane works with count and auto-step overloads", "[dynamic_for][lane]") {
    std::vector<std::pair<std::size_t, std::size_t>> by_count;
    poet::dynamic_for<4>(
      6U, [&by_count](auto lane_c, std::size_t i) { by_count.emplace_back(i, decltype(lane_c)::value); });

    REQUIRE(by_count.size() == 6);
    for (std::size_t iter = 0; iter < by_count.size(); ++iter) {
        REQUIRE(by_count[iter].first == iter);
        REQUIRE(by_count[iter].second == iter % 4U);
    }

    std::vector<std::pair<int, std::size_t>> auto_step_backward;
    poet::dynamic_for<4>(3, -2, [&auto_step_backward](auto lane_c, int i) {
        auto_step_backward.emplace_back(i, decltype(lane_c)::value);
    });

    const std::vector<int> expected = { 3, 2, 1, 0, -1 };
    REQUIRE(auto_step_backward.size() == expected.size());
    for (std::size_t iter = 0; iter < expected.size(); ++iter) {
        REQUIRE(auto_step_backward[iter].first == expected[iter]);
        REQUIRE(auto_step_backward[iter].second == iter % 4U);
    }
}

TEST_CASE("dynamic_for nested loops", "[dynamic_for][nested]") {
    std::vector<std::pair<int, int>> pairs;
    poet::dynamic_for<4>(
      0, 5, [&pairs](int i) { poet::dynamic_for<4>(0, 5, [&pairs, i](int j) { pairs.push_back({ i, j }); }); });

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

    REQUIRE(acc.sum == 45);
}

TEST_CASE("dynamic_for exception safety", "[dynamic_for][exception]") {
    int count = 0;
    auto throwing_func = [&count](int i) {
        count++;
        if (i == 5) { throw std::runtime_error("test exception"); }
    };

    REQUIRE_THROWS_AS(poet::dynamic_for<4>(0, 10, throwing_func), std::runtime_error);
    REQUIRE(count == 6);
}

TEST_CASE("dynamic_for lane form with various unroll factors", "[dynamic_for][lane]") {
    // Unroll=2
    {
        std::vector<std::pair<std::size_t, std::size_t>> visited;
        poet::dynamic_for<2>(std::size_t{ 0 }, std::size_t{ 5 }, [&visited](auto lane_c, std::size_t i) {
            visited.emplace_back(decltype(lane_c)::value, i);
        });
        REQUIRE(visited.size() == 5);
        for (std::size_t i = 0; i < visited.size(); ++i) {
            REQUIRE(visited[i].first == i % 2);
            REQUIRE(visited[i].second == i);
        }
    }
    // Unroll=3
    {
        std::vector<std::pair<std::size_t, std::size_t>> visited;
        poet::dynamic_for<3>(std::size_t{ 0 }, std::size_t{ 7 }, [&visited](auto lane_c, std::size_t i) {
            visited.emplace_back(decltype(lane_c)::value, i);
        });
        REQUIRE(visited.size() == 7);
        for (std::size_t i = 0; i < visited.size(); ++i) {
            REQUIRE(visited[i].first == i % 3);
            REQUIRE(visited[i].second == i);
        }
    }
    // Unroll=16
    {
        std::vector<std::pair<std::size_t, std::size_t>> visited;
        poet::dynamic_for<16>(std::size_t{ 0 }, std::size_t{ 20 }, [&visited](auto lane_c, std::size_t i) {
            visited.emplace_back(decltype(lane_c)::value, i);
        });
        REQUIRE(visited.size() == 20);
        for (std::size_t i = 0; i < visited.size(); ++i) {
            REQUIRE(visited[i].first == i % 16);
            REQUIRE(visited[i].second == i);
        }
    }
}

// ============================================================================
// Ranges tests (C++20)
// ============================================================================

#if __cplusplus >= 202002L

#include <ranges>
#include <tuple>

TEST_CASE("dynamic_for vs ranges (C++20)", "[dynamic_for][ranges][cpp20]") {
    std::vector<int> via_ranges;
    auto indices = std::views::iota(0) | std::views::take(10);
    std::ranges::for_each(indices, [&via_ranges](int i) { via_ranges.push_back(i); });

    std::vector<int> via_dynamic;
    poet::dynamic_for<4>(0, 10, 1, [&via_dynamic](int i) { via_dynamic.push_back(i); });

    REQUIRE(via_ranges == via_dynamic);

    std::vector<int> via_pipe;
    indices | poet::make_dynamic_for<4>([&via_pipe](int i) { via_pipe.push_back(i); });
    REQUIRE(via_pipe == via_ranges);
}

TEST_CASE("dynamic_for with transformed ranges (C++20)", "[dynamic_for][ranges][cpp20]") {
    std::vector<int> via_ranges;
    auto r = std::views::iota(0) | std::views::transform([](int i) { return i * 2; }) | std::views::take(5);
    std::ranges::for_each(r, [&via_ranges](int v) { via_ranges.push_back(v); });

    std::vector<int> via_dynamic;
    poet::dynamic_for<4>(0, 10, 2, [&via_dynamic](int i) { via_dynamic.push_back(i); });

    REQUIRE(via_ranges == via_dynamic);

    std::vector<int> via_tuple;
    std::tuple{ 0, 10, 2 } | poet::make_dynamic_for<4>([&via_tuple](int i) { via_tuple.push_back(i); });
    REQUIRE(via_tuple == via_dynamic);
}

TEST_CASE("dynamic_for lane works with range and tuple adaptors (C++20)", "[dynamic_for][ranges][cpp20][lane]") {
    std::vector<std::pair<int, std::size_t>> via_range_pipe;
    auto indices = std::views::iota(0) | std::views::take(10);
    indices | poet::make_dynamic_for<4>([&via_range_pipe](auto lane_c, int i) {
        via_range_pipe.emplace_back(i, decltype(lane_c)::value);
    });

    REQUIRE(via_range_pipe.size() == 10);
    for (std::size_t iter = 0; iter < via_range_pipe.size(); ++iter) {
        REQUIRE(via_range_pipe[iter].first == static_cast<int>(iter));
        REQUIRE(via_range_pipe[iter].second == iter % 4U);
    }

    std::vector<std::pair<int, std::size_t>> via_tuple_pipe;
    std::tuple{ 0, 10, 2 } | poet::make_dynamic_for<4>([&via_tuple_pipe](auto lane_c, int i) {
        via_tuple_pipe.emplace_back(i, decltype(lane_c)::value);
    });

    const std::vector<int> expected_indices = { 0, 2, 4, 6, 8 };
    REQUIRE(via_tuple_pipe.size() == expected_indices.size());
    for (std::size_t iter = 0; iter < via_tuple_pipe.size(); ++iter) {
        REQUIRE(via_tuple_pipe[iter].first == expected_indices[iter]);
        REQUIRE(via_tuple_pipe[iter].second == iter % 4U);
    }
}

#endif// __cplusplus >= 202002L
