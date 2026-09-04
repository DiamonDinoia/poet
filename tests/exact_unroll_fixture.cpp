/// \file exact_unroll_fixture.cpp
/// \brief Compile-only fixture for the exact-unroll check. Nothing here runs.
///
/// `dynamic_for<U>` must emit the body exactly `U` times per back-edge. The body
/// here is one `std::fma` through a serial accumulator, so the FMA count inside
/// the main loop is the whole measurement, and the dependence chain leaves the
/// vectorizer nothing to do. `exact_unroll_check.cpp` compiles this file to
/// assembly, locates each function's main loop by its back-edge, and counts.
///
/// Every function carries a stable C name and stays out of line, so the check
/// needs no name demangling and no knowledge of the target ISA beyond "an FMA
/// is one instruction, or one call to `fma` where the target has no FMA".

#include <cmath>
#include <cstddef>

#include <poet/poet.hpp>

#include <poet/core/macros.hpp>

/// One pair per unroll factor: a runtime trip count, which is what the
/// guarantee covers, and a compile-time-provable one, which is what
/// `opaque_count` covers.
#define POET_EXACT_UNROLL_CASE(U)                                                                               \
    extern "C" POET_NOINLINE_FLATTEN double sf##U##_runtime(const double *p, std::size_t n) {                   \
        double acc = 1.0;                                                                                       \
        poet::dynamic_for<(U), 1>(std::size_t{ 0 }, n, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); }); \
        return acc;                                                                                             \
    }                                                                                                           \
    extern "C" POET_NOINLINE_FLATTEN double sf##U##_const(const double *p) {                                    \
        double acc = 1.0;                                                                                       \
        poet::dynamic_for<(U), 1>(                                                                              \
          std::size_t{ 0 }, std::size_t{ 4096 }, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); });       \
        return acc;                                                                                             \
    }

POET_EXACT_UNROLL_CASE(1)
POET_EXACT_UNROLL_CASE(2)
POET_EXACT_UNROLL_CASE(4)
POET_EXACT_UNROLL_CASE(8)

#ifdef POET_EXACT_UNROLL_CONTROL

/// Positive control for the counter, and deliberately not a `dynamic_for` path:
/// a hand-written loop whose body is four FMAs, under the same barrier. The
/// check requires exactly 4 here. A counter stuck on the body size, or reading
/// one instruction per loop whatever the body, cannot see a violation either,
/// so anything but 4 fails the check. It also exercises `POET_NO_UNROLL` on its
/// own, away from `dynamic_for`.
extern "C" POET_NOINLINE_FLATTEN double control_barrier4(const double *p, std::size_t n) {
    double acc = 1.0;
    POET_NO_UNROLL
    for (std::size_t i = 0; i + 4 <= n; i += 4) {
        acc = std::fma(acc, p[i], 1.0);
        acc = std::fma(acc, p[i + 1], 1.0);
        acc = std::fma(acc, p[i + 2], 1.0);
        acc = std::fma(acc, p[i + 3], 1.0);
    }
    return acc;
}

/// The same one-FMA body with NO barrier. Not asserted: its reading says whether
/// the flag set under test unrolls a small loop at all, so a cell where poet's
/// loops hold their shape can be told from a cell where nothing was unrolling.
extern "C" POET_NOINLINE_FLATTEN double control_naked1(const double *p, std::size_t n) {
    double acc = 1.0;
    for (std::size_t i = 0; i < n; ++i) { acc = std::fma(acc, p[i], 1.0); }
    return acc;
}

#endif// POET_EXACT_UNROLL_CONTROL

#include <poet/core/undef_macros.hpp>
