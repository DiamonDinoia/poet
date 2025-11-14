#include <poet/core/static_for.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

namespace {
using poet::compute_range_count;

constexpr auto compute_squares_with_arg() {
  std::array<int, 4> values{};
  poet::static_for_constexpr<0, 4>([&values](auto index_constant) {
    // Use the integral-constant type's ::value in a constexpr-friendly way.
    constexpr auto v = decltype(index_constant)::value;
    values[static_cast<std::size_t>(v)] = static_cast<int>(v * v);
  });
  return values;
}

constexpr auto compute_incremented_with_arg() {
  std::array<int, 4> values{};
  poet::static_for_constexpr<0, 4>([&values](auto index_constant) {
    constexpr auto v = decltype(index_constant)::value;
    values[static_cast<std::size_t>(v)] = static_cast<int>(v + 1);
  });
  return values;
}

constexpr auto squares = compute_squares_with_arg();
static_assert(squares[0] == 0);
static_assert(squares[1] == 1);
static_assert(squares[2] == 4);
static_assert(squares[3] == 9);

constexpr auto incremented = compute_incremented_with_arg();
static_assert(incremented[0] == 1);
static_assert(incremented[1] == 2);
static_assert(incremented[2] == 3);
static_assert(incremented[3] == 4);

} // namespace

TEST_CASE("static_for forwards an integral_constant to lambdas", "[static_for][constexpr]") {
  auto s = compute_squares_with_arg();
  REQUIRE(s == std::array<int,4>{0,1,4,9});
}
