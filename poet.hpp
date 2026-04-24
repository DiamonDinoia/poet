/* Auto-generated single-header. Do not edit directly. */

#ifndef POET_SINGLE_HEADER_GOLDBOT_HPP
#define POET_SINGLE_HEADER_GOLDBOT_HPP

// BEGIN_FILE: include/poet/poet.hpp

/// \file poet.hpp
/// \brief Umbrella header for the public POET API.

// clang-format off
// Include order matters: macros.hpp must come first and undef_macros.hpp last.
// NOLINTBEGIN(llvm-include-order)
/* Begin inline (angle): include/poet/core/macros.hpp */
// BEGIN_FILE: include/poet/core/macros.hpp

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
//
// Vector width: prefer the widest SIMD width enabled at compile time.
//   GCC 13/14 sometimes drop to 128-bit even with AVX2; -mprefer-vector-width
//   ensures hot paths use the full register width. On AArch64 SVE, -msve-vector-bits
//   locks the VL so the compiler can unroll without predication overhead.
//   Uses #pragma GCC target (not optimize) since these are machine flags.
//   Scoped to push/pop, so it does not affect user code outside POET internals.
//   On SSE-only x86 and NEON (fixed 128-bit): no target pragma needed.

// -- Internal: optimization flags common to all GCC hot paths
#define POET_PUSH_OPTIMIZE_BASE_                                                                              \
    _Pragma("GCC push_options") _Pragma("GCC optimize(\"-fira-hoist-pressure\")")                             \
      _Pragma("GCC optimize(\"-fno-ira-share-spill-slots\")") _Pragma("GCC optimize(\"-frename-registers\")") \
        _Pragma("GCC optimize(\"-fno-semantic-interposition\")") _Pragma("GCC optimize(\"-fvect-cost-model=cheap\")")

// -- Internal: target pragma for widest available vector width
#if defined(__AVX512F__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=512\")")
#elif defined(__AVX2__) || defined(__AVX__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=256\")")
#elif defined(__ARM_FEATURE_SVE_BITS) && __ARM_FEATURE_SVE_BITS > 0
// SVE with known VL (e.g. -msve-vector-bits=256): lock it for the hot path.
#define POET_PUSH_SVE_BITS_STR_(x) #x
#define POET_PUSH_SVE_BITS_VAL_(x) POET_PUSH_SVE_BITS_STR_(x)
#define POET_PUSH_VECTOR_WIDTH_ \
    _Pragma("GCC target(\"sve-vector-bits=" POET_PUSH_SVE_BITS_VAL_(__ARM_FEATURE_SVE_BITS) "\")")
#else
#define POET_PUSH_VECTOR_WIDTH_
#endif

#define POET_PUSH_OPTIMIZE POET_PUSH_OPTIMIZE_BASE_ POET_PUSH_VECTOR_WIDTH_
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
// C++20 Feature Detection
// ============================================================================
/// Use `consteval` for C++20+, fallback to `constexpr` for C++17.
#if __cplusplus >= 202002L
#define POET_CPP20_CONSTEVAL consteval
#else
#define POET_CPP20_CONSTEVAL constexpr
#endif

// END_FILE: include/poet/core/macros.hpp
/* End inline (angle): include/poet/core/macros.hpp */
/* Begin inline (angle): include/poet/version.hpp */
// BEGIN_FILE: include/poet/version.hpp

/// \file version.hpp
/// \brief POET version macros and constants.
///
/// Generated from version.hpp.in by cmake/GenerateVersion.cmake.
/// Do not edit by hand; re-run CMake configure or the pre-commit hook.

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)
#define POET_VERSION_MAJOR 0
#define POET_VERSION_MINOR 0
#define POET_VERSION_PATCH 0
#define POET_VERSION_STRING "0.0.0"
#define POET_VERSION_FULL "0.0.0-dev.80"
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)

namespace poet {

inline constexpr int version_major = POET_VERSION_MAJOR;
inline constexpr int version_minor = POET_VERSION_MINOR;
inline constexpr int version_patch = POET_VERSION_PATCH;
inline constexpr const char *version_string = POET_VERSION_STRING;
inline constexpr const char *version_full = POET_VERSION_FULL;

}// namespace poet
// END_FILE: include/poet/version.hpp
/* End inline (angle): include/poet/version.hpp */
/* Begin inline (angle): include/poet/core/cpu_info.hpp */
// BEGIN_FILE: include/poet/core/cpu_info.hpp

