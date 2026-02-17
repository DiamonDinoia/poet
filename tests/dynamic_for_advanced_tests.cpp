#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "dynamic_for_test_support.hpp"

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
        for (std::size_t i = 0; i < total; ++i) {
            REQUIRE(visited[i] == i);
        }
    }
}

TEST_CASE("dynamic_for passes compile-time lane in tiny and tail ranges", "[dynamic_for][lane][tail]") {
    constexpr std::size_t kUnroll = 8;
    std::vector<std::pair<std::size_t, std::size_t>> tiny;
    poet::dynamic_for<kUnroll>(5U, 8U, [&tiny](auto lane_c, std::size_t i) {
        tiny.emplace_back(i, decltype(lane_c)::value);
    });

    REQUIRE(tiny == std::vector<std::pair<std::size_t, std::size_t>>{
      { 5U, 0U }, { 6U, 1U }, { 7U, 2U }
    });

    std::vector<std::pair<std::size_t, std::size_t>> with_tail;
    poet::dynamic_for<kUnroll>(5U, 16U, [&with_tail](auto lane_c, std::size_t i) {
        with_tail.emplace_back(i, decltype(lane_c)::value);
    });

    REQUIRE(with_tail.size() == 11);
    for (std::size_t iter = 0; iter < with_tail.size(); ++iter) {
        REQUIRE(with_tail[iter].first == 5U + iter);
        REQUIRE(with_tail[iter].second == iter % kUnroll);
    }
}

TEST_CASE("dynamic_for lane works with count and auto-step overloads", "[dynamic_for][lane]") {
    std::vector<std::pair<std::size_t, std::size_t>> by_count;
    poet::dynamic_for<4>(6U, [&by_count](auto lane_c, std::size_t i) {
        by_count.emplace_back(i, decltype(lane_c)::value);
    });

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
