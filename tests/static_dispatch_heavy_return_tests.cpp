// Heavy tests: dispatch return value handling
// Split from static_dispatch_heavy_tests.cpp to reduce linking memory usage

#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;
}

TEST_CASE("dispatch ND lambda returns std::vector", "[static_dispatch][return][nd][vector]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> std::vector<int> { return std::vector<int>{ I, J }; };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            auto vec = dispatch(setter, params);
            REQUIRE(vec == std::vector<int>{ i, j });
        }
    }
}

TEST_CASE("dispatch ND lambda sets separate arrays and returns std::vector", "[static_dispatch][array][nd][vector][side-effect]") {
    constexpr int N1 = 4;
    constexpr int N2 = 4;
    std::array<std::array<int, N2>, N1> arrI{};
    std::array<std::array<int, N2>, N1> arrJ{};
    using Seq1 = make_range<0, N1 - 1>;
    using Seq2 = make_range<0, N2 - 1>;

    std::vector<std::pair<int, int>> picks = { {1, 2}, {3, 0} };
    std::unordered_set<int> pick_set;
    for (const auto& p : picks) pick_set.insert(p.first * 16 + p.second);

    for (const auto& p : picks) {
        auto setter = [&arrI, &arrJ](auto I, auto J) -> std::vector<int> {
            arrI[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I;
            arrJ[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = J;
            return std::vector<int>{ I, J };
        };
        auto params = std::make_tuple(DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
        auto vec = dispatch(setter, params);
        REQUIRE(vec == std::vector<int>{ p.first, p.second });
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 16 + j;
            if (pick_set.count(key)) {
                REQUIRE(arrI[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i);
                REQUIRE(arrJ[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == j);
            } else {
                REQUIRE(arrI[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
                REQUIRE(arrJ[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
            }
        }
    }
}

TEST_CASE("dispatch ND lambda returns NonTrivial (non-trivially copyable)", "[static_dispatch][return][nd][nontrivial]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    struct NonTrivial {
        std::vector<int> v;
        NonTrivial() = default;
        NonTrivial(int a, int b) : v{a, b} {}
        NonTrivial(const NonTrivial& other) : v(other.v) {}
        bool operator==(const NonTrivial& o) const { return v == o.v; }
    };

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> NonTrivial { return NonTrivial(I, J); };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            auto nt = dispatch(setter, params);
            REQUIRE(nt == NonTrivial(i, j));
        }
    }
}

TEST_CASE("dispatch ND lambda returns move-only unique_ptr<vector>", "[static_dispatch][return][nd][move-only]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> std::unique_ptr<std::vector<int>> {
                return std::make_unique<std::vector<int>>(std::initializer_list<int>{ I, J });
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            auto p = dispatch(setter, params);
            REQUIRE(*p == std::vector<int>{ i, j });
        }
    }
}

TEST_CASE("dispatch ND lambda returns pointer (lvalue)", "[static_dispatch][return][nd][ptr]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    constexpr int N1 = 3;
    constexpr int N2 = 3;
    std::array<std::array<int, N2>, N1> arr{};
    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = 0;
        }
    }

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [&arr](auto I, auto J) -> int* {
                return &arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)];
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            int* p = dispatch(setter, params);
            *p = i * 100 + j;
            REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
        }
    }
}
