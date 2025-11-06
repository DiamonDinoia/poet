#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <type_traits>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;

static_assert(std::is_same_v<make_range<0, 0>, std::integer_sequence<int, 0>>,
              "make_range should include the end value");
static_assert(std::is_same_v<make_range<2, 4>, std::integer_sequence<int, 2, 3, 4>>,
              "make_range should produce inclusive ranges");
static_assert(std::is_same_v<make_range<-2, 1>,
                             std::integer_sequence<int, -2, -1, 0, 1>>,
              "make_range should support negative bounds");

struct vector_dispatcher {
  std::vector<int> *values;

  template <int Width, int Height>
  void operator()(int scale) const {
    values->push_back(scale + Width * 10 + Height);
  }
};

struct sum_dispatcher {
  template <int X, int Y, int Z>
  int operator()(int base) const {
    return base + X + Y + Z;
  }
};

struct guard_dispatcher {
  bool *invoked;

  template <int Value>
  int operator()(int base) const {
    *invoked = true;
    return base + Value;
  }
};

}  // namespace

TEST_CASE("dispatch routes to the matching template instantiation",
          "[static_dispatch]") {
  std::vector<int> values;
  auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{2},
                                DispatchParam<make_range<-2, 1>>{-1});

  dispatch(vector_dispatcher{&values}, params, 5);

  REQUIRE(values == std::vector<int>{24});
}

TEST_CASE("dispatch forwards runtime arguments to the functor",
          "[static_dispatch]") {
  auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{5},
                                DispatchParam<make_range<1, 3>>{2},
                                DispatchParam<make_range<-1, 1>>{0});

  const auto result = dispatch(sum_dispatcher{}, params, 10);

  REQUIRE(result == 17);
}

TEST_CASE("dispatch returns default values when no match exists",
          "[static_dispatch]") {
  bool invoked = false;
  auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{3});

  const auto result = dispatch(guard_dispatcher{&invoked}, params, 8);

  REQUIRE(result == 0);
  REQUIRE_FALSE(invoked);
}
