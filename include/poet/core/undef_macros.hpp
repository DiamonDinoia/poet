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
/// - POET_ALWAYS_INLINE_LAMBDA: Forces lambda call-operator inlining
/// - POET_NOINLINE_FLATTEN: noinline+flatten for register-isolated blocks
/// - POET_HOT_LOOP: Hot path optimization with aggressive inlining
/// - POET_LIKELY / POET_UNLIKELY: Branch prediction hints
/// - POET_ASSUME: Compiler assumption hint
/// - POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE: GCC register-allocator tuning
/// - POET_CPP20_CONSTEVAL: Feature detection
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
/// **Re-include to restore macros:**
/// After this header is included, re-including <poet/core/macros.hpp> will
/// redefine all POET macros.
///
/// **Important Notes:**
/// 1. Include this header ONLY after all code that uses POET macros.
/// 2. The poet_count_trailing_zeros function remains available (it's not a macro).
/// 3. Template-based POET utilities (static_for, dynamic_for, dispatch) are unaffected.

// Re-arm macros.hpp so a subsequent #include <poet/core/macros.hpp> redefines
// everything.
#ifdef POET_CORE_MACROS_HPP
#undef POET_CORE_MACROS_HPP
#endif

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
// Undefine POET_ALWAYS_INLINE_LAMBDA
// ============================================================================
#ifdef POET_ALWAYS_INLINE_LAMBDA
#undef POET_ALWAYS_INLINE_LAMBDA
#endif

// ============================================================================
// Undefine POET_NOINLINE_FLATTEN
// ============================================================================
#ifdef POET_NOINLINE_FLATTEN
#undef POET_NOINLINE_FLATTEN
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
// Undefine POET_ASSUME
// ============================================================================
#ifdef POET_ASSUME
#undef POET_ASSUME
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

#ifdef POET_PUSH_OPTIMIZE_BASE_
#undef POET_PUSH_OPTIMIZE_BASE_
#endif

#ifdef POET_PUSH_VECTOR_WIDTH_
#undef POET_PUSH_VECTOR_WIDTH_
#endif

#ifdef POET_PUSH_SVE_BITS_STR_
#undef POET_PUSH_SVE_BITS_STR_
#endif

#ifdef POET_PUSH_SVE_BITS_VAL_
#undef POET_PUSH_SVE_BITS_VAL_
#endif

// ============================================================================
// Undefine POET_HIGH_OPTIMIZATION
// ============================================================================
#ifdef POET_HIGH_OPTIMIZATION
#undef POET_HIGH_OPTIMIZATION
#endif

// ============================================================================
// Undefine C++20/C++23 feature detection macros
// ============================================================================
#ifdef POET_CPP20_CONSTEVAL
#undef POET_CPP20_CONSTEVAL
#endif

#endif// POET_UNDEF_MACROS_HPP
