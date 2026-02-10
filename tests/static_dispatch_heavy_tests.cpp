// Heavy test splitting mechanism:
//
// This file contains memory-intensive static_dispatch test cases. To reduce peak memory
// usage during compilation, the build system compiles this file multiple times - once per
// test case - into separate executables. Each compilation defines POET_STATIC_DISPATCH_HEAVY_TEST_ID
// to enable only one test case via the POET_HEAVY_TEST_ENABLED() guard.
//
// Why not separate files?
// - Avoids file proliferation (would need 12+ separate .cpp files)
// - Tests share common setup code and includes
// - Easy to add new heavy tests without creating new files
//
// Build configuration (see tests/CMakeLists.txt):
// - poet_tests_std17_heavy_1 compiles with POET_STATIC_DISPATCH_HEAVY_TEST_ID=1
// - poet_tests_std17_heavy_2 compiles with POET_STATIC_DISPATCH_HEAVY_TEST_ID=2
// - ... and so on for each test ID

#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <memory>
#include <random>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;
}

#ifndef POET_STATIC_DISPATCH_HEAVY_TEST_ID
#define POET_STATIC_DISPATCH_HEAVY_TEST_ID 0
#endif

// Guard macro: only compiles test case with matching ID (or ID=0 for local testing)
#define POET_HEAVY_TEST_ENABLED(ID) \
  (POET_STATIC_DISPATCH_HEAVY_TEST_ID == 0 || POET_STATIC_DISPATCH_HEAVY_TEST_ID == (ID))

// HEAVY_TEST_CASE macro: simpler syntax for guarded test cases
// Usage:
//   #if POET_HEAVY_TEST_ENABLED(1)
//   HEAVY_TEST_CASE("test name", "[tags]") { /* test body */ }
//   #endif
//
// Note: The #if guard is required to conditionally compile the entire test case including
// its body. A macro-only solution doesn't work because when disabled, the orphaned { }
// body causes a syntax error.
#define HEAVY_TEST_CASE TEST_CASE

#if POET_HEAVY_TEST_ENABLED(1)
HEAVY_TEST_CASE("dispatch fills array using runtime index (lambda)", "[static_dispatch][array]") {
    constexpr int N = 8;
    int arr[N] = {};
    using Seq = make_range<0, N - 1>;

    for (int i = 0; i < N; ++i) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        dispatch(setter, DispatchParam<Seq>{ i });
    }

    for (int i = 0; i < N; ++i) REQUIRE(arr[i] == i);
}
#endif

#if POET_HEAVY_TEST_ENABLED(2)
HEAVY_TEST_CASE("dispatch sets selected random indexes only", "[static_dispatch][array][random]") {
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
        dispatch(setter, DispatchParam<Seq>{ idx });
    }

    for (int i = 0; i < N; ++i) {
        if (picks.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(3)

HEAVY_TEST_CASE("dispatch loop over non-contiguous sequence", "[static_dispatch][array][non-contiguous]") {
    constexpr int N = 16;
    int arr[N] = {};
    using Seq = std::integer_sequence<int, 1, 3, 5, 12>;
    std::vector<int> indices = { 1, 3, 5, 12 };
    std::unordered_set<int> index_set(indices.begin(), indices.end());

    for (int idx : indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        dispatch(setter, DispatchParam<Seq>{ idx });
    }

    for (int i = 0; i < N; ++i) {
        if (index_set.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(4)

HEAVY_TEST_CASE("dispatch non-contiguous subset set", "[static_dispatch][array][non-contiguous][subset]") {
    constexpr int N = 16;
    int arr[N] = {};
    using Seq = std::integer_sequence<int, 1, 3, 5, 12>;
    std::vector<int> set_indices = { 3, 12 };
    std::unordered_set<int> set_index_set(set_indices.begin(), set_indices.end());

    for (int idx : set_indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        dispatch(setter, DispatchParam<Seq>{ idx });
    }

    for (int i = 0; i < N; ++i) {
        if (set_index_set.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(5)

HEAVY_TEST_CASE("dispatch 2D sets selected random indexes only (lambda ND)", "[static_dispatch][array][random][nd]") {
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
        dispatch(setter, DispatchParam<Seq1>{ x }, DispatchParam<Seq2>{ y });
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 16 + j;
            if (picks.count(key)) REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
            else REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(6)

HEAVY_TEST_CASE("dispatch loop over non-contiguous sequences (lambda ND)", "[static_dispatch][array][non-contiguous][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    std::vector<int> indices1 = { 1, 3, 5, 12 };
    std::vector<int> indices2 = { 0, 2, 7 };
    std::unordered_set<int> index_set;
    for (int a : indices1) for (int b : indices2) index_set.insert(a * 256 + b);

    for (int a : indices1) {
        for (int b : indices2) {
            auto setter = [&arr](auto I, auto J) {
                arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J;
            };
            dispatch(setter, DispatchParam<Seq1>{ a }, DispatchParam<Seq2>{ b });
        }
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 256 + j;
            if (index_set.count(key)) REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(7)

HEAVY_TEST_CASE("dispatch non-contiguous subset set (lambda ND)", "[static_dispatch][array][non-contiguous][subset][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    std::vector<std::pair<int, int>> set_pairs = { {3, 2}, {12, 7} };
    std::unordered_set<int> set_keys;
    for (const auto& p : set_pairs) set_keys.insert(p.first * 256 + p.second);

    for (const auto& p : set_pairs) {
        auto setter = [&arr](auto I, auto J) {
            arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J;
        };
        dispatch(setter, DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            const int key = i * 256 + j;
            if (set_keys.count(key)) REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(8)

HEAVY_TEST_CASE("dispatch ND lambda returns std::vector", "[static_dispatch][return][nd][vector]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> std::vector<int> { return std::vector<int>{ I, J }; };
            auto vec = dispatch(setter, DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            REQUIRE(vec == std::vector<int>{ i, j });
        }
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(9)

HEAVY_TEST_CASE("dispatch ND lambda sets separate arrays and returns std::vector", "[static_dispatch][array][nd][vector][side-effect]") {
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
        auto vec = dispatch(setter, DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
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
#endif

#if POET_HEAVY_TEST_ENABLED(10)

HEAVY_TEST_CASE("dispatch ND lambda returns NonTrivial (non-trivially copyable)", "[static_dispatch][return][nd][nontrivial]") {
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
            auto nt = dispatch(setter, DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            REQUIRE(nt == NonTrivial(i, j));
        }
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(11)

HEAVY_TEST_CASE("dispatch ND lambda returns move-only unique_ptr<vector>", "[static_dispatch][return][nd][move-only]") {
    using Seq1 = make_range<0, 2>;
    using Seq2 = make_range<0, 2>;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [](auto I, auto J) -> std::unique_ptr<std::vector<int>> {
                return std::make_unique<std::vector<int>>(std::initializer_list<int>{ I, J });
            };
            auto p = dispatch(setter, DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            REQUIRE(*p == std::vector<int>{ i, j });
        }
    }
}
#endif

#if POET_HEAVY_TEST_ENABLED(12)

HEAVY_TEST_CASE("dispatch ND lambda returns pointer (lvalue)", "[static_dispatch][return][nd][ptr]") {
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
            int* p = dispatch(setter, DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            *p = i * 100 + j;
            REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
        }
    }
}
#endif
