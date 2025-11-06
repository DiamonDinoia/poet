#include <poet/core/dynamic_for.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

TEST_CASE("dynamic_for handles divisible counts", "[dynamic_for]") {
  std::vector<std::size_t> visited;
  poet::dynamic_for<4>(0U, 16U, [&visited](std::size_t index) {
    visited.push_back(index);
  });

  std::vector<std::size_t> expected(16);
  std::iota(expected.begin(), expected.end(), 0U);

  REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for applies offsets and tails", "[dynamic_for]") {
  std::vector<std::size_t> visited;
  poet::dynamic_for<4>(5U, 15U, [&visited](std::size_t index) {
    visited.push_back(index);
  });

  std::vector<std::size_t> expected(10);
  std::iota(expected.begin(), expected.end(), 5U);

  REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for skips zero-length ranges", "[dynamic_for]") {
  bool invoked = false;
  poet::dynamic_for<8>(42U, 42U, [&invoked](std::size_t) { invoked = true; });

  REQUIRE_FALSE(invoked);
}

TEST_CASE("dynamic_for ignores reversed ranges", "[dynamic_for]") {
  bool invoked = false;
  poet::dynamic_for<4>(10U, 5U, [&invoked](std::size_t) { invoked = true; });

  REQUIRE_FALSE(invoked);
}

TEST_CASE("dynamic_for executes single-step loops", "[dynamic_for]") {
  std::vector<std::size_t> visited;

  const std::size_t begin = 3U;
  const std::size_t end = 9U;

  poet::dynamic_for<1>(begin, end, [&visited](std::size_t index) {
    visited.push_back(index);
  });

  std::vector<std::size_t> expected(end - begin);
  std::iota(expected.begin(), expected.end(), begin);

  REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for honours custom unroll factors", "[dynamic_for]") {
  std::vector<std::size_t> visited;
  poet::dynamic_for<5>(12U, [&visited](std::size_t index) {
    visited.push_back(index * index);
  });

  std::vector<std::size_t> expected(12);
  for (std::size_t i = 0; i < expected.size(); ++i) {
    expected[i] = i * i;
  }

  REQUIRE(visited == expected);
}

TEST_CASE("dynamic_for supports the maximum unroll factor", "[dynamic_for]") {
  constexpr std::size_t kMaxUnroll = poet::kMaxStaticLoopBlock;
  std::vector<std::size_t> visited;
  const std::size_t begin = 11U;
  const std::size_t end = begin + kMaxUnroll;

  poet::dynamic_for<kMaxUnroll>(begin, end, [&visited](std::size_t index) {
    visited.push_back(index);
  });

  std::vector<std::size_t> expected(kMaxUnroll);
  std::iota(expected.begin(), expected.end(), begin);

  REQUIRE(visited == expected);
}
