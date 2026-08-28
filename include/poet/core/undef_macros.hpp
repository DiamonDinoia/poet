/// \file undef_macros.hpp
/// \brief Undefines every POET macro; `<poet/poet.hpp>` includes it last.
/// With individual headers, include it after all POET macro use.
/// No include guard: the macros.hpp/undef_macros.hpp cycle must be repeatable.

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
