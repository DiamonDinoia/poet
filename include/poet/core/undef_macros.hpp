/// \file undef_macros.hpp
/// \brief Undefines every POET macro to prevent namespace pollution.
///
/// The umbrella header `<poet/poet.hpp>` includes this header last, so macros
/// are cleaned up by default. When including individual POET headers, include
/// this header after all code that uses POET macros.
///
/// Re-including `<poet/core/macros.hpp>` afterwards restores them.
///
/// Only macros are removed: `poet::detail::count_trailing_zeros` and the
/// template utilities (static_for, dynamic_for, dispatch) stay available.
///
/// No include guard by design: the macros.hpp/undef_macros.hpp cycle must be
/// repeatable, and a guard would silently make every pass after the first a
/// no-op. `#undef` is idempotent, so re-including costs nothing.

// Re-arms macros.hpp so a subsequent include redefines everything. `#undef`
// of an undefined macro is a well-formed no-op, so nothing is guarded.
#undef POET_CORE_MACROS_HPP

#undef POET_CPLUSPLUS
#undef POET_UNREACHABLE
#undef POET_FORCEINLINE
#undef POET_ALWAYS_INLINE_LAMBDA
#undef POET_NOINLINE_FLATTEN
#undef POET_LIKELY
#undef POET_UNLIKELY
#undef POET_HIGH_OPTIMIZATION
#undef POET_HOT_LOOP
#undef POET_CPP20_CONSTEVAL

// Optimization pragmas and the internal pieces POET_PUSH_OPTIMIZE is built from.
#undef POET_PUSH_OPTIMIZE
#undef POET_POP_OPTIMIZE
#undef POET_PUSH_OPTIMIZE_BASE_
#undef POET_PUSH_VECTOR_WIDTH_
#undef POET_PUSH_SVE_BITS_STR_
#undef POET_PUSH_SVE_BITS_VAL_
