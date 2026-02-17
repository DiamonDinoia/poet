#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <vector>

#include "dynamic_for_test_support.hpp"

TEST_CASE("dynamic_for supports unsigned backward iteration with wrapped step", "[dynamic_for]") {
    std::vector<unsigned> visited;
    constexpr unsigned kStart = 10U;
    constexpr unsigned kEnd = 5U;
    constexpr unsigned kStep = static_cast<unsigned>(-1);

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) {
        visited.push_back(index);
    });

    std::vector<unsigned> expected = { 10U, 9U, 8U, 7U, 6U };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports unsigned backward iteration with wrapped step -2", "[dynamic_for]") {
    std::vector<unsigned> visited;
    constexpr unsigned kStart = 20U;
    constexpr unsigned kEnd = 10U;
    constexpr unsigned kStep = static_cast<unsigned>(-2);

    poet::dynamic_for<4>(kStart, kEnd, kStep, [&visited](unsigned index) {
        visited.push_back(index);
    });

    std::vector<unsigned> expected = { 20U, 18U, 16U, 14U, 12U };
    REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for unsigned backward iteration handles empty range", "[dynamic_for]") {
    std::vector<unsigned> visited;
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
    constexpr std::size_t kLargeCount = 1000000U;

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

    constexpr std::size_t expected = (kEnd - kStart + kStep - 1) / kStep;
    REQUIRE(count == expected);
}

TEST_CASE("dynamic_for handles single iteration edge case", "[dynamic_for][edge_case]") {
    std::vector<int> visited;
    poet::dynamic_for<8>(5, 6, [&visited](int index) {
        visited.push_back(index);
    });

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
