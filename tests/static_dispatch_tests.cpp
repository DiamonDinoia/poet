#include <poet/core/static_dispatch.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>

#include <tuple>
#include <type_traits>
#include <vector>
#include <random>
#include <unordered_set>

namespace {
using poet::DispatchParam;
using poet::dispatch;
using poet::make_range;

static_assert(std::is_same_v<make_range<0, 0>, std::integer_sequence<int, 0>>,
  "make_range should include the end value");
static_assert(std::is_same_v<make_range<2, 4>, std::integer_sequence<int, 2, 3, 4>>,
  "make_range should produce inclusive ranges");
static_assert(std::is_same_v<make_range<-2, 1>, std::integer_sequence<int, -2, -1, 0, 1>>,
  "make_range should support negative bounds");

struct vector_dispatcher {
    std::vector<int> *values;

    template<int Width, int Height> void operator()(int scale) const { values->push_back(scale + Width * 10 + Height); }
};

struct sum_dispatcher {
    template<int X, int Y, int Z> int operator()(int base) const { return base + X + Y + Z; }
};

struct guard_dispatcher {
    bool *invoked;

    template<int Value> int operator()(int base) const {
        *invoked = true;
        return base + Value;
    }
};

struct return_type_dispatcher {
    template<int X> double operator()(double multiplier) const { return X * multiplier; }
};

}// namespace

// Helper functors placed at namespace scope because local classes cannot
// declare member templates.
namespace {
struct mover {
    template<int X> std::unique_ptr<int> operator()(int base) const { return std::make_unique<int>(base + X); }
};

struct receiver {
    template<int X> int operator()(std::unique_ptr<int> p) const { return X + *p; }
};

struct probe {
    int *out;
    template<int X, int Y> void operator()(int base) const { *out = base + X + Y; }
};

}// namespace

// Reporter for duplicate-value dispatch tests. Must live at namespace scope.
namespace {
struct duplicate_reporter {
    int *out;
    template<int X> void operator()() const { *out = X; }
};
}// namespace

struct array_setter {
    int *arr;
    template<int I> void operator()() const { arr[I] = I; }
};

struct throwing_dispatcher {
    int* counter;
    template<int X>
    void operator()(int threshold) const {
        (*counter)++;
        if (X >= threshold) { throw std::runtime_error("dispatch exception"); }
    }
};

struct triple_dispatcher {
    template<int X, int Y, int Z>
    int operator()(int base) const { return base + X * 100 + Y * 10 + Z; }
};

struct value_arg_functor {
    template<int X, int Y>
    int operator()(std::integral_constant<int, X>, std::integral_constant<int, Y>, int base) const {
        return base + X * 10 + Y;
    }
};

TEST_CASE("dispatch routes to the matching template instantiation", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<-2, 1>>{ -1 });

    dispatch(vector_dispatcher{ &values }, params, 5);

    REQUIRE(values == std::vector<int>{ 24 });
}

TEST_CASE("dispatch forwards runtime arguments to the functor", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{ 5 },
      DispatchParam<make_range<1, 3>>{ 2 },
      DispatchParam<make_range<-1, 1>>{ 0 });

    const auto result = dispatch(sum_dispatcher{}, params, 10);

    REQUIRE(result == 17);
}

TEST_CASE("dispatch returns default values when no match exists", "[static_dispatch]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 3 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 8);

    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch handles single-element ranges", "[static_dispatch]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<5, 5>>{ 5 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 10);

    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch handles boundary values in ranges", "[static_dispatch]") {
    std::vector<int> values;

    SECTION("minimum boundary") {
        auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 0 }, DispatchParam<make_range<-2, 1>>{ -2 });
        dispatch(vector_dispatcher{ &values }, params, 5);
        REQUIRE(values == std::vector<int>{ 3 });
    }

    SECTION("maximum boundary") {
        auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 3 }, DispatchParam<make_range<-2, 1>>{ 1 });
        dispatch(vector_dispatcher{ &values }, params, 5);
        REQUIRE(values == std::vector<int>{ 36 });
    }
}

