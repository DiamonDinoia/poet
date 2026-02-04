#include <poet/core/static_for.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace {
using poet::detail::compute_range_count;
using poet::detail::kMaxStaticLoopBlock;
using poet::detail::compute_default_static_loop_block_size;

constexpr std::size_t kForwardBlockSize = 2;
static_assert(kForwardBlockSize <= kMaxStaticLoopBlock, "Tests should not request block sizes above the cap");
constexpr std::size_t kRemainderBlockSize = 3;
static_assert(kRemainderBlockSize <= kMaxStaticLoopBlock, "Tests should not request block sizes above the cap");

static_assert(compute_range_count<0, 0, 1>() == 0, "Zero-length ranges should not iterate");
static_assert(compute_range_count<0, 4, 1>() == 4, "Positive ranges compute the expected count");
static_assert(compute_range_count<2, 11, 3>() == 3, "Positive ranges honour custom steps");
static_assert(compute_range_count<5, 5, -1>() == 0, "Zero-length negative ranges yield zero iterations");
static_assert(compute_range_count<5, 1, -1>() == 4, "Negative ranges compute the expected count");
static_assert(compute_range_count<8, -1, -3>() == 3, "Negative ranges honour custom steps");

static_assert(compute_default_static_loop_block_size<0, 0, 1>() == 1,
  "Empty ranges clamp the default block size to one");
static_assert(compute_default_static_loop_block_size<0, 4, 1>() == 4,
  "The default block size spans the entire positive range");
static_assert(compute_default_static_loop_block_size<5, 1, -1>() == 4,
  "The default block size spans the entire negative range");
static_assert(compute_default_static_loop_block_size<0, kMaxStaticLoopBlock, 1>() == kMaxStaticLoopBlock,
  "Ranges matching the cap use the maximum default block size");
static_assert(compute_default_static_loop_block_size<0, kMaxStaticLoopBlock + 10, 1>() == kMaxStaticLoopBlock,
  "Large ranges clamp the default block size to the inclusive maximum");
static_assert(compute_default_static_loop_block_size<0, 4096, 1>() == kMaxStaticLoopBlock,
  "Very large ranges clamp the default block size to kMaxStaticLoopBlock");
static_assert(compute_default_static_loop_block_size<kMaxStaticLoopBlock + 5, 0, -1>() == kMaxStaticLoopBlock,
  "Descending ranges clamp the default block size to the maximum");

constexpr std::array<int, 4> enumerate_forward() {
    std::array<int, 4> values{};
    poet::detail::static_loop<0, 4, 1, kForwardBlockSize>([&values](auto index_constant) {
        const auto index = static_cast<std::size_t>(index_constant);
        values[index] = static_cast<int>(index);
    });
    return values;
}

constexpr std::array<int, 4> enumerate_forward_with_defaults() {
    std::array<int, 4> values{};
    poet::detail::static_loop<0, 4>([&values](auto index_constant) {
        const auto index = static_cast<std::size_t>(index_constant);
        values[index] = static_cast<int>(index + 1);
    });
    return values;
}

constexpr std::array<int, 5> enumerate_with_remainder() {
    std::array<int, 5> values{};
    poet::detail::static_loop<0, 5, 1, kRemainderBlockSize>([&values](auto index_constant) {
        const auto index = static_cast<std::size_t>(index_constant);
        values[index] = static_cast<int>(index + 1);
    });
    return values;
}

constexpr auto forward_indices = enumerate_forward();
static_assert(forward_indices[0] == 0);
static_assert(forward_indices[1] == 1);
static_assert(forward_indices[2] == 2);
static_assert(forward_indices[3] == 3);

constexpr auto forward_defaults = enumerate_forward_with_defaults();
static_assert(forward_defaults[0] == 1);
static_assert(forward_defaults[1] == 2);
static_assert(forward_defaults[2] == 3);
static_assert(forward_defaults[3] == 4);

constexpr auto remainder_indices = enumerate_with_remainder();
static_assert(remainder_indices[0] == 1);
static_assert(remainder_indices[1] == 2);
static_assert(remainder_indices[2] == 3);
static_assert(remainder_indices[3] == 4);
static_assert(remainder_indices[4] == 5);

template<typename Array> struct square_functor {
    Array *target;

    template<auto I> constexpr void operator()() const {
        (*target)[static_cast<std::size_t>(I)] = static_cast<int>(I * I);
    }
};