#include <cstddef>
/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet {

enum class instruction_set : unsigned char {
    generic,///< Generic/unknown ISA
    sse2,///< x86-64 SSE2 (128-bit vectors)
    sse4_2,///< x86-64 SSE4.2 (128-bit vectors)
    avx,///< x86-64 AVX (256-bit vectors)
    avx2,///< x86-64 AVX2 (256-bit vectors, integer ops)
    avx_512,///< x86-64 AVX-512 (512-bit vectors)
    arm_neon,///< ARM NEON (128-bit vectors)
    arm_sve,///< ARM SVE (scalable vectors)
    arm_sve2,///< ARM SVE2 (scalable vectors, enhanced)
    ppc_altivec,///< PowerPC AltiVec (128-bit vectors)
    ppc_vsx,///< PowerPC VSX (128/256-bit vectors)
    mips_msa,///< MIPS MSA (128-bit vectors)
};

/// Register and vector characteristics for a target ISA.
struct register_info {
    size_t gp_registers;
    size_t vector_registers;
    size_t vector_width_bits;
    size_t lanes_64bit;
    size_t lanes_32bit;
    instruction_set isa;
};

/// Cache line sizes used for padding and alignment decisions.
struct cache_line_info {
    size_t destructive_size;
    size_t constructive_size;
};

namespace detail {

    POET_CPP20_CONSTEVAL auto detect_instruction_set() noexcept -> instruction_set {
#ifdef __AVX512F__
        return instruction_set::avx_512;
#endif

#ifdef __AVX2__
        return instruction_set::avx2;
#endif

#ifdef __AVX__
        return instruction_set::avx;
#endif

#ifdef __SSE4_2__
        return instruction_set::sse4_2;
#endif

#ifdef __SSE2__
        return instruction_set::sse2;
#endif

#if defined(__ARM_FEATURE_SVE2) || defined(__ARM_FEATURE_SVE2__)
        return instruction_set::arm_sve2;
#endif

#if defined(__ARM_FEATURE_SVE) || defined(__ARM_FEATURE_SVE__)
        return instruction_set::arm_sve;
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        return instruction_set::arm_neon;
#endif

#ifdef __VSX__
        return instruction_set::ppc_vsx;
#endif

#ifdef __ALTIVEC__
        return instruction_set::ppc_altivec;
#endif

#ifdef __mips_msa
        return instruction_set::mips_msa;
#endif

        return instruction_set::generic;
    }

    POET_CPP20_CONSTEVAL auto get_register_info(instruction_set isa) noexcept -> register_info {
        switch (isa) {
        case instruction_set::sse2:
        case instruction_set::sse4_2:
            return register_info{
                16,// gp_registers
                16,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::avx:
        case instruction_set::avx2:
            return register_info{
                16,// gp_registers
                16,// vector_registers
                256,// vector_width_bits
                4,// lanes_64bit
                8,// lanes_32bit
                isa,
            };

        case instruction_set::avx_512:
            return register_info{
                16,// gp_registers
                32,// vector_registers
                512,// vector_width_bits
                8,// lanes_64bit
                16,// lanes_32bit
                isa,
            };

        case instruction_set::arm_neon:
        case instruction_set::arm_sve:
        case instruction_set::arm_sve2:
            return register_info{
                31,// gp_registers
                32,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::ppc_altivec:
            return register_info{
                32,// gp_registers
                32,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::ppc_vsx:
            return register_info{
                32,// gp_registers
                64,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::mips_msa:
            return register_info{
                32,// gp_registers
                32,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::generic:
        default:
            return register_info{
                16,// gp_registers
                16,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                instruction_set::generic,
            };
        }
    }

    POET_CPP20_CONSTEVAL auto detect_cache_line_info() noexcept -> cache_line_info {
#if defined(__GCC_DESTRUCTIVE_SIZE) && defined(__GCC_CONSTRUCTIVE_SIZE)
        return cache_line_info{ __GCC_DESTRUCTIVE_SIZE, __GCC_CONSTRUCTIVE_SIZE };
#else
        switch (detect_instruction_set()) {
        case instruction_set::sse2:
        case instruction_set::sse4_2:
        case instruction_set::avx:
        case instruction_set::avx2:
        case instruction_set::avx_512:
        case instruction_set::arm_neon:
        case instruction_set::arm_sve:
        case instruction_set::arm_sve2:
            return cache_line_info{ 64, 64 };

        case instruction_set::ppc_altivec:
        case instruction_set::ppc_vsx:
            return cache_line_info{ 128, 128 };

        case instruction_set::mips_msa:
            return cache_line_info{ 32, 32 };

        case instruction_set::generic:
        default:
            return cache_line_info{ 64, 64 };
        }
#endif
    }

}// namespace detail

/// Detects the compile target ISA from compiler defines.
POET_CPP20_CONSTEVAL auto detected_isa() noexcept -> instruction_set { return detail::detect_instruction_set(); }

/// Register information for the detected ISA.
POET_CPP20_CONSTEVAL auto available_registers() noexcept -> register_info {
    return detail::get_register_info(detected_isa());
}

/// Register information for a specific ISA.
POET_CPP20_CONSTEVAL auto registers_for(instruction_set isa) noexcept -> register_info {
    return detail::get_register_info(isa);
}

POET_CPP20_CONSTEVAL auto vector_register_count() noexcept -> size_t { return available_registers().vector_registers; }

POET_CPP20_CONSTEVAL auto vector_width_bits() noexcept -> size_t { return available_registers().vector_width_bits; }

POET_CPP20_CONSTEVAL auto vector_lanes_64bit() noexcept -> size_t { return available_registers().lanes_64bit; }

POET_CPP20_CONSTEVAL auto vector_lanes_32bit() noexcept -> size_t { return available_registers().lanes_32bit; }

POET_CPP20_CONSTEVAL auto cache_line() noexcept -> cache_line_info { return detail::detect_cache_line_info(); }

POET_CPP20_CONSTEVAL auto destructive_interference_size() noexcept -> size_t { return cache_line().destructive_size; }

POET_CPP20_CONSTEVAL auto constructive_interference_size() noexcept -> size_t { return cache_line().constructive_size; }

}// namespace poet
// END_FILE: include/poet/core/cpu_info.hpp
/* End inline (angle): include/poet/core/cpu_info.hpp */
/* Begin inline (angle): include/poet/core/dynamic_for.hpp */
// BEGIN_FILE: include/poet/core/dynamic_for.hpp

/// \file dynamic_for.hpp
/// \brief Runtime loops emitted as compile-time unrolled blocks.

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */


namespace poet {

namespace detail {

    template<typename...> inline constexpr bool always_false_v = false;

    struct lane_by_value_tag {};///< func(integral_constant<size_t, Lane>{}, index)
    struct index_only_tag {};///< func(index)

    template<typename Func, typename T> constexpr auto detect_callable_form() {
        if constexpr (std::is_invocable_v<Func &, std::integral_constant<std::size_t, 0>, T>) {
            return lane_by_value_tag{};
        } else if constexpr (std::is_invocable_v<Func &, T>) {
            return index_only_tag{};
        } else {
            static_assert(always_false_v<Func>, "dynamic_for callable must accept (lane, index) or (index)");
            return index_only_tag{};
        }
    }

    template<typename Func, typename T> using callable_form_t = decltype(detect_callable_form<Func, T>());

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(lane_by_value_tag /*tag*/, Func &func, T index) {
        func(std::integral_constant<std::size_t, Lane>{}, index);
    }

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(index_only_tag /*tag*/, Func &func, T index) {
        func(index);
    }

    // Emits `Count` calls as a single expanded pack; the comma-fold carries `index` forward
    // so each lane sees a distinct compile-time `Lane` and the running runtime `index`.
    template<typename FormTag, typename Callable, typename T, std::size_t... Lanes>
    POET_FORCEINLINE constexpr void
      emit_carried(Callable &callable, T index, T stride, std::index_sequence<Lanes...> /*seq*/) {
        ((invoke_lane<Lanes>(FormTag{}, callable, index), index += stride), ...);
    }

    template<typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void emit_block(FormTag /*tag*/,
      [[maybe_unused]] Callable &callable,
      [[maybe_unused]] T base,
      [[maybe_unused]] T stride) {
        if constexpr (Count > 0) { emit_carried<FormTag>(callable, base, stride, std::make_index_sequence<Count>{}); }
    }

    template<std::ptrdiff_t Step, typename FormTag, typename Callable, typename T, std::size_t... Lanes>
    POET_FORCEINLINE constexpr void
      emit_carried_ct(Callable &callable, T index, std::index_sequence<Lanes...> /*seq*/) {
        ((invoke_lane<Lanes>(FormTag{}, callable, index), index += static_cast<T>(Step)), ...);
    }

    template<std::ptrdiff_t Step, typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void
      emit_block_ct(FormTag /*tag*/, [[maybe_unused]] Callable &callable, [[maybe_unused]] T base) {
        if constexpr (Count > 0) { emit_carried_ct<Step, FormTag>(callable, base, std::make_index_sequence<Count>{}); }
    }

    // Handles a leftover count in [0, N) by emitting at most log2(N) fixed-size unrolled
    // blocks — picks the largest power of two <= N/2, optionally emits it, then recurses on
    // the remainder. Each level has a compile-time `half`, so codegen stays fully unrolled.
    template<std::size_t N, typename FormTag, typename Callable, typename T>
    POET_FORCEINLINE void tail_binary(std::size_t count, Callable &callable, T index, T stride) {
        if constexpr (N <= 1) {
        } else {
            // Largest power of two strictly less than N — the block size we might emit here.
            constexpr std::size_t half = []() constexpr -> std::size_t {
                std::size_t pow2 = 1;
                while (pow2 * 2 < N) { pow2 *= 2; }
                return pow2;
            }();
            // If this level fires, it consumes exactly `half` iterations; otherwise all
            // `count` pass through to the smaller level.
            const std::size_t rem = (count >= half) ? (count - half) : count;
            tail_binary<half, FormTag>(rem, callable, index, stride);
            if (count >= half) {
                // Smaller blocks run first over the low indices; this block picks up at
                // `index + rem*stride` so iteration order is preserved.
                emit_block<FormTag, Callable, T, half>(
                  FormTag{}, callable, static_cast<T>(index + (static_cast<T>(rem) * stride)), stride);
            }
        }
    }

    template<std::size_t N, typename FormTag, typename Callable, typename T>
    POET_NOINLINE_FLATTEN void tail_binary_noinline(std::size_t count, Callable &callable, T index, T stride) {
        tail_binary<N, FormTag>(count, callable, index, stride);
    }

    template<std::size_t N, std::ptrdiff_t Step, typename FormTag, typename Callable, typename T>
    POET_FORCEINLINE void tail_binary_ct(std::size_t count, Callable &callable, T index) {
        if constexpr (N <= 1) {
        } else {
            constexpr std::size_t half = []() constexpr -> std::size_t {
                std::size_t pow2 = 1;
                while (pow2 * 2 < N) { pow2 *= 2; }
                return pow2;
            }();
            const std::size_t rem = (count >= half) ? (count - half) : count;
            tail_binary_ct<half, Step, FormTag>(rem, callable, index);
            if (count >= half) {
                emit_block_ct<Step, FormTag, Callable, T, half>(
                  FormTag{}, callable, static_cast<T>(index + static_cast<T>(static_cast<std::ptrdiff_t>(rem) * Step)));
            }
        }
    }

    template<std::size_t N, std::ptrdiff_t Step, typename FormTag, typename Callable, typename T>
    POET_NOINLINE_FLATTEN void tail_binary_ct_noinline(std::size_t count, Callable &callable, T index) {
        tail_binary_ct<N, Step, FormTag>(count, callable, index);
    }

    POET_PUSH_OPTIMIZE

    // Handles signed and unsigned-wrapped-negative strides uniformly. For unsigned T, a
    // "negative" stride arrives as a large positive value (> half_max); we detect and flip
    // it so both directions share the same (dist + |stride| - 1) / |stride| ceiling formula.
    template<typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_complex(T begin, T end, T stride) -> std::size_t {
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (POET_UNLIKELY(stride < 0 || is_wrapped_negative)) {
            // Descending: empty unless begin > end.
            if (POET_UNLIKELY(begin <= end)) { return 0; }
            T abs_stride;
            if constexpr (std::is_signed_v<T>) {
                abs_stride = static_cast<T>(-stride);
            } else {
                // Unsigned two's-complement negation recovers the original magnitude.
                abs_stride = static_cast<T>(0) - stride;
            }
            auto dist = static_cast<std::size_t>(begin - end);
            auto ustride = static_cast<std::size_t>(abs_stride);
            return (dist + ustride - 1) / ustride;
        }

        if (POET_UNLIKELY(begin >= end)) { return 0; }

        auto dist = static_cast<std::size_t>(end - begin);
        auto ustride = static_cast<std::size_t>(stride);
        // Classic `x & (x-1) == 0` power-of-two test; replaces the divide with a shift.
        const bool is_power_of_2 = (ustride & (ustride - 1)) == 0;

        if (POET_LIKELY(is_power_of_2)) {
            const unsigned int shift = poet_count_trailing_zeros(ustride);
            return (dist + ustride - 1) >> shift;
        }
        return (dist + ustride - 1) / ustride;
    }

    template<std::ptrdiff_t Step, typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_ct(T begin, T end) -> std::size_t {
        static_assert(Step != 0, "Step must be non-zero");
        if constexpr (Step > 0) {
            if (begin >= end) { return 0; }
            auto dist = static_cast<std::size_t>(end - begin);
            constexpr auto ustride = static_cast<std::size_t>(Step);
            return (dist + ustride - 1) / ustride;
        } else {
            if (begin <= end) { return 0; }
            auto dist = static_cast<std::size_t>(begin - end);
            constexpr auto ustride = static_cast<std::size_t>(-Step);
            return (dist + ustride - 1) / ustride;
        }
    }

    template<typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP void
      dynamic_for_impl_general(const T begin, const T end, const T stride, Callable &callable, const FormTag tag) {
        if (POET_UNLIKELY(stride == 0)) { return; }

        std::size_t count = calculate_iteration_count_complex(begin, end, stride);
        if (POET_UNLIKELY(count == 0)) { return; }

        if constexpr (Unroll == 1) {
            T index = begin;
            for (std::size_t i = 0; i < count; ++i) {
                invoke_lane<0>(tag, callable, index);
                index += stride;
            }
        } else {
            T index = begin;
            std::size_t remaining = count;

            if (POET_UNLIKELY(count < Unroll)) {
                if (count > 0) { tail_binary<Unroll, FormTag>(count, callable, index, stride); }
                return;
            }

            const T stride_times_unroll = static_cast<T>(Unroll) * stride;
            while (remaining >= Unroll) {
                emit_block<FormTag, Callable, T, Unroll>(tag, callable, index, stride);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            if (remaining > 0) { tail_binary_noinline<Unroll, FormTag>(remaining, callable, index, stride); }
        }
    }

    template<std::ptrdiff_t Step, typename T, typename Callable, std::size_t Unroll, typename FormTag>
    POET_HOT_LOOP void dynamic_for_impl_ct_stride(const T begin, const T end, Callable &callable, const FormTag tag) {
        std::size_t count = calculate_iteration_count_ct<Step>(begin, end);
        if (POET_UNLIKELY(count == 0)) { return; }

        if constexpr (Unroll == 1) {
            T index = begin;
            constexpr T ct_stride = static_cast<T>(Step);
            for (std::size_t i = 0; i < count; ++i) {
                invoke_lane<0>(tag, callable, index);
                index += ct_stride;
            }
        } else {
            T index = begin;
            std::size_t remaining = count;

            if (POET_UNLIKELY(count < Unroll)) {
                if (count > 0) { tail_binary_ct<Unroll, Step, FormTag>(count, callable, index); }
                return;
            }

            constexpr T stride_times_unroll = static_cast<T>(static_cast<std::ptrdiff_t>(Unroll) * Step);
            while (remaining >= Unroll) {
                emit_block_ct<Step, FormTag, Callable, T, Unroll>(tag, callable, index);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            if (remaining > 0) { tail_binary_ct_noinline<Unroll, Step, FormTag>(remaining, callable, index); }
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// ============================================================================
// Public API
// ============================================================================

/// \brief Runs `[begin, end)` with compile-time unrolled blocks.
///
/// `func` may take `(index)` or `(lane, index)`, where `lane` is
/// `std::integral_constant<std::size_t, L>`. Use the lane form for
/// multi-accumulator kernels; prefer a plain `for` loop for trivial index-only work.
template<std::size_t Unroll, typename T1, typename T2, typename T3, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");

    using T = std::common_type_t<T1, T2, T3>;
    const T stride = static_cast<T>(step);

    auto run = [&](auto &callable) POET_ALWAYS_INLINE_LAMBDA -> void {
        using callable_t = std::remove_reference_t<decltype(callable)>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        if (stride == static_cast<T>(1)) {
            detail::dynamic_for_impl_ct_stride<1, T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
        } else {
            detail::dynamic_for_impl_general<T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), stride, callable, form_tag{});
        }
    };

    if constexpr (std::is_lvalue_reference_v<Func>) {
        run(func);
    } else {
        std::remove_reference_t<Func> local(std::forward<Func>(func));
        run(local);
    }
}

/// \brief Runs `[begin, end)` with a compile-time stride.
template<std::size_t Unroll, std::ptrdiff_t Step, typename T1, typename T2, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");

    using T = std::common_type_t<T1, T2>;

    auto run = [&](auto &callable) POET_ALWAYS_INLINE_LAMBDA -> void {
        using callable_t = std::remove_reference_t<decltype(callable)>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        detail::dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
    };

    if constexpr (std::is_lvalue_reference_v<Func>) {
        run(func);
    } else {
        std::remove_reference_t<Func> local(std::forward<Func>(func));
        run(local);
    }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/// \brief Runs `[begin, end)` with an inferred step of `+1` or `-1`.
template<std::size_t Unroll, typename T1, typename T2, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    T s_begin = static_cast<T>(begin);
    T s_end = static_cast<T>(end);
    T step = (s_begin <= s_end) ? static_cast<T>(1) : static_cast<T>(-1);

    dynamic_for<Unroll>(s_begin, s_end, step, std::forward<Func>(func));
}

/// \brief Convenience overload for `[0, count)`.
template<std::size_t Unroll, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll>(static_cast<std::size_t>(0), count, std::size_t{ 1 }, std::forward<Func>(func));
}
#endif

}// namespace poet


#if __cplusplus >= 202002L
#include <ranges>
#include <tuple>

namespace poet {

template<typename Func, std::size_t Unroll> struct dynamic_for_adaptor {
    Func func;
    constexpr explicit dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

template<typename Func, std::size_t Unroll, typename Range>
requires std::ranges::range<Range> void operator|(Range const &r, dynamic_for_adaptor<Func, Unroll> const &ad) {
    auto it = std::ranges::begin(r);
    auto it_end = std::ranges::end(r);

    if (it == it_end) return;// empty range

    using ValT = std::remove_reference_t<decltype(*it)>;
    ValT start = *it;

    std::size_t count = 0;
    for (auto jt = it; jt != it_end; ++jt) ++count;

    // Treat the range as a consecutive [start, start + count) sequence.
    poet::dynamic_for<Unroll>(start, static_cast<ValT>(start + static_cast<ValT>(count)), ad.func);
}

template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
    auto [b, e, s] = t;
    poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

template<std::size_t U, typename F> constexpr auto make_dynamic_for(F &&f) -> dynamic_for_adaptor<std::decay_t<F>, U> {
    return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

}// namespace poet
#endif// __cplusplus >= 202002L
// END_FILE: include/poet/core/dynamic_for.hpp
/* End inline (angle): include/poet/core/dynamic_for.hpp */
/* Begin inline (angle): include/poet/core/dispatch.hpp */
// BEGIN_FILE: include/poet/core/dispatch.hpp

/// \file dispatch.hpp
/// \brief Runtime-to-compile-time dispatch for integer choices and tuples.

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */
/* Begin inline (angle): include/poet/core/mdspan_utils.hpp */
// BEGIN_FILE: include/poet/core/mdspan_utils.hpp

/// \file mdspan_utils.hpp
/// \brief Multidimensional index utilities for N-D dispatch table generation.
///
/// Provides row-major stride computation and total-size calculation used by
/// the N-D function-pointer-table dispatch in dispatch.hpp.

#include <array>
#include <cstddef>
/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet::detail {

/// Total size (product of all dimensions).
template<std::size_t N>
POET_CPP20_CONSTEVAL auto compute_total_size(const std::array<std::size_t, N> &dims) -> std::size_t {
    std::size_t total = 1;
    for (std::size_t i = 0; i < N; ++i) { total *= dims[i]; }
    return total;
}

/// Compute row-major strides. stride[i] = product of dims[i+1..N-1].
template<std::size_t N>
POET_CPP20_CONSTEVAL auto compute_strides(const std::array<std::size_t, N> &dims) -> std::array<std::size_t, N> {
    std::array<std::size_t, N> strides{};
    if constexpr (N > 0) {
        strides[N - 1] = 1;
        for (std::size_t i = N - 1; i > 0; --i) { strides[i - 1] = strides[i] * dims[i]; }
    }
    return strides;
}

}// namespace poet::detail
// END_FILE: include/poet/core/mdspan_utils.hpp
/* End inline (angle): include/poet/core/mdspan_utils.hpp */

namespace poet {

/// \brief Concise tuple syntax for `dispatch_set`.
template<auto... Vs> struct tuple_ {};

namespace detail {

    template<typename T>
    using result_holder = std::conditional_t<std::is_void_v<T>, std::optional<std::monostate>, std::optional<T>>;

    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args> struct seq_matcher;

    template<typename ValueType,
      ValueType... V,
      typename ResultType,
      typename RuntimeTuple,
      typename Functor,
      typename... Args>
    struct seq_matcher<std::integer_sequence<ValueType, V...>, ResultType, RuntimeTuple, Functor, Args...> {
        template<std::size_t... Idx, typename F>
        static auto
          impl(std::index_sequence<Idx...> /*idx_seq*/, const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
            -> result_holder<ResultType> {
            result_holder<ResultType> res;
            // Short-circuiting AND fold: all runtime slots must equal their compile-time counterparts.
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                if constexpr (std::is_void_v<ResultType>) {
                    std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                    res = std::monostate{};
                } else {
                    res = std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                }
            }
            return res;
        }

        template<typename F>
        static auto match_and_call(const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
          -> result_holder<ResultType> {
            return impl(std::make_index_sequence<sizeof...(V)>{},
              runtime_tuple,
              std::forward<F>(func),
              std::forward<Args>(args)...);
        }
    };

    template<typename Seq, typename Functor, typename... Args> struct seq_call_result;

    template<typename ValueType, ValueType... V, typename Functor, typename... Args>
    struct seq_call_result<std::integer_sequence<ValueType, V...>, Functor, Args...> {
        using type = decltype(std::declval<Functor>().template operator()<V...>(std::declval<Args>()...));
    };

    template<int Start, int... Is>
    auto inclusive_range_impl(std::integer_sequence<int, Is...>) -> std::integer_sequence<int, (Start + Is)...>;

}// namespace detail

/// \brief Inclusive integer sequence `[Start, End]`.
template<int Start, int End>
using inclusive_range =
  decltype(detail::inclusive_range_impl<Start>(std::make_integer_sequence<int, End - Start + 1>{}));

/// \brief Runtime value paired with the compile-time candidates to probe.
template<typename Seq> struct dispatch_param {
    int runtime_val;
    using seq_type = Seq;
};

namespace detail {
    template<typename T> struct is_dispatch_param : std::false_type {};
    template<typename Seq> struct is_dispatch_param<dispatch_param<Seq>> : std::true_type {};

    template<typename T> inline constexpr bool is_dispatch_param_v = is_dispatch_param<std::decay_t<T>>::value;

    template<typename T> struct is_dispatch_param_tuple : std::false_type {};

    template<typename... Ts>
    struct is_dispatch_param_tuple<std::tuple<Ts...>> : std::bool_constant<(is_dispatch_param_v<Ts> && ...)> {};

    template<typename T>
    inline constexpr bool is_dispatch_param_tuple_v = is_dispatch_param_tuple<std::decay_t<T>>::value;
}// namespace detail

namespace detail {

    template<typename Sequence> struct sequence_size;

    template<typename T, T... Values>
    struct sequence_size<std::integer_sequence<T, Values...>>
      : std::integral_constant<std::size_t, sizeof...(Values)> {};

    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<int First, int... Rest>
    struct is_contiguous_sequence<std::integer_sequence<int, First, Rest...>>
      : std::bool_constant<(
          std::max({ First, Rest... }) - std::min({ First, Rest... }) + 1 == static_cast<int>(1 + sizeof...(Rest)))> {};

    template<typename ParamTuple, typename = std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>>
    struct all_contiguous;

    template<typename ParamTuple, std::size_t... Idx>
    struct all_contiguous<ParamTuple, std::index_sequence<Idx...>>
      : std::bool_constant<(
          is_contiguous_sequence<typename std::tuple_element_t<Idx, std::decay_t<ParamTuple>>::seq_type>::value
          && ...)> {};

    template<typename ParamTuple> inline constexpr bool all_contiguous_v = all_contiguous<ParamTuple>::value;

    template<typename Sequence> struct sequence_first;

    template<typename Seq> struct sparse_index;

    template<int... Values> struct sparse_index<std::integer_sequence<int, Values...>> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<int, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        // Insertion sort that carries original positions alongside keys, so the dispatch table
        // preserves user-declared slot order while lookups can use ordered search (binary/strided).
        static constexpr sorted_data_t sorted_data = []() constexpr -> sorted_data_t {
            sorted_data_t out{};
            out.sorted_keys = std::array<int, value_count>{ Values... };
            for (std::size_t i = 0; i < value_count; ++i) { out.sorted_indices[i] = i; }
            for (std::size_t i = 1; i < value_count; ++i) {
                const int current_key = out.sorted_keys[i];
                const std::size_t current_index = out.sorted_indices[i];
                std::size_t insert_pos = i;
                // Shift larger keys (and their original-position tags) right in lockstep
                // until we find the slot where `current_key` belongs.
                while (insert_pos > 0 && out.sorted_keys[insert_pos - 1] > current_key) {
                    out.sorted_keys[insert_pos] = out.sorted_keys[insert_pos - 1];
                    out.sorted_indices[insert_pos] = out.sorted_indices[insert_pos - 1];
                    --insert_pos;
                }
                out.sorted_keys[insert_pos] = current_key;
                out.sorted_indices[insert_pos] = current_index;
            }
            return out;
        }();

        static constexpr std::size_t unique_count = []() constexpr -> std::size_t {
            if constexpr (value_count == 0) { return 0; }
            std::size_t count = 1;
            for (std::size_t i = 1; i < value_count; ++i) {
                if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) { ++count; }
            }
            return count;
        }();

        static constexpr std::array<int, unique_count> keys = []() constexpr -> std::array<int, unique_count> {
            std::array<int, unique_count> out{};
            if constexpr (value_count > 0) {
                std::size_t out_i = 0;
                out[out_i++] = sorted_data.sorted_keys[0];
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        out[out_i++] = sorted_data.sorted_keys[i];
                    }
                }
            }
            return out;
        }();

        static constexpr std::array<std::size_t, unique_count> indices =
          []() constexpr -> std::array<std::size_t, unique_count> {
            std::array<std::size_t, unique_count> out{};
            if constexpr (value_count > 0) {
                std::size_t out_i = 0;
                out[out_i++] = sorted_data.sorted_indices[0];
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        out[out_i++] = sorted_data.sorted_indices[i];
                    }
                }
            }
            return out;
        }();
    };

    inline constexpr std::size_t dispatch_npos = static_cast<std::size_t>(-1);

    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct seq_lookup;

    template<int... Values> struct seq_lookup<std::integer_sequence<int, Values...>, true> {
        static constexpr int first = sequence_first<std::integer_sequence<int, Values...>>::value;
        static constexpr std::size_t len = sizeof...(Values);
        static constexpr bool ascending = (first == std::min({ Values... }));

        static POET_FORCEINLINE auto find(int value) -> std::size_t {
            // Unsigned subtraction folds "below first" into "far above len", so a single
            // `idx < len` check handles both underflow and overflow with no extra branch.
            std::size_t idx = 0;
            if constexpr (ascending) {
                idx = static_cast<std::size_t>(static_cast<unsigned int>(value) - static_cast<unsigned int>(first));
            } else {
                idx = static_cast<std::size_t>(static_cast<unsigned int>(first) - static_cast<unsigned int>(value));
            }
            if (POET_LIKELY(idx < len)) { return idx; }
            return dispatch_npos;
        }
    };

    // Non-contiguous sequences: detect a uniform positive stride at compile time and
    // specialise `find` to a div/mod (strided) instead of a binary search (truly sparse).
    template<int... Values> struct seq_lookup<std::integer_sequence<int, Values...>, false> {
        using sparse_data = sparse_index<std::integer_sequence<int, Values...>>;

        static constexpr bool is_strided = []() constexpr -> bool {
            if constexpr (sparse_data::unique_count < 2) {
                return false;
            } else {
                // Reject non-positive strides up front so `find` can use unsigned math.
                constexpr int stride0 = sparse_data::keys[1] - sparse_data::keys[0];
                if constexpr (stride0 <= 0) { return false; }
                // cppcheck-suppress syntaxError
                // All adjacent gaps must match `stride0`, otherwise fall back to binary search.
                for (std::size_t i = 2; i < sparse_data::unique_count; ++i) {
                    if (sparse_data::keys[i] - sparse_data::keys[i - 1] != stride0) { return false; }
                }
                return true;
            }
        }();

        static POET_FORCEINLINE auto find(int value) -> std::size_t {
            if constexpr (is_strided) {
                static constexpr int first = sparse_data::keys[0];
                static constexpr int stride = sparse_data::keys[1] - sparse_data::keys[0];
                const int diff = value - first;
                // Miss when below range or not aligned to the stride grid.
                if (diff < 0 || diff % stride != 0) { return dispatch_npos; }
                const auto idx = static_cast<std::size_t>(diff / stride);
                // Remap sorted position back to the user's declared slot.
                if (POET_LIKELY(idx < sparse_data::unique_count)) { return sparse_data::indices[idx]; }
                return dispatch_npos;
            } else {
                // Sorted keys → binary search; `indices` undoes the sort to the original slot.
                const auto pos = std::lower_bound(sparse_data::keys.begin(), sparse_data::keys.end(), value);
                if (pos != sparse_data::keys.end() && *pos == value) {
                    return sparse_data::indices[static_cast<std::size_t>(pos - sparse_data::keys.begin())];
                }
                return dispatch_npos;
            }
        }
    };

    template<int First, int... Rest>
    struct sequence_first<std::integer_sequence<int, First, Rest...>> : std::integral_constant<int, First> {};

    template<typename ParamTuple, std::size_t... Idx>
    POET_CPP20_CONSTEVAL auto dimensions_of_impl(std::index_sequence<Idx...> /*idxs*/)
      -> std::array<std::size_t, sizeof...(Idx)> {
        using P = std::decay_t<ParamTuple>;
        return std::array<std::size_t, sizeof...(Idx)>{
            sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value...
        };
    }

    template<typename ParamTuple>
    POET_CPP20_CONSTEVAL auto dimensions_of() -> std::array<std::size_t, std::tuple_size_v<std::decay_t<ParamTuple>>> {
        return dimensions_of_impl<ParamTuple>(std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
    }

    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index_sparse(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/)
      -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        const std::array<std::size_t, sizeof...(Idx)> indices = {
            seq_lookup<typename std::tuple_element_t<Idx, P>::seq_type>::find(std::get<Idx>(params).runtime_val)...
        };

        const bool all_hit = ((indices[Idx] != dispatch_npos) && ...);
        if (POET_UNLIKELY(!all_hit)) { return dispatch_npos; }

        return ((indices[Idx] * strides[Idx]) + ...);
    }

    template<typename Seq> POET_FORCEINLINE constexpr auto contiguous_offset(int value) noexcept -> std::size_t {
        constexpr auto ufirst = static_cast<unsigned int>(sequence_first<Seq>::value);
        const auto uval = static_cast<unsigned int>(value);
        if constexpr (seq_lookup<Seq>::ascending) {
            return static_cast<std::size_t>(uval - ufirst);
        } else {
            return static_cast<std::size_t>(ufirst - uval);
        }
    }

    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index_contiguous(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/)
      -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        const std::array<std::size_t, sizeof...(Idx)> mapped = {
            contiguous_offset<typename std::tuple_element_t<Idx, P>::seq_type>(std::get<Idx>(params).runtime_val)...
        };

        // Bitwise-OR fold (not logical) so each bound check is evaluated branch-free;
        // the aggregate OOB flag is consumed once at the bottom.
        const std::size_t oob = (static_cast<std::size_t>(
                                   mapped[Idx] >= sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value)
                                 | ...);

        const std::size_t flat = ((mapped[Idx] * strides[Idx]) + ...);

        return POET_LIKELY(oob == 0) ? flat : dispatch_npos;
    }

    template<typename ParamTuple> POET_FORCEINLINE auto extract_flat_index(const ParamTuple &params) -> std::size_t {
        constexpr std::size_t num_dims = std::tuple_size_v<std::decay_t<ParamTuple>>;
        if constexpr (all_contiguous_v<ParamTuple>) {
            return flat_index_contiguous(params, std::make_index_sequence<num_dims>{});
        } else {
            return flat_index_sparse(params, std::make_index_sequence<num_dims>{});
        }
    }

    template<typename A, typename B> struct seq_equal;
    template<typename T, T... A, T... B>
    struct seq_equal<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>>
      : std::bool_constant<((A == B) && ...)> {};

    template<typename... S> struct unique_helper;
    template<> struct unique_helper<> : std::true_type {};
    template<typename Head, typename... Rest>
    struct unique_helper<Head, Rest...>
      : std::bool_constant<(!(seq_equal<Head, Rest>::value || ...) && unique_helper<Rest...>::value)> {};

    template<typename T> struct is_tuple : std::false_type {};
    template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    template<typename S> POET_CPP20_CONSTEVAL auto as_seq_tuple(S /*seq*/) {
        if constexpr (is_tuple<S>::value) {
            return S{};
        } else {
            return std::tuple<S>{ S{} };
        }
    }

    template<typename Tuple, std::size_t... Indices>
    POET_CPP20_CONSTEVAL auto extract_sequences_impl(std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        return std::tuple_cat(as_seq_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{})...);
    }

    template<typename Tuple> POET_CPP20_CONSTEVAL auto extract_sequences() {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl<TupleType>(std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

    // Computes the functor's return type by probing both calling conventions the dispatcher
    // supports: `func(integral_constant<int, V>{}, args...)` (value form) and
    // `func.template operator()<V>(args...)` (template form). Value form is preferred when viable.
    template<typename Functor, typename... Seq> struct dispatch_result_helper {
        // First preference: value-argument form (passes std::integral_constant values as parameters).
        template<typename... Args>
        static auto compute_impl(std::true_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>()(std::integral_constant<int, sequence_first<Seq>::value>{}...,
            std::declval<Args>()...));

        // Fallback: template-parameter form.
        template<typename... Args>
        static auto compute_impl(std::false_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>().template operator()<sequence_first<Seq>::value...>(
            std::declval<Args>()...));

        // Detection of value-argument viability using std::is_invocable
        template<typename... Args>
        static auto compute() -> decltype(compute_impl<Args...>(std::integral_constant<bool,
          std::is_invocable_v<Functor &, std::integral_constant<int, sequence_first<Seq>::value>..., Args...>>{})) {
            return compute_impl<Args...>(std::integral_constant<bool,
              std::is_invocable_v<Functor &, std::integral_constant<int, sequence_first<Seq>::value>..., Args...>>{});
        }
    };

    template<typename Functor, typename SequenceTuple, typename... Args> struct dispatch_result;

    template<typename Functor, typename... Seq, typename... Args>
    struct dispatch_result<Functor, std::tuple<Seq...>, Args...> {
        using type = decltype(dispatch_result_helper<Functor, Seq...>::template compute<Args...>());
    };

    template<typename Functor, typename SequenceTuple, typename... Args>
    using dispatch_result_t = typename dispatch_result<Functor, SequenceTuple, Args...>::type;

    template<typename... Args> struct arg_pack {};

    template<typename T>
    inline constexpr bool is_stateless_v = std::is_empty_v<T> && std::is_default_constructible_v<T>;

    // Picks the calling convention for each forwarded arg through the function-pointer table.
    // Small trivially-copyable rvalue/const-lvalue args are passed by value (cheaper than
    // synthesising a reference); everything else keeps its original reference category.
    template<typename T> struct arg_pass {
        using raw = std::remove_reference_t<T>;
        using raw_unqual = std::remove_cv_t<raw>;
        static constexpr bool is_small_trivial =
          std::is_trivially_copyable_v<raw_unqual> && (sizeof(raw_unqual) <= 2 * sizeof(void *));

        static constexpr bool caller_allows_copy =
          std::is_rvalue_reference_v<T> || (std::is_lvalue_reference_v<T> && std::is_const_v<raw>);

        static constexpr bool by_value = is_small_trivial && caller_allows_copy;

        using type = std::conditional_t<by_value, raw_unqual, T>;
    };

    template<typename T> using pass_t = typename arg_pass<T>::type;

    template<typename Functor, int Value, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, int Value, typename... Args>
    struct can_use_value_form<Functor, Value, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, std::integral_constant<int, Value>, Args &&...>> {};

    template<typename Functor, typename ArgPack, typename R, int... Values> struct table_builder;

    template<typename Functor, typename... Args, typename R, int... Values>
    struct table_builder<Functor, arg_pack<Args...>, R, Values...> {
        static constexpr int first_value = sequence_first<std::integer_sequence<int, Values...>>::value;

        // Each entry is a plain function pointer. Stateless functors are default-constructed
        // inside the thunk (no closure needed); stateful functors take the functor by ref so
        // the signature stays identical across all entries in the array.
        template<int V> static POET_CPP20_CONSTEVAL auto make_entry() {
            if constexpr (is_stateless_v<Functor>) {
                return +[](pass_t<Args &&>... args) -> R {
                    Functor func{};
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, std::forward<Args>(args)...);
                    } else {
                        return func.template operator()<V>(std::forward<Args>(args)...);
                    }
                };
            } else {
                return +[](Functor &func, pass_t<Args &&>... args) -> R {
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, std::forward<Args>(args)...);
                    } else {
                        return func.template operator()<V>(std::forward<Args>(args)...);
                    }
                };
            }
        }

        static POET_CPP20_CONSTEVAL auto make() {
            using fn_type = decltype(make_entry<first_value>());
            return std::array<fn_type, sizeof...(Values)>{ make_entry<Values>()... };
        }
    };

    template<typename Functor, typename ArgPack, typename R, int... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<int, Values...> /*seq*/) {
        return table_builder<Functor, ArgPack, R, Values...>::make();
    }

    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq> struct nd_table_builder;

    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct nd_table_builder<Functor, arg_pack<Args...>, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        static constexpr std::array<std::size_t, sizeof...(Seqs)> dims_ = { sequence_size<Seqs>::value... };
        static constexpr std::array<std::size_t, sizeof...(Seqs)> strides_ = compute_strides(dims_);

        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, int... Values> struct get_sequence_value<I, std::integer_sequence<int, Values...>> {
            static constexpr std::array<int, sizeof...(Values)> values = { Values... };
            static constexpr int value = values[I];
        };

        // Decode a flat table index back to its per-dimension coordinate via row-major strides.
        template<std::size_t FlatIdx, std::size_t DimIdx>
        static constexpr std::size_t dim_index_v = FlatIdx / strides_[DimIdx] % dims_[DimIdx];

        // For a given flat index, materialises the tuple of per-dim values as an array and
        // exposes each as `integral_constant<int, V>` via `ic<N>` — that's what the functor sees.
        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            static constexpr std::array<int, sizeof...(SeqIdx)> values = {
                get_sequence_value<dim_index_v<FlatIdx, SeqIdx>,
                  std::tuple_element_t<SeqIdx, std::tuple<Seqs...>>>::value...
            };

            template<std::size_t N> using ic = std::integral_constant<int, values[N]>;
        };

        template<std::size_t FlatIdx> struct nd_index_caller {
            template<std::size_t... Is>
            static auto make_ve(std::index_sequence<Is...>) -> value_extractor<FlatIdx, Is...>;
            using VE = decltype(make_ve(std::make_index_sequence<sizeof...(Seqs)>{}));

            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto invoke(Functor &func, std::index_sequence<SeqIdx...> /*idx*/, Args &&...args)
              -> R {
                using VE_local = value_extractor<FlatIdx, SeqIdx...>;
                constexpr bool use_value_form =
                  std::is_invocable_v<Functor &, typename VE_local::template ic<SeqIdx>..., Args &&...>;
                if constexpr (use_value_form) {
                    return func(typename VE_local::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                } else {
                    return func.template operator()<VE_local::template ic<SeqIdx>::value...>(
                      std::forward<Args>(args)...);
                }
            }

            template<typename R> static POET_FORCEINLINE auto call(Functor &func, pass_t<Args &&>... args) -> R {
                return invoke<R>(func, std::make_index_sequence<sizeof...(Seqs)>{}, std::forward<Args>(args)...);
            }

            template<typename R> static POET_FORCEINLINE auto call_stateless(pass_t<Args &&>... args) -> R {
                Functor func{};
                return invoke<R>(func, std::make_index_sequence<sizeof...(Seqs)>{}, std::forward<Args>(args)...);
            }
        };

        template<typename R> static constexpr auto make_table() {
            if constexpr (is_stateless_v<Functor>) {
                using fn_type = decltype(&nd_index_caller<0>::template call_stateless<R>);
                return std::array<fn_type, sizeof...(FlatIndices)>{
                    &nd_index_caller<FlatIndices>::template call_stateless<R>...
                };
            } else {
                using fn_type = decltype(&nd_index_caller<0>::template call<R>);
                return std::array<fn_type, sizeof...(FlatIndices)>{
                    &nd_index_caller<FlatIndices>::template call<R>...
                };
            }
        }
    };

    template<typename Functor, typename ArgPack, typename R, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table(std::tuple<Seqs...> /*seqs*/) {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return nd_table_builder<Functor, ArgPack, std::tuple<Seqs...>, std::make_index_sequence<total_size>>::
          template make_table<R>();
    }

}// namespace detail