TEST_CASE("dispatch handles all negative ranges", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<-5, -2>>{ -3 },
      DispatchParam<make_range<-10, -8>>{ -9 },
      DispatchParam<make_range<-1, 0>>{ 0 });

    const auto result = dispatch(sum_dispatcher{}, params, 100);

    REQUIRE(result == 88);// 100 + (-3) + (-9) + 0
}

TEST_CASE("dispatch handles ranges crossing zero", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<-3, 3>>{ 0 }, DispatchParam<make_range<-1, 1>>{ 0 });

    dispatch(vector_dispatcher{ &values }, params, 7);

    REQUIRE(values == std::vector<int>{ 7 });
}

TEST_CASE("dispatch handles void return type explicitly", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<1, 2>>{ 1 }, DispatchParam<make_range<3, 4>>{ 4 });

    // This should compile and execute without errors
    dispatch(vector_dispatcher{ &values }, params, 0);

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 14);
}

TEST_CASE("dispatch handles multiple out-of-range parameters", "[static_dispatch]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 10 }, DispatchParam<make_range<5, 7>>{ 15 });

    dispatch(vector_dispatcher{ &values }, params, 8);

    REQUIRE(values.empty());// Should not invoke because both params are out of range
}

TEST_CASE("dispatch handles non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NonContiguousSeq = std::integer_sequence<int, 1, 3, 7, 12>;

    std::vector<int> values;

    SECTION("matches first element") {
        auto params =
          std::make_tuple(DispatchParam<NonContiguousSeq>{ 1 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 2 });

        dispatch(vector_dispatcher{ &values }, params, 10);
        REQUIRE(values == std::vector<int>{ 22 });
    }

    SECTION("matches middle element") {
        auto params =
          std::make_tuple(DispatchParam<NonContiguousSeq>{ 7 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 5 });

        dispatch(vector_dispatcher{ &values }, params, 10);
        REQUIRE(values == std::vector<int>{ 85 });
    }

    SECTION("matches last element") {
        auto params =
          std::make_tuple(DispatchParam<NonContiguousSeq>{ 12 }, DispatchParam<std::integer_sequence<int, 2, 5>>{ 2 });

        dispatch(vector_dispatcher{ &values }, params, 10);
        REQUIRE(values == std::vector<int>{ 132 });
    }

    SECTION("value between sequence elements fails") {
        bool invoked = false;
        auto params = std::make_tuple(DispatchParam<NonContiguousSeq>{ 5 });

        const auto result = dispatch(guard_dispatcher{ &invoked }, params, 10);
        REQUIRE(result == 0);
        REQUIRE_FALSE(invoked);
    }
}

TEST_CASE("dispatch handles mixed contiguous and non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NonContiguousSeq = std::integer_sequence<int, 0, 10, 20>;

    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<NonContiguousSeq>{ 10 }, DispatchParam<make_range<1, 3>>{ 2 });

    dispatch(vector_dispatcher{ &values }, params, 5);
    REQUIRE(values == std::vector<int>{ 107 });
}

TEST_CASE("dispatch deterministic with duplicate sequence values", "[static_dispatch][duplicates]") {
    int out = 0;
    using DupSeq = std::integer_sequence<int, 5, 7, 5>;
    auto params = std::make_tuple(DispatchParam<DupSeq>{ 5 });

    dispatch(duplicate_reporter{ &out }, params);

    // Expect deterministic mapping (implementation uses first-match index)
    REQUIRE(out == 5);
}

TEST_CASE("dispatch handles negative non-contiguous sequences", "[static_dispatch][non-contiguous]") {
    using NegativeSeq = std::integer_sequence<int, -10, -5, 0, 7>;

    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<NegativeSeq>{ -5 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 20);
    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch preserves return value types correctly", "[static_dispatch]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });
    const auto result = dispatch(return_type_dispatcher{}, params, 2.5);

    REQUIRE(result == 7.5);
    REQUIRE(std::is_same_v<decltype(result), const double>);
}

TEST_CASE("dispatch return type is double at runtime", "[static_dispatch][type]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });
    auto result = dispatch(return_type_dispatcher{}, params, 2.5);
    REQUIRE(typeid(result) == typeid(double));
}

