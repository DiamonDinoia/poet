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
    extern "C" POET_NOINLINE_FLATTEN double sf##U##_runtime(const double *p, std::size_t n, double acc) {       \
        poet::dynamic_for<(U), 1>(std::size_t{ 0 }, n, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); }); \
        return acc;                                                                                             \
    }                                                                                                           \
    extern "C" POET_NOINLINE_FLATTEN double sf##U##_const(const double *p, double acc) {                        \
        poet::dynamic_for<(U), 1>(                                                                              \
          std::size_t{ 0 }, std::size_t{ 4096 }, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); });       \
        return acc;                                                                                             \
    }

POET_EXACT_UNROLL_CASE(1)
POET_EXACT_UNROLL_CASE(2)
POET_EXACT_UNROLL_CASE(4)
POET_EXACT_UNROLL_CASE(8)

/// A constant count that leaves BOTH a main loop and a tail: two full blocks
/// plus one spare iteration, the shape admiral's `codelet_many_body<40, double>`
/// runs as `dynamic_for<2>` over `kFull = 5`. A trip count the optimizer can see
/// invites the complete unroller to peel the main loop away, which drops the
/// contract and, on a body this wide, the register allocation with it. The pair
/// above cannot catch that: 4096 divides by every `Unroll` here, so it has no
/// tail, and it is far too long to peel.
#define POET_EXACT_UNROLL_SMALL_CASE(U, N)                                                               \
    extern "C" POET_NOINLINE_FLATTEN double sf##U##_small(const double *p, double acc) {                 \
        poet::dynamic_for<(U), 1>(                                                                       \
          std::size_t{ 0 }, std::size_t{ (N) }, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); }); \
        return acc;                                                                                      \
    }

POET_EXACT_UNROLL_SMALL_CASE(2, 5)
POET_EXACT_UNROLL_SMALL_CASE(4, 9)

/// A constant count of exactly `Unroll`: one block, no tail. admiral's col
/// codelet runs this as `dynamic_for<8>` over `N = 8`. One block is already
/// exactly `Unroll` bodies, so the whole call must fold to straight-line code:
/// the check requires zero back-edges AND zero branches here, because a hidden
/// block count would buy a compare, a branch and a runtime tail test for a
/// shape that has nothing left to decide.
#define POET_EXACT_UNROLL_ONE_CASE(U)                                                                    \
    extern "C" POET_NOINLINE_FLATTEN double sf##U##_one(const double *p, double acc) {                   \
        poet::dynamic_for<(U), 1>(                                                                       \
          std::size_t{ 0 }, std::size_t{ (U) }, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); }); \
        return acc;                                                                                      \
    }

POET_EXACT_UNROLL_ONE_CASE(2)
POET_EXACT_UNROLL_ONE_CASE(4)
POET_EXACT_UNROLL_ONE_CASE(8)

#ifdef POET_EXACT_UNROLL_CONTROL

/// Positive control for the counter, and deliberately not a `dynamic_for` path:
/// a hand-written loop whose body is four FMAs, under the same barrier. The
/// check requires exactly 4 here. A counter stuck on the body size, or reading
/// one instruction per loop whatever the body, cannot see a violation either,
/// so anything but 4 fails the check. It also exercises `POET_NO_UNROLL` on its
/// own, away from `dynamic_for`.
extern "C" POET_NOINLINE_FLATTEN double control_barrier4(const double *p, std::size_t n, double acc) {
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
extern "C" POET_NOINLINE_FLATTEN double control_naked1(const double *p, std::size_t n, double acc) {
    for (std::size_t i = 0; i < n; ++i) { acc = std::fma(acc, p[i], 1.0); }
    return acc;
}

#endif// POET_EXACT_UNROLL_CONTROL

#include <poet/core/undef_macros.hpp>