constexpr auto compute_squares_default_unroll() {
    std::array<int, 4> values{};
    poet::static_for<0, 4, 1>(square_functor<std::array<int, 4>>{ &values });
    return values;
}

constexpr auto compute_squares_inferred_defaults() {
    std::array<int, 4> values{};
    poet::static_for<0, 4>(square_functor<std::array<int, 4>>{ &values });
    return values;
}

constexpr auto compute_squares_custom_unroll() {
    std::array<int, 5> values{};
    poet::static_for<0, 5, 1, kForwardBlockSize>(square_functor<std::array<int, 5>>{ &values });
    return values;
}

constexpr auto compute_squares_clamped_default() {
    std::array<int, kMaxStaticLoopBlock + 10> values{};
    poet::static_for<0, kMaxStaticLoopBlock + 10>(square_functor<std::array<int, kMaxStaticLoopBlock + 10>>{ &values });
    return values;
}


constexpr auto default_squares = compute_squares_default_unroll();
static_assert(default_squares[0] == 0);
static_assert(default_squares[1] == 1);
static_assert(default_squares[2] == 4);
static_assert(default_squares[3] == 9);

constexpr auto inferred_defaults = compute_squares_inferred_defaults();
static_assert(inferred_defaults[0] == 0);
static_assert(inferred_defaults[1] == 1);
static_assert(inferred_defaults[2] == 4);
static_assert(inferred_defaults[3] == 9);

constexpr auto custom_squares = compute_squares_custom_unroll();
static_assert(custom_squares[0] == 0);
static_assert(custom_squares[1] == 1);
static_assert(custom_squares[2] == 4);
static_assert(custom_squares[3] == 9);
static_assert(custom_squares[4] == 16);

constexpr auto clamped_default_squares = compute_squares_clamped_default();
static_assert(clamped_default_squares.front() == 0);
static_assert(
  clamped_default_squares.back() == static_cast<int>((kMaxStaticLoopBlock + 9) * (kMaxStaticLoopBlock + 9)));


struct vector_accumulator {
    std::vector<int> *values;

    template<auto I> void operator()() const { values->push_back(static_cast<int>(I)); }
};

}// namespace

// Helper functor for lvalue-preservation test. Must live at namespace scope
// because member templates are not allowed in local classes.
namespace {
struct mutator {
    int sum = 0;
    template<auto I> void operator()() { sum += static_cast<int>(I); }
};
}// namespace