// Move-only return type: ensure dispatch can return move-only results
TEST_CASE("dispatch handles move-only return types", "[static_dispatch][move-only]") {
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 1 });
    auto result = dispatch(::mover{}, params, 10);
    REQUIRE(result);
    REQUIRE(*result == 11);
}

// Forwarding test: pass a move-only argument through dispatch to the functor
TEST_CASE("dispatch forwards move-only arguments", "[static_dispatch][forwarding]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 1>>{ 1 });

    std::unique_ptr<int> p = std::make_unique<int>(7);
    auto result = dispatch(::receiver{}, params, std::move(p));
    REQUIRE(result == 8);
}

// Stress test: moderate-sized table to ensure dispatch table generation works
TEST_CASE("dispatch moderate table stress", "[static_dispatch][stress]") {
    // smallish product (8*8 = 64) to avoid template explosion in CI
    using A = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;
    using B = std::integer_sequence<int, 0, 1, 2, 3, 4, 5, 6, 7>;

    int out = 0;

    auto params = std::make_tuple(DispatchParam<A>{ 3 }, DispatchParam<B>{ 4 });
    dispatch(::probe{ &out }, params, 100);
    REQUIRE(out == 107);
}

TEST_CASE("dispatch throws when no match exists with throw_t (non-void)", "[static_dispatch][throw]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 3 });

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 8), std::runtime_error);

    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch throws when no match exists with throw_t (void)", "[static_dispatch][throw]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<1, 2>>{ 3 }, DispatchParam<make_range<3, 4>>{ 4 });

    REQUIRE_THROWS_AS(dispatch(poet::throw_t, vector_dispatcher{ &values }, params, 0), std::runtime_error);

    REQUIRE(values.empty());
}

TEST_CASE("dispatch with throw_t succeeds on valid match", "[static_dispatch][throw]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{ 3 });

    // Should NOT throw when match exists
    const auto result = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 100);

    REQUIRE(invoked);
    REQUIRE(result == 103);  // 100 + 3
}

TEST_CASE("dispatch with throw_t handles multiple parameters correctly", "[static_dispatch][throw]") {
    auto params = std::make_tuple(
        DispatchParam<make_range<1, 3>>{ 2 },
        DispatchParam<make_range<5, 7>>{ 6 },
        DispatchParam<make_range<10, 12>>{ 11 }
    );

    // Valid match
    const auto result = dispatch(poet::throw_t, sum_dispatcher{}, params, 100);
    REQUIRE(result == 119);  // 100 + 2 + 6 + 11

    // Invalid match - first param out of range
    auto bad_params1 = std::make_tuple(
        DispatchParam<make_range<1, 3>>{ 0 },
        DispatchParam<make_range<5, 7>>{ 6 },
        DispatchParam<make_range<10, 12>>{ 11 }
    );
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, sum_dispatcher{}, bad_params1, 100), std::runtime_error);
}

TEST_CASE("dispatch with throw_t handles boundary values", "[static_dispatch][throw]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -10 });

    // Boundary: minimum value in range
    const auto result1 = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params, 50);
    REQUIRE(invoked);
    REQUIRE(result1 == 40);  // 50 + (-10)

    invoked = false;
    auto params2 = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -5 });

    // Boundary: maximum value in range
    const auto result2 = dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params2, 50);
    REQUIRE(invoked);
    REQUIRE(result2 == 45);  // 50 + (-5)

    // Boundary: just outside range
    auto params3 = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -11 });
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params3, 50), std::runtime_error);

    auto params4 = std::make_tuple(DispatchParam<make_range<-10, -5>>{ -4 });
    REQUIRE_THROWS_AS(dispatch(poet::throw_t, guard_dispatcher{ &invoked }, params4, 50), std::runtime_error);
}

TEST_CASE("dispatch with throw_t preserves return type deduction", "[static_dispatch][throw]") {
    auto params = std::make_tuple(DispatchParam<make_range<1, 5>>{ 3 });

    const auto result = dispatch(poet::throw_t, return_type_dispatcher{}, params, 2.5);

    static_assert(std::is_same_v<decltype(result), const double>, "Return type should be double");
    REQUIRE(result == 7.5);  // 3 * 2.5
}

