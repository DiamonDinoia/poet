/// \file exact_unroll_fixture.cpp
/// \brief Compile-only fixture for exact_unroll_check: one `std::fma` per iteration.
///
/// The serial accumulator keeps the vectorizer out, so the FMAs in a function are
/// the bodies it emits. `opaque_count` hides every trip count, so a constant range
/// runs the same main loop as a runtime one and leaves nothing else to count.

#include <cmath>
#include <cstddef>

#include <poet/poet.hpp>

#define POET_EXACT_UNROLL_CASE(name, U, N)                                                               \
    extern "C" double name(const double *p, double acc) {                                                \
        poet::dynamic_for<(U), 1>(                                                                       \
          std::size_t{ 0 }, std::size_t{ (N) }, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); }); \
        return acc;                                                                                      \
    }

// Many blocks, no tail.
POET_EXACT_UNROLL_CASE(u1_blocks, 1, 64)
POET_EXACT_UNROLL_CASE(u2_blocks, 2, 64)
POET_EXACT_UNROLL_CASE(u4_blocks, 4, 64)
POET_EXACT_UNROLL_CASE(u8_blocks, 8, 64)
// Two blocks plus a tail: a constant the complete unroller would peel.
POET_EXACT_UNROLL_CASE(u2_tail, 2, 5)
POET_EXACT_UNROLL_CASE(u4_tail, 4, 9)
// Exactly one block: straight-line code, no loop.
POET_EXACT_UNROLL_CASE(u2_one, 2, 2)
POET_EXACT_UNROLL_CASE(u4_one, 4, 4)
POET_EXACT_UNROLL_CASE(u8_one, 8, 8)

/// Positive control: no pragma, no barrier. `-funroll-loops` must inflate it.
extern "C" double naked(const double *p, std::size_t n, double acc) {
    for (std::size_t i = 0; i < n; ++i) { acc = std::fma(acc, p[i], 1.0); }
    return acc;
}