// Merged constexpr-friendly tests (previously in static_for_constexpr_tests.cpp)
namespace {

constexpr auto compute_squares_with_arg() {
    std::array<int, 4> values{};
    poet::static_for<0, 4>([&values](auto index_constant) {
        // Use the integral-constant type's ::value in a constexpr-friendly way.
        constexpr auto v = decltype(index_constant)::value;
        values[static_cast<std::size_t>(v)] = static_cast<int>(v * v);
    });
    return values;
}

constexpr auto compute_incremented_with_arg() {
    std::array<int, 4> values{};
    poet::static_for<0, 4>([&values](auto index_constant) {
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

// Functor for exception testing - must be at namespace scope
struct throwing_functor {
    int* counter;
    template<auto I> void operator()() {
        (*counter)++;
        if (I == 2) { throw std::runtime_error("test exception"); }
    }
};

}// namespace

TEST_CASE("static_loop emits descending sequences at runtime", "[static_for][loop]") {
    std::vector<int> values;
    poet::detail::static_loop<3, -1, -1, kForwardBlockSize>(
      [&values](auto index_constant) { values.push_back(static_cast<int>(index_constant)); });

    REQUIRE(values == std::vector<int>{ 3, 2, 1, 0 });
}

TEST_CASE("static_for dispatches template functors", "[static_for][dispatch]") {
    std::vector<int> invoked;
    poet::static_for<0, 4, 1, kForwardBlockSize>(vector_accumulator{ &invoked });

    REQUIRE(invoked == std::vector<int>{ 0, 1, 2, 3 });
}

TEST_CASE("static_for handles large iteration counts", "[static_for][limits]") {
    // Keep the iteration span modest to avoid excessive template instantiations
    // while still covering multiple blocks.
#ifdef _WIN32
    constexpr auto kSpan = kMaxStaticLoopBlock + 4;
#else
    constexpr auto kSpan = 2 * kMaxStaticLoopBlock;
#endif
    std::array<int, kSpan> values{};
    poet::static_for<0, kSpan, 1>(square_functor<std::array<int, kSpan>>{ &values });

    REQUIRE(values.front() == 0);
    REQUIRE(values.back() == (kSpan - 1) * (kSpan - 1));
}

TEST_CASE("static_for preserves lvalue functor state", "[static_for][lvalue]") {
    mutator m;
    // Passing an lvalue functor should update `m` itself.
    poet::static_for<0, 4>(m);
    REQUIRE(m.sum == (0 + 1 + 2 + 3));

    // Passing an rvalue should not modify `m`.
    poet::static_for<0, 4>(mutator{});
    REQUIRE(m.sum == (0 + 1 + 2 + 3));
}

TEST_CASE("static_for helper overload works", "[static_for][overload]") {
    std::vector<int> values;
    poet::static_for<3>([&values](auto index_constant) { values.push_back(static_cast<int>(index_constant)); });

    REQUIRE(values == std::vector<int>{ 0, 1, 2 });
}

TEST_CASE("static_for negative step with custom block size and remainder", "[static_for][negative][remainder]") {
    std::vector<int> values;
    // Range: 10 to 0 (exclusive), step -3 -> 10, 7, 4, 1
    // Count: 4 iterations, with block size 3, should have 1 full block + 1 remainder
    constexpr std::size_t kBlockSize = 3;
    poet::static_for<10, 0, -3, kBlockSize>([&values](auto index_constant) {
        values.push_back(static_cast<int>(index_constant));
    });

    REQUIRE(values == std::vector<int>{ 10, 7, 4, 1 });
}

TEST_CASE("static_for with all iterations in full blocks (no remainder)", "[static_for][blocks]") {
    std::vector<int> values;
    // 8 iterations with block size 4 -> exactly 2 full blocks, no remainder
    constexpr std::size_t kBlockSize = 4;
    poet::static_for<0, 8, 1, kBlockSize>([&values](auto index_constant) {
        values.push_back(static_cast<int>(index_constant));
    });

    REQUIRE(values == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
}

TEST_CASE("static_for nested loops", "[static_for][nested]") {
    std::vector<std::pair<int, int>> pairs;
    poet::static_for<0, 3>([&pairs](auto i) {
        poet::static_for<0, 3>([&pairs, i](auto j) {
            pairs.push_back({ static_cast<int>(i), static_cast<int>(j) });
        });
    });

    REQUIRE(pairs.size() == 9);
    REQUIRE(pairs[0] == std::make_pair(0, 0));
    REQUIRE(pairs[4] == std::make_pair(1, 1));
    REQUIRE(pairs[8] == std::make_pair(2, 2));
}

TEST_CASE("static_for exception safety", "[static_for][exception]") {
    int count = 0;
    try {
        poet::static_for<0, 5>(::throwing_functor{ &count });
        FAIL("Expected exception was not thrown");
    } catch (const std::runtime_error&) {
        // Expected - should have executed iterations 0, 1, 2 before throwing
        REQUIRE(count == 3);
    }
}

TEST_CASE("static_for with step > 1 forward", "[static_for][step]") {
    std::vector<int> values;
    // Range: 0 to 10, step 2 -> 0, 2, 4, 6, 8
    poet::static_for<0, 10, 2>([&values](auto index_constant) {
        values.push_back(static_cast<int>(index_constant));
    });

    REQUIRE(values == std::vector<int>{ 0, 2, 4, 6, 8 });
}

TEST_CASE("static_for with step > 1 backward", "[static_for][step]") {
    std::vector<int> values;
    // Range: 10 to 0, step -2 -> 10, 8, 6, 4, 2
    poet::static_for<10, 0, -2>([&values](auto index_constant) {
        values.push_back(static_cast<int>(index_constant));
    });

    REQUIRE(values == std::vector<int>{ 10, 8, 6, 4, 2 });
}

TEST_CASE("static_for single iteration", "[static_for][edge_case]") {
    int value = 0;
    poet::static_for<5, 6>([&value](auto index_constant) {
        value = static_cast<int>(index_constant);
    });

    REQUIRE(value == 5);
}
