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
// POET_ALWAYS_INLINE_LAMBDA
// ============================================================================
/// Forces inlining of lambda call operators. Place after the parameter list:
///
///   auto fn = [&](auto x) POET_ALWAYS_INLINE_LAMBDA { return x; };
///
/// Uses __attribute__((always_inline)) on GCC/Clang (the only syntax that
/// applies to the call operator) and [[msvc::forceinline]] on MSVC.
/// GCC 15+ / Clang 22+: attributed generic lambdas must be assigned to a
/// variable before passing to template functions.
#if defined(_MSC_VER) && !defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
#else
#define POET_ALWAYS_INLINE_LAMBDA
#endif

// ============================================================================
// POET_ASSUME
// ============================================================================
/// Generic assumption hint. UB if expression is false at runtime.
/// Uses [[assume(expr)]] when the compiler reports support via __has_cpp_attribute
/// (GCC >= 13, Clang >= 19), otherwise falls back to compiler builtins.
#ifdef __has_cpp_attribute
#if __has_cpp_attribute(assume)
#define POET_ASSUME(expr) [[assume(expr)]]// NOLINT(cppcoreguidelines-macro-usage)
#endif
#endif
#ifndef POET_ASSUME
#if defined(__clang__)
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
#endif// ifndef POET_ASSUME

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
#error "poet_count_trailing_zeros: no implementation for this compiler (need C++20 <bit>, GCC/Clang builtins, or MSVC)"
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
// C++20 Feature Detection
// ============================================================================
/// Use `consteval` for C++20+, fallback to `constexpr` for C++17.
#if __cplusplus >= 202002L
#define POET_CPP20_CONSTEVAL consteval
#else
#define POET_CPP20_CONSTEVAL constexpr
#endif

#endif// POET_CORE_MACROS_HPP