TEST_CASE("dispatch handles contiguous descending sequence", "[static_dispatch][contiguous-descending]") {
    std::vector<int> values;
    using Desc = std::integer_sequence<int, 6, 5, 4, 3, 2, 1, 0>;
    for (int v : { 6, 5, 4, 3, 2, 1, 0 }) {
        auto params = std::make_tuple(DispatchParam<Desc>{ v }, DispatchParam<make_range<0, 0>>{ 0 });
        dispatch(vector_dispatcher{ &values }, params, 5);
    }
    REQUIRE(values == std::vector<int>{ 65, 55, 45, 35, 25, 15, 5 });
}

TEST_CASE("dispatch preserved non-throwing behavior with nothrow_on_no_match (non-void)",
  "[static_dispatch][nothrow]") {
    bool invoked = false;
    auto params = std::make_tuple(DispatchParam<make_range<0, 2>>{ 3 });

    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 8);

    REQUIRE(result == 0);
    REQUIRE_FALSE(invoked);
}

TEST_CASE("dispatch preserved non-throwing behavior with nothrow_on_no_match (void)", "[static_dispatch][nothrow]") {
    std::vector<int> values;
    auto params = std::make_tuple(DispatchParam<make_range<1, 2>>{ 3 }, DispatchParam<make_range<3, 4>>{ 4 });

    REQUIRE_NOTHROW(dispatch(vector_dispatcher{ &values }, params, 0));

    REQUIRE(values.empty());
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

    // deterministic pseudo-random choices for test stability
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
    // we'll only set a subset of the sequence: 3 and 12
    std::vector<int> set_indices = { 3, 12 };
    std::unordered_set<int> set_index_set(set_indices.begin(), set_indices.end());

    for (int idx : set_indices) {
        auto setter = [&arr](auto I) { arr[I] = I; };
        auto params = std::make_tuple(DispatchParam<Seq>{ idx });
        dispatch(setter, params);
    }

    // verify only the selected subset entries are set
    for (int i = 0; i < N; ++i) {
        if (set_index_set.count(i)) REQUIRE(arr[i] == i);
        else REQUIRE(arr[i] == 0);
    }
}

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
        auto setter = [&arr](auto I, auto J) { arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 100 + J; };
        auto params = std::make_tuple(DispatchParam<Seq1>{ x }, DispatchParam<Seq2>{ y });
        dispatch(setter, params);
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            int key = i * 16 + j;
            if (picks.count(key)) REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
            else REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
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
    for (int a : indices1) for (int b : indices2) index_set.insert(a * 256 + b);

    for (int a : indices1) {
            for (int b : indices2) {
            auto setter = [&arr](auto I, auto J) { arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J; };
            auto params = std::make_tuple(DispatchParam<Seq1>{ a }, DispatchParam<Seq2>{ b });
            dispatch(setter, params);
        }
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            int key = i * 256 + j;
            if (index_set.count(key)) REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
}

