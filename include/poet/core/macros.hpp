#ifndef POET_CORE_MACROS_HPP
#define POET_CORE_MACROS_HPP

/// \file macros.hpp
/// \brief Compiler-specific macros for portability and optimization.

// ============================================================================
// POET_UNREACHABLE
// ============================================================================
/// Marks code path as unreachable. UB if reached at runtime.
#if defined(__GNUC__) || defined(__clang__)
#define POET_UNREACHABLE() __builtin_unreachable()// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER)
#define POET_UNREACHABLE() __assume(false)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_UNREACHABLE() \
    do {                   \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// POET_FORCEINLINE
// ============================================================================
/// Forces function inlining regardless of compiler heuristics.
#ifdef _MSC_VER
#define POET_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define POET_FORCEINLINE inline __attribute__((always_inline))
#else
#define POET_FORCEINLINE inline
#endif

// ============================================================================
// POET_ASSUME
// ============================================================================
/// Generic assumption hint. UB if expression is false at runtime.
/// Uses [[assume(expr)]] when the compiler reports support via __has_cpp_attribute
/// (GCC >= 13, Clang >= 19), otherwise falls back to compiler builtins.
#if __has_cpp_attribute(assume)
#define POET_ASSUME(expr) [[assume(expr)]]// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(__clang__)
#define POET_ASSUME(expr) __builtin_assume(expr)// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(__GNUC__)
#define POET_ASSUME(expr)                \
    do {                                 \
        if (!(expr)) POET_UNREACHABLE(); \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER)
#define POET_ASSUME(expr) __assume(expr)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_ASSUME(expr) \
    do {                  \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// POET_ASSUME_NOT_NULL
// ============================================================================
/// Tells compiler that pointer is non-null. UB if null at runtime.
/// Use on pointers derived from references or after null checks.
#if __has_cpp_attribute(assume)
#define POET_ASSUME_NOT_NULL(ptr) [[assume((ptr) != nullptr)]]// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(__clang__)
#define POET_ASSUME_NOT_NULL(ptr) __builtin_assume((ptr) != nullptr)// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(__GNUC__)
// GCC doesn't have __builtin_assume, use __builtin_unreachable() with guard
#define POET_ASSUME_NOT_NULL(ptr)       \
    do {                                \
        if (!(ptr)) POET_UNREACHABLE(); \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER)
#define POET_ASSUME_NOT_NULL(ptr) __assume((ptr) != nullptr)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_ASSUME_NOT_NULL(ptr) \
    do {                          \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// POET_NOINLINE
// ============================================================================
/// Prevents function inlining. Use to reduce code bloat or control I-cache.
#ifdef _MSC_VER
#define POET_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define POET_NOINLINE __attribute__((noinline))
#else
#define POET_NOINLINE
#endif

// ============================================================================
// POET_NOINLINE_FLATTEN
// ============================================================================
/// Prevents a function from being inlined into its caller (register isolation)
/// while forcing all functions it calls to be inlined into it.
///
/// This is critical for GCC codegen in static_for isolated blocks: without
/// flatten, GCC's ISRA pass extracts each functor operator() instantiation
/// into a separate out-of-line clone, causing redundant constant reloads
/// from .rodata on every call (5 FMA constants * 32 iterations = 160
/// wasted loads per block).  With flatten, GCC inlines all functor calls
/// within the block so constants are hoisted into registers once at block
/// entry — matching Clang's default behavior.
///
/// Clang already inlines everything within noinline blocks, so flatten is
/// harmless but included for consistency.
#ifdef _MSC_VER
#define POET_NOINLINE_FLATTEN __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define POET_NOINLINE_FLATTEN __attribute__((noinline, flatten))
#else
#define POET_NOINLINE_FLATTEN
#endif

// ============================================================================
// POET_LIKELY / POET_UNLIKELY
// ============================================================================
/// Branch prediction hints. Use for conditions true/false >95% of the time.
#if defined(__GNUC__) || defined(__clang__)
#define POET_LIKELY(x) __builtin_expect(!!(x), 1)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) __builtin_expect(!!(x), 0)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_LIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// poet_count_trailing_zeros
// ============================================================================
/// Counts trailing zero bits. UB if value is 0.
/// Guarded separately so it is defined only once even when macros.hpp is
/// re-included after undef_macros.hpp.
#ifndef POET_COUNT_TRAILING_ZEROS_DEFINED
#define POET_COUNT_TRAILING_ZEROS_DEFINED
#if __cplusplus >= 202002L
#include <bit>

constexpr auto poet_count_trailing_zeros(unsigned int value) noexcept -> unsigned int {
    return static_cast<unsigned int>(std::countr_zero(value));
}

constexpr auto poet_count_trailing_zeros(unsigned long value) noexcept -> unsigned int {// NOLINT(google-runtime-int)
    return static_cast<unsigned int>(std::countr_zero(value));
}

constexpr auto poet_count_trailing_zeros(unsigned long long value) noexcept
  -> unsigned int {// NOLINT(google-runtime-int)
    return static_cast<unsigned int>(std::countr_zero(value));
}

#elif defined(__GNUC__) || defined(__clang__)

constexpr auto poet_count_trailing_zeros(unsigned int value) noexcept -> unsigned int {
    return static_cast<unsigned int>(__builtin_ctz(value));
}

constexpr auto poet_count_trailing_zeros(unsigned long value) noexcept -> unsigned int {// NOLINT(google-runtime-int)
    return static_cast<unsigned int>(__builtin_ctzl(value));
}

constexpr auto poet_count_trailing_zeros(unsigned long long value) noexcept
  -> unsigned int {// NOLINT(google-runtime-int)
    return static_cast<unsigned int>(__builtin_ctzll(value));
}

#elif defined(_MSC_VER)

#include <intrin.h>

inline unsigned int poet_count_trailing_zeros(unsigned long value) noexcept {
    unsigned long index;
    _BitScanForward(&index, value);
    return static_cast<unsigned int>(index);
}

#if defined(_WIN64)
inline unsigned int poet_count_trailing_zeros(unsigned long long value) noexcept {
    unsigned long index;
    _BitScanForward64(&index, value);
    return static_cast<unsigned int>(index);
}
#endif

inline unsigned int poet_count_trailing_zeros(unsigned int value) noexcept {
    return poet_count_trailing_zeros(static_cast<unsigned long>(value));
}

#else

// Fallback: DeBruijn sequence lookup
namespace detail {
constexpr unsigned char debruijn_ctz_table[32] = { 0,
    1,
    28,
    2,
    29,
    14,
    24,
    3,
    30,
    22,
    20,
    15,
    25,
    17,
    4,
    8,
    31,
    27,
    13,
    23,
    21,
    19,
    16,
    7,
    26,
    12,
    18,
    6,
    11,
    5,
    10,
    9 };
}

inline constexpr unsigned int poet_count_trailing_zeros(unsigned int value) noexcept {
    return detail::debruijn_ctz_table[((value & -value) * 0x077CB531U) >> 27];
}

inline constexpr unsigned int poet_count_trailing_zeros(unsigned long value) noexcept {
    return poet_count_trailing_zeros(static_cast<unsigned int>(value));
}

inline constexpr unsigned int poet_count_trailing_zeros(unsigned long long value) noexcept {
    const auto lower = static_cast<unsigned int>(value);
    if (lower != 0) { return poet_count_trailing_zeros(lower); }
    const auto upper = static_cast<unsigned int>(value >> 32);
    return 32 + poet_count_trailing_zeros(upper);
}

#endif
#endif// POET_COUNT_TRAILING_ZEROS_DEFINED

// ============================================================================
// Optimization level detection
// ============================================================================
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER) && !defined(_DEBUG) && defined(NDEBUG)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_HIGH_OPTIMIZATION 0// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// POET_HOT_LOOP
// ============================================================================
/// Marks hot-path functions for aggressive optimization and inlining.
#if defined(__GNUC__) || defined(__clang__)
#define POET_HOT_LOOP inline __attribute__((hot, always_inline))
#elif defined(_MSC_VER)
#define POET_HOT_LOOP __forceinline
#else
#define POET_HOT_LOOP inline
#endif

// ============================================================================
// POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE
// ============================================================================
/// GCC register-allocator tuning for hot paths. Wrap performance-critical
/// function groups in POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE pairs.
///
/// At -O3 (POET_HIGH_OPTIMIZATION=1) on GCC, enables IRA pressure flags
/// (-fira-hoist-pressure, -fno-ira-share-spill-slots, -frename-registers)
/// that improve register allocation in unrolled and isolated blocks.
/// Without -O3, promotes the section to -O3.
/// On MSVC, enables aggressive optimization (/Ogt).
/// On Clang and others: no-op (Clang cannot enable optimizations via pragma).
///
/// Opt-out via -DPOET_DISABLE_PUSH_OPTIMIZE to preserve custom flags.
#ifndef POET_DISABLE_PUSH_OPTIMIZE
#if defined(__GNUC__) && !defined(__clang__)
#if POET_HIGH_OPTIMIZATION
// At -O3: Apply IRA pressure tuning + semantic-interposition removal for hot paths.
// -fno-semantic-interposition: allow inlining/IPO across function boundaries
//   (GCC default assumes exported symbols may be LD_PRELOAD-interposed, which
//    blocks optimizations even within the same TU; safe for header-only POET).
// -fvect-cost-model=cheap: allow vectorization even when GCC's cost model is
//   uncertain (helps SLP-vectorize independent accumulator chains in static_for).
#define POET_PUSH_OPTIMIZE                                                                                    \
    _Pragma("GCC push_options") _Pragma("GCC optimize(\"-fira-hoist-pressure\")")                             \
      _Pragma("GCC optimize(\"-fno-ira-share-spill-slots\")") _Pragma("GCC optimize(\"-frename-registers\")") \
        _Pragma("GCC optimize(\"-fno-semantic-interposition\")") _Pragma("GCC optimize(\"-fvect-cost-model=cheap\")")
#define POET_POP_OPTIMIZE _Pragma("GCC pop_options")
#else
// Without -O3: Enable -O3 for this section
#define POET_PUSH_OPTIMIZE _Pragma("GCC push_options") _Pragma("GCC optimize(\"-O3\")")
#define POET_POP_OPTIMIZE _Pragma("GCC pop_options")
#endif
#elif defined(_MSC_VER)
// In Debug builds, /RTC1 (runtime checks) is incompatible with /O2.
// Only enable optimization pragma in non-debug MSVC builds.
#ifndef _DEBUG
#define POET_PUSH_OPTIMIZE __pragma(optimize("gt", on))
#define POET_POP_OPTIMIZE __pragma(optimize("", on))
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
// Clang and others: no-op (Clang can only disable opts, not enable)
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
// User opted out: no-op to preserve their custom flags
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif

// ============================================================================
// POET_UNROLL_DISABLE
// ============================================================================
/// Suppresses compiler loop unrolling on the immediately following loop.
/// Place directly before `for` or `while` when the loop body is already
/// explicitly unrolled (e.g. via static_for) and further unrolling would
/// cause register spills.
#if defined(__clang__)
#define POET_UNROLL_DISABLE _Pragma("clang loop unroll(disable)")
#elif defined(__GNUC__)
#define POET_UNROLL_DISABLE _Pragma("GCC unroll 1")
#else
#define POET_UNROLL_DISABLE
#endif

// ============================================================================
// C++20/C++23 Feature Detection
// ============================================================================
/// Use `consteval` for C++20+, fallback to `constexpr` for C++17.
#if __cplusplus >= 202002L
#define POET_CPP20_CONSTEVAL consteval
#define POET_CPP20_CONSTEXPR constexpr
#else
#define POET_CPP20_CONSTEVAL constexpr
#define POET_CPP20_CONSTEXPR constexpr
#endif

/// Use the stronger C++23 `constexpr` guarantees when available.
#if __cplusplus >= 202302L
#define POET_CPP23_CONSTEXPR constexpr
#else
#define POET_CPP23_CONSTEXPR POET_CPP20_CONSTEXPR
#endif

#endif// POET_CORE_MACROS_HPP
