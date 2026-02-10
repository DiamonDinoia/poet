#include <poet/poet.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <numeric>
#include <tuple>
#include <vector>

namespace {

struct static_collector {
    std::array<int, 4> *values;

    template<auto I> constexpr void operator()() const { (*values)[I] = static_cast<int>(I); }
};

struct dispatch_probe {
    int *value;

    template<int Width, int Height> void operator()(int scale) const { *value = Width * Height + scale; }
};

}// namespace

TEST_CASE("poet umbrella header exposes dynamic_for", "[poet][header]") {
    std::vector<std::size_t> visited;
    poet::dynamic_for<4>(0U, 4U, [&visited](std::size_t index) { visited.push_back(index); });

    std::vector<std::size_t> expected(4);
    std::iota(expected.begin(), expected.end(), 0U);

    REQUIRE(visited == expected);
}

TEST_CASE("poet umbrella header exposes static_for", "[poet][header]") {
    std::array<int, 4> values{};
    poet::static_for<0, 4>(static_collector{ &values });

    std::array<int, 4> expected{};
    std::iota(expected.begin(), expected.end(), 0);

    REQUIRE(values == expected);
}

TEST_CASE("poet umbrella header exposes dispatch", "[poet][header]") {
    int computed = -1;

    poet::dispatch(dispatch_probe{ &computed },
      poet::DispatchParam<poet::make_range<1, 4>>{ 2 },
      poet::DispatchParam<poet::make_range<1, 4>>{ 3 },
      5);

    REQUIRE(computed == 2 * 3 + 5);
}
