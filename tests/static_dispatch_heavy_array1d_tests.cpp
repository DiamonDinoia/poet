// Heavy tests: 1D array dispatch operations
// Split from static_dispatch_heavy_tests.cpp to reduce linking memory usage

#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <random>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;
}

TEST_CASE("dispatch fills array using runtime index (lambda)", "[static_dispatch][array]") {
    constexpr int N = 8;
    int arr[N] = {};
    using Seq = make_range<0, N - 1>;

    for (int i = 0; i < N; ++i) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ i });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) REQUIRE(arr[i] == i);
}

TEST_CASE("dispatch sets selected random indexes only", "[static_dispatch][array][random]") {
    constexpr int N = 16;
    constexpr int M = 5;
    int arr[N] = {};
    using Seq = make_range<0, N - 1>;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, N - 1);
    std::unordered_set<int> picks;
    while (picks.size() < static_cast<std::size_t>(M)) picks.insert(dist(rng));

    for (int idx : picks) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) {
        if (picks.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}

TEST_CASE("dispatch loop over non-contiguous sequence", "[static_dispatch][array][non-contiguous]") {
    constexpr int N = 16;
    int arr[N] = {};
    using Seq = std::integer_sequence<int, 1, 3, 5, 12>;
    std::vector<int> indices = { 1, 3, 5, 12 };
    std::unordered_set<int> index_set(indices.begin(), indices.end());

    for (int idx : indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) {
        if (index_set.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}

TEST_CASE("dispatch non-contiguous subset set", "[static_dispatch][array][non-contiguous][subset]") {
    constexpr int N = 16;
    int arr[N] = {};
    using Seq = std::integer_sequence<int, 1, 3, 5, 12>;
    std::vector<int> set_indices = { 3, 12 };
    std::unordered_set<int> set_index_set(set_indices.begin(), set_indices.end());

    for (int idx : set_indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    for (int i = 0; i < N; ++i) {
        if (set_index_set.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}