/// \brief Exact set of allowed tuples for sparse dispatch.
template<typename ValueType, typename... Tuples> struct dispatch_set {
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<tuple_<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;
    using first_t = std::tuple_element_t<0, seq_type>;

    static_assert(sizeof...(Tuples) >= 1, "dispatch_set requires at least one allowed tuple");

    static constexpr std::size_t tuple_arity = detail::sequence_size<first_t>::value;

    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in dispatch_set must have the same arity");

    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "dispatch_set contains duplicate allowed tuples");

    using runtime_array_t = std::array<ValueType, tuple_arity>;

  private:
    runtime_array_t runtime_val;

  public:
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit dispatch_set(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_on_no_match{};

/// \brief Thrown when a `throw_on_no_match` dispatch has no matching specialization.
struct no_match_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

    POET_PUSH_OPTIMIZE

    template<typename R, typename EntryFn, typename FunctorFwd, typename... Args>
    POET_FORCEINLINE auto invoke_table_entry(FunctorFwd &functor, EntryFn entry, Args &&...args) -> R {
        using FT = std::decay_t<FunctorFwd>;
        if constexpr (is_stateless_v<FT>) {
            if constexpr (std::is_void_v<R>) {
                entry(std::forward<Args>(args)...);
                return;
            } else {
                return entry(std::forward<Args>(args)...);
            }
        } else {
            if constexpr (std::is_void_v<R>) {
                entry(static_cast<FT &>(functor), std::forward<Args>(args)...);
                return;
            } else {
                return entry(static_cast<FT &>(functor), std::forward<Args>(args)...);
            }
        }
    }

    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_1d(Functor &functor, ParamTuple const &params, Args &&...args) -> R {
        using FirstParam = std::tuple_element_t<0, std::remove_reference_t<ParamTuple>>;
        using Seq = typename FirstParam::seq_type;
        const int runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = seq_lookup<Seq>::find(runtime_val);

        if (POET_LIKELY(idx != dispatch_npos)) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_dispatch_table<FunctorT, arg_pack<Args...>, R>(Seq{});
            return invoke_table_entry<R>(functor, table[idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time combination for runtime inputs");
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_nd(Functor &functor, ParamTuple const &params, Args &&...args) -> R {
        constexpr auto dimensions = dimensions_of<ParamTuple>();
        constexpr std::size_t table_size = compute_total_size(dimensions);

        const std::size_t flat_idx = extract_flat_index(params);
        if (POET_LIKELY(flat_idx != dispatch_npos)) {
            POET_ASSUME(flat_idx < table_size);

            using sequences_t = decltype(extract_sequences<ParamTuple>());
            static constexpr sequences_t sequences{};

            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(sequences);
            return invoke_table_entry<R>(functor, table[flat_idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time combination for runtime inputs");
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_impl(Functor &functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
        constexpr std::size_t param_count = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        using sequences_t = decltype(extract_sequences<ParamTuple>());
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

namespace detail {
    template<typename... Ts> struct leading_param_count;

    template<> struct leading_param_count<> {
        static constexpr std::size_t value = 0;
    };

    template<typename First, typename... Rest> struct leading_param_count<First, Rest...> {
        static constexpr std::size_t value = is_dispatch_param_v<First> ? (1 + leading_param_count<Rest...>::value) : 0;
    };

    template<bool ThrowOnNoMatch, typename Functor, std::size_t... ParamIdx, std::size_t... ArgIdx, typename... All>
    POET_FORCEINLINE auto dispatch_split_impl(Functor &functor,
      std::index_sequence<ParamIdx...> /*p*/,
      std::index_sequence<ArgIdx...> /*a*/,
      All &&...all) -> decltype(auto) {

        constexpr std::size_t num_params = sizeof...(ParamIdx);
        // Reference-tuple view of the entire pack so we can index it twice without copies.
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        // Leading `num_params` entries are the dispatch_params → copy into a value tuple
        // (they're small structs holding a runtime int).
        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        // Remaining entries are forwarded with their original value categories preserved
        // via `std::move(all_refs)` (the references inside are unaffected).
        return dispatch_impl<ThrowOnNoMatch>(functor,
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    // Splits the variadic pack into [leading dispatch_params | trailing regular args] by
    // counting dispatch_param types until the first non-dispatch_param — everything after is
    // forwarded as plain args into the chosen specialisation.
    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor &functor, FirstParam &&first_param, Rest &&...rest)
      -> decltype(auto) {
        // `first_param` is known to be a dispatch_param (enable_if on the public overload);
        // count contiguous dispatch_params in the rest, the remainder is the regular arg pack.
        constexpr std::size_t num_params = 1 + leading_param_count<Rest...>::value;
        constexpr std::size_t num_args = sizeof...(Rest) + 1 - num_params;

        if constexpr (num_args == 0) {
            auto params = std::make_tuple(std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
            return dispatch_impl<ThrowOnNoMatch>(functor, params);
        } else {
            return dispatch_split_impl<ThrowOnNoMatch>(functor,
              std::make_index_sequence<num_params>{},
              std::make_index_sequence<num_args>{},
              std::forward<FirstParam>(first_param),
              std::forward<Rest>(rest)...);
        }
    }
}// namespace detail

/// \brief Dispatches runtime integers to compile-time specializations.
///
/// Accepts either leading `dispatch_param` arguments or a tuple of them. On miss,
/// the non-throwing overload returns `void` or a default-constructed result.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid
                                // copy; internally always used by lvalue ref
  FirstParam &&first_param,
  Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<false>(
      functor, std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Tuple overload for `dispatch_param` dispatch.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid
                                // copy; internally always used by lvalue ref
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<false>(functor, params, std::forward<Args>(args)...);
}

namespace detail {
    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor &&functor,
      TupleList const & /*tl*/,
      const RuntimeTuple &runtime_tuple,
      Args &&...args)// NOLINT(cppcoreguidelines-missing-std-forward) forwarded inside short-circuiting fold
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type = typename seq_call_result<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

        result_holder<result_type> out;

        using FunctorT = std::decay_t<Functor>;
        FunctorT functor_copy(std::forward<Functor>(functor));

        const bool matched = std::apply(
          [&](auto... seqs) POET_ALWAYS_INLINE_LAMBDA -> bool {
              return ([&](auto &seq) POET_ALWAYS_INLINE_LAMBDA -> bool {
                  using SeqType = std::decay_t<decltype(seq)>;
                  auto result = seq_matcher<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
                    runtime_tuple, functor_copy, std::forward<Args>(args)...);

                  if (result.has_value()) {
                      out = std::move(result);
                      return true;
                  }
                  return false;
              }(seqs) || ...);
          },
          TL{});

        if (matched) {
            if constexpr (std::is_void_v<result_type>) {
                return;
            } else {
                return result_type(std::move(*out));
            }
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch_tuples: no matching compile-time tuple for runtime inputs");
        } else if constexpr (!std::is_void_v<result_type>) {
            return result_type{};
        }
    }
}// namespace detail

/// \brief Dispatches using a `dispatch_set`.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(Functor &&functor, const dispatch_set<Tuples...> &set, Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(std::forward<Functor>(functor),
      typename dispatch_set<Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `dispatch_set` dispatch.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/, Functor &&functor, const dispatch_set<Tuples...> &set, Args &&...args)
  -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(std::forward<Functor>(functor),
      typename dispatch_set<Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing `dispatch_param` overload.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid copy;
                    // internally always used by lvalue ref
  FirstParam &&first_param,
  Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<true>(
      functor, std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Throwing tuple overload for `dispatch_param` dispatch.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward) — accepted as universal ref to avoid copy;
                    // internally always used by lvalue ref
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<true>(functor, params, std::forward<Args>(args)...);
}

}// namespace poet
// END_FILE: include/poet/core/dispatch.hpp
/* End inline (angle): include/poet/core/dispatch.hpp */
/* Begin inline (angle): include/poet/core/static_for.hpp */
// BEGIN_FILE: include/poet/core/static_for.hpp

/// \file static_for.hpp
/// \brief Compile-time loop unrolling over integer ranges.

#include <cstddef>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/for_utils.hpp */
// BEGIN_FILE: include/poet/core/for_utils.hpp

/// \file for_utils.hpp
/// \brief Internal helpers shared by the loop primitives.

#include <cstddef>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet::detail {

template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::ptrdiff_t Step>
[[nodiscard]] POET_CPP20_CONSTEVAL auto compute_range_count() noexcept -> std::size_t {
    static_assert(Step != 0, "static_for requires a non-zero step");
    if constexpr (Step > 0) {
        static_assert(Begin <= End, "static_for with a positive step requires Begin <= End");
    } else {
        static_assert(Begin >= End, "static_for with a negative step requires Begin >= End");
    }
    if constexpr (Begin == End) { return 0; }
    constexpr auto distance = (End - Begin) < 0 ? -(End - Begin) : (End - Begin);
    constexpr auto magnitude = Step < 0 ? -Step : Step;
    return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
}

template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_FORCEINLINE constexpr auto run_block(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr std::ptrdiff_t Base = Begin + (Step * static_cast<std::ptrdiff_t>(StartIndex));
    (func(std::integral_constant<std::ptrdiff_t, Base + (Step * static_cast<std::ptrdiff_t>(Is))>{}), ...);
}

POET_PUSH_OPTIMIZE

template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_NOINLINE_FLATTEN constexpr auto run_block_iso(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr std::ptrdiff_t Base = Begin + (Step * static_cast<std::ptrdiff_t>(StartIndex));
    (func(std::integral_constant<std::ptrdiff_t, Base + (Step * static_cast<std::ptrdiff_t>(Is))>{}), ...);
}

template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t BlockSize, std::size_t... Is>
POET_FORCEINLINE constexpr auto emit_blocks(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    (run_block<Func, Begin, Step, Is * BlockSize>(func, std::make_index_sequence<BlockSize>{}), ...);
}

template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t BlockSize, std::size_t... Is>
POET_FORCEINLINE constexpr auto emit_blocks_iso(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    (run_block_iso<Func, Begin, Step, Is * BlockSize>(func, std::make_index_sequence<BlockSize>{}), ...);
}

POET_POP_OPTIMIZE

template<typename Functor> struct template_invoker {
    Functor &functor;

    template<std::ptrdiff_t Value>
    POET_FORCEINLINE constexpr auto operator()(std::integral_constant<std::ptrdiff_t, Value> /*ic*/) const -> void {
        functor.template operator()<Value>();
    }
};

}// namespace poet::detail
// END_FILE: include/poet/core/for_utils.hpp
/* End inline (angle): include/poet/core/for_utils.hpp */

namespace poet {

namespace detail {

    template<typename Callable,
      std::ptrdiff_t Begin,
      std::ptrdiff_t Step,
      std::size_t BlockSize,
      std::size_t FullBlocks,
      std::size_t Remainder>
    POET_FORCEINLINE constexpr void run_blocks(Callable &callable) {
        if constexpr (FullBlocks > 0) {
            if constexpr (FullBlocks > 1) {
                emit_blocks_iso<Callable, Begin, Step, BlockSize>(callable, std::make_index_sequence<FullBlocks>{});
            } else {
                emit_blocks<Callable, Begin, Step, BlockSize>(callable, std::make_index_sequence<FullBlocks>{});
            }
        }

        if constexpr (Remainder > 0) {
            run_block<Callable, Begin, Step, FullBlocks * BlockSize>(callable, std::make_index_sequence<Remainder>{});
        }
    }

    template<std::ptrdiff_t Begin, std::ptrdiff_t End, std::ptrdiff_t Step>
    POET_CPP20_CONSTEVAL auto default_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        return count == 0 ? 1 : count;
    }

}// namespace detail

/// \brief Runs a compile-time unrolled loop over `[Begin, End)`.
///
/// `func` may take `std::integral_constant<std::ptrdiff_t, I>` or expose
/// `template <auto I> operator()()`. `BlockSize` defaults to the full range;
/// pass a smaller value to isolate heavier bodies into separate outlined blocks.
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
///                   total iteration count, or `1` for empty ranges).
/// \tparam Func Callable type.
/// \param func Callable instance invoked once per iteration.
template<std::ptrdiff_t Begin,
  std::ptrdiff_t End,
  std::ptrdiff_t Step = 1,
  std::size_t BlockSize = detail::default_block_size<Begin, End, Step>(),
  typename Func>
POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_assert(BlockSize > 0, "static_for requires BlockSize > 0");

    constexpr auto count = detail::compute_range_count<Begin, End, Step>();
    if constexpr (count == 0) { return; }

    constexpr auto full_blocks = count / BlockSize;
    constexpr auto remainder = count % BlockSize;

    using callable_t = std::remove_reference_t<Func>;

    auto do_for = [&](auto &ref) POET_ALWAYS_INLINE_LAMBDA -> void {
        if constexpr (std::is_invocable_v<callable_t &, std::integral_constant<std::ptrdiff_t, Begin>>) {
            detail::run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder>(ref);
        } else {
            using invoker_t = detail::template_invoker<callable_t>;
            invoker_t invoker{ ref };
            detail::run_blocks<invoker_t, Begin, Step, BlockSize, full_blocks, remainder>(invoker);
        }
    };

    if constexpr (std::is_lvalue_reference_v<Func>) {
        do_for(func);
    } else {
        callable_t callable(std::forward<Func>(func));
        do_for(callable);
    }
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/// \brief Convenience overload for `static_for<0, End>(func)`.
template<std::ptrdiff_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}
#endif

}// namespace poet
// END_FILE: include/poet/core/static_for.hpp
/* End inline (angle): include/poet/core/static_for.hpp */
/* Begin inline (angle): include/poet/core/undef_macros.hpp */
// BEGIN_FILE: include/poet/core/undef_macros.hpp
// NOLINTBEGIN(llvm-header-guard)
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

// END_FILE: include/poet/core/undef_macros.hpp
/* End inline (angle): include/poet/core/undef_macros.hpp */
// NOLINTEND(llvm-include-order)
// clang-format on
// END_FILE: include/poet/poet.hpp

#endif // POET_SINGLE_HEADER_GOLDBOT_HPP
