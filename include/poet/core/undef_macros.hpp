#ifndef POET_UNDEF_MACROS_HPP
#define POET_UNDEF_MACROS_HPP

/// \file undef_macros.hpp
/// \brief Undefines all POET macros to prevent namespace pollution.
///
/// This header should be included AFTER including <poet/poet.hpp> if you want
/// to use POET's functionality but prevent its macros from polluting the global
/// preprocessor namespace.
///
/// POET defines several utility macros for portability and optimization:
/// - POET_UNREACHABLE: Marks unreachable code paths
/// - POET_FORCEINLINE: Forces function inlining
/// - POET_HOT_LOOP: Hot path optimization with loop unrolling
/// - POET_LIKELY / POET_UNLIKELY: Branch prediction hints
/// - POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE: Scoped optimization control
/// - POET_UNROLL_LOOP: Loop-specific unrolling hints
/// - POET_HIGH_OPTIMIZATION: Optimization level detection (internal)
/// - poet_count_trailing_zeros: (function, not macro)
///
/// **Usage Pattern:**
///
/// ```cpp
/// // Use POET with macros available
/// #include <poet/poet.hpp>
///
/// void my_poet_code() {
///     // POET_LIKELY, POET_UNREACHABLE, etc. are available here
///     if (POET_LIKELY(condition)) {
///         // ...
///     }
/// }
///
/// // Clean up macro namespace before including other headers
/// #include <poet/undef_macros.hpp>
///
/// // Other library headers that might define conflicting macros
/// #include <other_library.hpp>
/// ```
///
/// **Important Notes:**
/// 1. Include this header ONLY after all code that uses POET macros.
/// 2. Once macros are undefined, you cannot use them again unless you re-include <poet/poet.hpp>.
/// 3. The poet_count_trailing_zeros function remains available (it's not a macro).
/// 4. Template-based POET utilities (static_for, dynamic_for, dispatch) are unaffected.
///
/// **When to Use This:**
/// - When integrating POET into a larger codebase with many dependencies
/// - When other libraries define macros with similar names (e.g., LIKELY/UNLIKELY)
/// - When following a strict "no macro pollution" coding standard
/// - When using POET only in specific translation units
///
/// **When NOT to Use This:**
/// - In header files that use POET macros (it would break downstream users)
/// - In implementation files that heavily use POET macros throughout
/// - When performance-critical code relies on macro-based optimizations

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
// Undefine POET_LIKELY / POET_UNLIKELY
// ============================================================================
#ifdef POET_LIKELY
    #undef POET_LIKELY
#endif

#ifdef POET_UNLIKELY
    #undef POET_UNLIKELY
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
// Undefine POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE
// ============================================================================
#ifdef POET_PUSH_OPTIMIZE
    #undef POET_PUSH_OPTIMIZE
#endif

#ifdef POET_POP_OPTIMIZE
    #undef POET_POP_OPTIMIZE
#endif

// ============================================================================
// Undefine POET_UNROLL_LOOP variants
// ============================================================================
#ifdef POET_UNROLL_LOOP
    #undef POET_UNROLL_LOOP
#endif

#ifdef POET_UNROLL_LOOP_FULL
    #undef POET_UNROLL_LOOP_FULL
#endif

#ifdef POET_UNROLL_LOOP_DISABLE
    #undef POET_UNROLL_LOOP_DISABLE
#endif

// ============================================================================
// Note: poet_count_trailing_zeros is NOT undefined
// ============================================================================
// The poet_count_trailing_zeros function is NOT a macro - it's a regular
// C++ function, so it cannot be undefined and remains in the poet:: namespace.
// If you need to avoid name conflicts, use fully qualified names:
//   ::poet_count_trailing_zeros(value)
//
// Or use namespace aliasing:
//   namespace pt = poet;
//   pt::poet_count_trailing_zeros(value)

#endif// POET_UNDEF_MACROS_HPP
