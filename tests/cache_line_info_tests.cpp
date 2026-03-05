#include <catch2/catch_test_macros.hpp>
#include <poet/core/cpu_info.hpp>

#ifdef POET_HAS_HW_DETECTION
#include <poet_detected_isa.hpp>
#endif

using namespace poet;

// ============================================================================
// Compile-time structural invariants
// ============================================================================

// Helper: validate cache_line_info for a given ISA's defaults
constexpr bool validate_cache_line(cache_line_info cl) {
    return cl.destructive_size >= cl.constructive_size && cl.destructive_size >= 32 && cl.destructive_size <= 256
           && cl.constructive_size >= 32 && cl.constructive_size <= 256
           && (cl.destructive_size & (cl.destructive_size - 1)) == 0// power of 2
           && (cl.constructive_size & (cl.constructive_size - 1)) == 0;// power of 2
}

static_assert(validate_cache_line(cache_line()), "detected cache line info fails invariants");

// Validate convenience functions match struct
static_assert(destructive_interference_size() == cache_line().destructive_size);
static_assert(constructive_interference_size() == cache_line().constructive_size);

// ============================================================================
// Validate __GCC_DESTRUCTIVE_SIZE consistency (when available)
// ============================================================================

#if defined(__GCC_DESTRUCTIVE_SIZE) && defined(__GCC_CONSTRUCTIVE_SIZE)
static_assert(cache_line().destructive_size == __GCC_DESTRUCTIVE_SIZE,
  "cache_line().destructive_size disagrees with __GCC_DESTRUCTIVE_SIZE");
static_assert(cache_line().constructive_size == __GCC_CONSTRUCTIVE_SIZE,
  "cache_line().constructive_size disagrees with __GCC_CONSTRUCTIVE_SIZE");
#endif

// ============================================================================
// Compile-time: validate against hardware ground truth from sysfs
// ============================================================================

#ifdef POET_HAS_HW_DETECTION
#if POET_HW_CACHE_LINE_SIZE > 0
static_assert(constructive_interference_size() == POET_HW_CACHE_LINE_SIZE,
  "constructive_interference_size() disagrees with /sys cache line size");
#endif
#endif

// ============================================================================
// Runtime: Catch2 tests
// ============================================================================

TEST_CASE("cache_line_info struct properties") {
    constexpr auto cl = cache_line();
    STATIC_REQUIRE(cl.destructive_size >= cl.constructive_size);
    STATIC_REQUIRE(cl.destructive_size >= 32);
    STATIC_REQUIRE(cl.constructive_size >= 32);
}

TEST_CASE("convenience functions match cache_line()") {
    constexpr auto cl = cache_line();
    STATIC_REQUIRE(destructive_interference_size() == cl.destructive_size);
    STATIC_REQUIRE(constructive_interference_size() == cl.constructive_size);
}

TEST_CASE("cache line sizes are powers of 2") {
    constexpr auto cl = cache_line();
    STATIC_REQUIRE((cl.destructive_size & (cl.destructive_size - 1)) == 0);
    STATIC_REQUIRE((cl.constructive_size & (cl.constructive_size - 1)) == 0);
}

TEST_CASE("cache_line is consistent with detected_isa") {
    [[maybe_unused]] constexpr auto isa = detected_isa();

    // When compiler macros are not available, validate ISA-based defaults
#if !defined(__GCC_DESTRUCTIVE_SIZE)
    constexpr auto cl = cache_line();
    switch (isa) {
    case instruction_set::ppc_altivec:
    case instruction_set::ppc_vsx:
        REQUIRE(cl.destructive_size == 128);
        break;
    case instruction_set::mips_msa:
        REQUIRE(cl.destructive_size == 32);
        break;
    default:
        REQUIRE(cl.destructive_size == 64);
        break;
    }
#endif
}

#ifdef POET_HAS_HW_DETECTION
#if POET_HW_CACHE_LINE_SIZE > 0
TEST_CASE("constructive size matches hardware (/sys)") {
    REQUIRE(constructive_interference_size() == POET_HW_CACHE_LINE_SIZE);
}
#endif
#endif
