// NOLINTBEGIN(llvm-header-guard)
#ifndef POET_UNDEF_MACROS_HPP
#define POET_UNDEF_MACROS_HPP
// NOLINTEND(llvm-header-guard)

/// \file undef_macros.hpp
/// \brief Undefines all POET macros to prevent namespace pollution.
///
/// The umbrella header `<poet/poet.hpp>` includes this header automatically as
/// its last include, so macros are cleaned up by default.  If you include
/// individual POET headers instead, you can include this header manually after
/// all code that uses POET macros.
///
/// POET defines several utility macros for portability and optimization:
/// - POET_UNREACHABLE: Marks unreachable code paths
/// - POET_FORCEINLINE: Forces function inlining
/// - POET_NOINLINE: Prevents function inlining
/// - POET_HOT_LOOP: Hot path optimization with aggressive inlining
/// - POET_LIKELY / POET_UNLIKELY: Branch prediction hints
/// - POET_ASSUME / POET_ASSUME_NOT_NULL: Compiler assumption hints
/// - POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE: Scoped optimization control
/// - POET_CPP20_CONSTEVAL / POET_CPP20_CONSTEXPR / POET_CPP23_CONSTEXPR: Feature detection
/// - POET_HIGH_OPTIMIZATION: Optimization level detection (internal)
/// - poet_count_trailing_zeros: (function, not macro — unaffected)
///
/// **Usage with individual headers:**
///
/// ```cpp
/// #include <poet/core/static_for.hpp>
///
/// void my_poet_code() {
///     if (POET_LIKELY(condition)) {
///         // ...
///     }
/// }
///
/// // Clean up macro namespace before including other headers
/// #include <poet/core/undef_macros.hpp>
/// ```
///
/// **Important Notes:**
/// 1. Include this header ONLY after all code that uses POET macros.
/// 2. Once macros are undefined, you cannot use them again unless you re-include the POET headers.
/// 3. The poet_count_trailing_zeros function remains available (it's not a macro).
/// 4. Template-based POET utilities (static_for, dynamic_for, dispatch) are unaffected.

// ============================================================================
// Undefine POET_UNREACHABLE
// ============================================================================
#ifdef POET_UNREACHABLE
#undef POET_UNREACHABLE
#endif

// ============================================================================
// Undefine POET_FORCEINLINE
// ============================================================================
#ifdef POET_FORCEINLINE
#undef POET_FORCEINLINE
#endif

// ============================================================================
// Undefine POET_NOINLINE
// ============================================================================
#ifdef POET_NOINLINE
#undef POET_NOINLINE
#endif

// ============================================================================
// Undefine POET_LIKELY / POET_UNLIKELY
// ============================================================================
#ifdef POET_LIKELY
#undef POET_LIKELY
#endif

#ifdef POET_UNLIKELY
#undef POET_UNLIKELY
#endif

// ============================================================================
// Undefine POET_ASSUME / POET_ASSUME_NOT_NULL
// ============================================================================
#ifdef POET_ASSUME
#undef POET_ASSUME
#endif

#ifdef POET_ASSUME_NOT_NULL
#undef POET_ASSUME_NOT_NULL
#endif

// ============================================================================
// Undefine POET_HIGH_OPTIMIZATION (internal optimization detection)
// ============================================================================
#ifdef POET_HIGH_OPTIMIZATION
#undef POET_HIGH_OPTIMIZATION
#endif

// ============================================================================
// Undefine POET_HOT_LOOP
// ============================================================================
#ifdef POET_HOT_LOOP
#undef POET_HOT_LOOP
#endif

// ============================================================================
// Undefine POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE / POET_DISABLE_PUSH_OPTIMIZE
// ============================================================================
#ifdef POET_PUSH_OPTIMIZE
#undef POET_PUSH_OPTIMIZE
#endif

#ifdef POET_POP_OPTIMIZE
#undef POET_POP_OPTIMIZE
#endif

#ifdef POET_DISABLE_PUSH_OPTIMIZE
#undef POET_DISABLE_PUSH_OPTIMIZE
#endif

// ============================================================================
// Undefine C++20/C++23 feature detection macros
// ============================================================================
#ifdef POET_CPP20_CONSTEVAL
#undef POET_CPP20_CONSTEVAL
#endif

#ifdef POET_CPP20_CONSTEXPR
#undef POET_CPP20_CONSTEXPR
#endif

#ifdef POET_CPP23_CONSTEXPR
#undef POET_CPP23_CONSTEXPR
#endif

#endif// POET_UNDEF_MACROS_HPP
