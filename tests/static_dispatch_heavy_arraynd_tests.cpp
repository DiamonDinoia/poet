// Heavy tests: multi-dimensional array dispatch operations
// Split from static_dispatch_heavy_tests.cpp to reduce linking memory usage

#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <random>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;
}// namespace

TEST_CASE("dispatch 2D sets selected random indexes only (lambda ND)", "[static_dispatch][array][random][nd]") {
    constexpr int N1 = 4;
    constexpr int N2 = 4;
    constexpr int M = 5;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = make_range<0, N1 - 1>;
    using Seq2 = make_range<0, N2 - 1>;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist1(0, N1 - 1);
    std::uniform_int_distribution<int> dist2(0, N2 - 1);
    std::unordered_set<int> picks;
    while (picks.size() < static_cast<std::size_t>(M)) picks.insert(dist1(rng) * 16 + dist2(rng));

    for (int key : picks) {
        const int x = key / 16;
        const int y = key % 16;
        auto setter = [&arr](auto I, auto J) {
            arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 100 + J;
        };
        auto params = std::make_tuple(DispatchParam<Seq1>{ x }, DispatchParam<Seq2>{ y });
        dispatch(setter, params);
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 16 + j;
            if (picks.count(key))
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
            else
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}

TEST_CASE("dispatch loop over non-contiguous sequences (lambda ND)", "[static_dispatch][array][non-contiguous][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    std::vector<int> indices1 = { 1, 3, 5, 12 };
    std::vector<int> indices2 = { 0, 2, 7 };
    std::unordered_set<int> index_set;
    for (int a : indices1)
        for (int b : indices2) index_set.insert(a * 256 + b);

    for (int a : indices1) {
        for (int b : indices2) {
            auto setter = [&arr](auto I, auto J) {
                arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J;
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ a }, DispatchParam<Seq2>{ b });
            dispatch(setter, params);
        }
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 256 + j;
            if (index_set.count(key))
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}

TEST_CASE("dispatch non-contiguous subset set (lambda ND)", "[static_dispatch][array][non-contiguous][subset][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    std::vector<std::pair<int, int>> set_pairs = { { 3, 2 }, { 12, 7 } };
    std::unordered_set<int> set_keys;
    for (const auto &p : set_pairs) set_keys.insert(p.first * 256 + p.second);

    for (const auto &p : set_pairs) {
        auto setter = [&arr](
                        auto I, auto J) { arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J; };
        auto params = std::make_tuple(DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
        dispatch(setter, params);
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 256 + j;
            if (set_keys.count(key))
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else
                REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}
