#if __cplusplus >= 202002L

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>


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