TEST_CASE("dispatch non-contiguous subset set (lambda ND)", "[static_dispatch][array][non-contiguous][subset][nd]") {
    constexpr int N1 = 16;
    constexpr int N2 = 16;
    std::array<std::array<int, N2>, N1> arr{};
    using Seq1 = std::integer_sequence<int, 1, 3, 5, 12>;
    using Seq2 = std::integer_sequence<int, 0, 2, 7>;
    // only set a subset of pairs: (3,2) and (12,7)
    std::vector<std::pair<int,int>> set_pairs = { {3, 2}, {12, 7} };
    std::unordered_set<int> set_keys;
    for (auto &p : set_pairs) set_keys.insert(p.first * 256 + p.second);

    for (auto &p : set_pairs) {
        auto setter = [&arr](auto I, auto J) { arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)] = I * 10 + J; };
        auto params = std::make_tuple(DispatchParam<Seq1>{ p.first }, DispatchParam<Seq2>{ p.second });
        dispatch(setter, params);
    }

    for (int i = 0; i < N1; ++i) {
        for (int j = 0; j < N2; ++j) {
            int key = i * 256 + j;
            if (set_keys.count(key)) REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 10 + j);
            else REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == 0);
        }
    }
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

    // pick a small subset of indices to set
    std::vector<std::pair<int,int>> picks = { {1, 2}, {3, 0} };
    std::unordered_set<int> pick_set;
    for (auto &p : picks) pick_set.insert(p.first * 16 + p.second);

    for (auto &p : picks) {
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
            int key = i * 16 + j;
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
        NonTrivial(const NonTrivial& other) : v(other.v) {} // non-trivial copy
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
                return std::make_unique<std::vector<int>>(std::initializer_list<int>{I, J});
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
    for (int i = 0; i < N1; ++i) for (int j = 0; j < N2; ++j) arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = 0;

    for (int i = 0; i <= 2; ++i) {
        for (int j = 0; j <= 2; ++j) {
            auto setter = [&arr](auto I, auto J) -> int* {
                return &arr[static_cast<std::size_t>(I)][static_cast<std::size_t>(J)];
            };
            auto params = std::make_tuple(DispatchParam<Seq1>{ i }, DispatchParam<Seq2>{ j });
            int *p = dispatch(setter, params);
            *p = i * 100 + j;
            REQUIRE(arr[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] == i * 100 + j);
        }
    }
}

TEST_CASE("dispatch with single-value repeated sequence", "[static_dispatch][edge_case]") {
    // All values are the same - should still work deterministically
    using RepeatedSeq = std::integer_sequence<int, 5, 5, 5, 5>;
    bool invoked = false;

    auto params = std::make_tuple(DispatchParam<RepeatedSeq>{ 5 });
    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 10);

    REQUIRE(result == 15);
    REQUIRE(invoked);
}

TEST_CASE("dispatch with value-argument form (integral_constant parameters)", "[static_dispatch][value-args]") {
    // Test functor that accepts integral_constant values as parameters
    auto params = std::make_tuple(DispatchParam<make_range<0, 3>>{ 2 }, DispatchParam<make_range<0, 3>>{ 1 });
    const auto result = dispatch(::value_arg_functor{}, params, 5);

    REQUIRE(result == 26);  // 5 + 2*10 + 1
}

TEST_CASE("dispatch 3D (triple dispatch)", "[static_dispatch][3d]") {
    // Small ranges to avoid template explosion
    auto params = std::make_tuple(
        DispatchParam<make_range<0, 2>>{ 1 },
        DispatchParam<make_range<0, 2>>{ 2 },
        DispatchParam<make_range<0, 2>>{ 0 }
    );

    const auto result = dispatch(::triple_dispatcher{}, params, 5);
    REQUIRE(result == 125);  // 5 + 1*100 + 2*10 + 0
}

TEST_CASE("dispatch exception safety", "[static_dispatch][exception]") {
    int count = 0;
    auto params = std::make_tuple(DispatchParam<make_range<0, 5>>{ 3 });

    try {
        dispatch(::throwing_dispatcher{ &count }, params, 2);
        FAIL("Expected exception was not thrown");
    } catch (const std::runtime_error&) {
        // Expected
        REQUIRE(count == 1);  // Should have executed once before throwing
    }
}

TEST_CASE("dispatch with single-element non-contiguous sequence", "[static_dispatch][edge_case]") {
    using SingleSeq = std::integer_sequence<int, 42>;
    bool invoked = false;

    auto params = std::make_tuple(DispatchParam<SingleSeq>{ 42 });
    const auto result = dispatch(guard_dispatcher{ &invoked }, params, 100);

    REQUIRE(result == 142);
    REQUIRE(invoked);

    // Test non-matching value
    invoked = false;
    auto params2 = std::make_tuple(DispatchParam<SingleSeq>{ 41 });
    const auto result2 = dispatch(guard_dispatcher{ &invoked }, params2, 100);

    REQUIRE(result2 == 0);
    REQUIRE_FALSE(invoked);
}
