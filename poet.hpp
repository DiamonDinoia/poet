/* Auto-generated single-header. Do not edit directly. */

#ifndef POET_SINGLE_HEADER_GOLDBOT_HPP
#define POET_SINGLE_HEADER_GOLDBOT_HPP

// BEGIN_FILE: include/poet/poet.hpp

/// \file poet.hpp
/// \brief Umbrella header for the public POET API.

// clang-format off
// macros.hpp comes first: the other headers use its macros. undef_macros.hpp comes last: it removes them.
// NOLINTBEGIN(llvm-include-order)
/* Begin inline (angle): include/poet/core/macros.hpp */
// BEGIN_FILE: include/poet/core/macros.hpp

/// \file macros.hpp
/// \brief Compiler-specific macros for portability and optimization.

// --- POET_CPLUSPLUS ---
/// The language standard in effect. MSVC leaves `__cplusplus` at 199711L
/// unless `/Zc:__cplusplus` is passed, so testing it directly hides C++20
/// code paths from MSVC users.
#ifdef _MSVC_LANG
#define POET_CPLUSPLUS _MSVC_LANG// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_CPLUSPLUS __cplusplus// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_UNREACHABLE ---
/// Marks a code path as unreachable. UB if reached at runtime.
#if defined(__GNUC__) || defined(__clang__)
#define POET_UNREACHABLE() __builtin_unreachable()// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER)
#define POET_UNREACHABLE() __assume(false)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_UNREACHABLE() \
    do {                   \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_FORCEINLINE ---
/// Forces function inlining regardless of compiler heuristics.
#ifdef _MSC_VER
#define POET_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define POET_FORCEINLINE inline __attribute__((always_inline))
#else
#define POET_FORCEINLINE inline
#endif

// --- POET_ALWAYS_INLINE_LAMBDA ---
/// Forces inlining of a lambda's call operator, after the parameter list.
/// Attribute syntax is the only form the call operator accepts.
/// GCC 15+ / Clang 22+: assign an attributed generic lambda to a variable
/// before passing it to a template function.
#if defined(_MSC_VER) && !defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
#else
#define POET_ALWAYS_INLINE_LAMBDA
#endif

// --- POET_NOINLINE_FLATTEN ---
/// Keeps a function out of its caller (register isolation) while inlining
/// everything it calls. Without `flatten`, GCC's ISRA pass clones each functor
/// `operator()` out of line, reloading its constants per call; clang needs none.
#ifdef _MSC_VER
#define POET_NOINLINE_FLATTEN __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define POET_NOINLINE_FLATTEN __attribute__((noinline, flatten))
#else
#define POET_NOINLINE_FLATTEN
#endif

// --- POET_LIKELY / POET_UNLIKELY ---
/// Branch prediction hints. Use for conditions true/false >95% of the time.
#if defined(__GNUC__) || defined(__clang__)
#define POET_LIKELY(x) __builtin_expect(!!(x), 1)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) __builtin_expect(!!(x), 0)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_LIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- poet::detail::count_trailing_zeros ---
/// Counts trailing zero bits of a std::size_t. UB if value is 0.
/// Own guard: re-inclusion after undef_macros.hpp must define it only once.
#ifndef POET_COUNT_TRAILING_ZEROS_DEFINED
#define POET_COUNT_TRAILING_ZEROS_DEFINED

#include <cstddef>

#if POET_CPLUSPLUS >= 202002L
#include <bit>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

namespace poet::detail {

#if POET_CPLUSPLUS >= 202002L

constexpr auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    return static_cast<unsigned int>(std::countr_zero(value));
}

#elif defined(__GNUC__) || defined(__clang__)

constexpr auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    static_assert(sizeof(std::size_t) <= sizeof(unsigned long long), "unsupported std::size_t width");
    if constexpr (sizeof(std::size_t) <= sizeof(unsigned int)) {
        return static_cast<unsigned int>(__builtin_ctz(static_cast<unsigned int>(value)));
    } else {
        return static_cast<unsigned int>(__builtin_ctzll(static_cast<unsigned long long>(value)));
    }
}

#elif defined(_MSC_VER)

inline auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    unsigned long index = 0;
#if defined(_WIN64)
    _BitScanForward64(&index, static_cast<unsigned __int64>(value));
#else
    _BitScanForward(&index, static_cast<unsigned long>(value));
#endif
    return static_cast<unsigned int>(index);
}

#else

/// Portable fallback, width-agnostic. Reached only on C++17 compilers other
/// than GCC/Clang/MSVC, once per `dynamic_for` call with a non-constant
/// power-of-two stride.
constexpr auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    unsigned int count = 0;
    while ((value & std::size_t{ 1 }) == 0) {
        value >>= 1;
        ++count;
    }
    return count;
}

#endif

}// namespace poet::detail

#endif// POET_COUNT_TRAILING_ZEROS_DEFINED

// --- Optimization level detection ---
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER) && !defined(_DEBUG) && defined(NDEBUG)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_HIGH_OPTIMIZATION 0// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_HOT_LOOP ---
/// Marks hot-path functions for aggressive optimization and inlining.
#if defined(__GNUC__) || defined(__clang__)
#define POET_HOT_LOOP inline __attribute__((hot, always_inline))
#elif defined(_MSC_VER)
#define POET_HOT_LOOP __forceinline
#else
#define POET_HOT_LOOP inline
#endif

// --- POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE ---
/// Register-allocator tuning for hot paths, in push/pop pairs. Active only
/// when the build already optimizes for speed; it never raises the
/// optimization level. MSVC gets /Ogt; clang cannot enable optimizations via
/// pragma. Opt out with -DPOET_DISABLE_PUSH_OPTIMIZE.
#ifndef POET_DISABLE_PUSH_OPTIMIZE
#if defined(__GNUC__) && !defined(__clang__)
#if POET_HIGH_OPTIMIZATION
// Cheap vector cost model lets SLP pack unrolled accumulators; GCC 13/14 drop
// to 128-bit under AVX2 without the width pin, and pinning the SVE VL permits
// unrolling without predication. Width flags are machine flags: `target`, not
// `optimize`, and scoped to the push/pop.
#define POET_PUSH_OPTIMIZE_BASE_                                                                              \
    _Pragma("GCC push_options") _Pragma("GCC optimize(\"-fira-hoist-pressure\")")                             \
      _Pragma("GCC optimize(\"-fno-ira-share-spill-slots\")") _Pragma("GCC optimize(\"-frename-registers\")") \
        _Pragma("GCC optimize(\"-fvect-cost-model=cheap\")")

#if defined(__AVX512F__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=512\")")
#elif defined(__AVX2__) || defined(__AVX__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=256\")")
#elif defined(__ARM_FEATURE_SVE_BITS) && __ARM_FEATURE_SVE_BITS > 0
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
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#elif defined(_MSC_VER)
// /RTC1 is incompatible with /O2, so the optimize pragma applies to non-debug builds only.
#ifndef _DEBUG
#define POET_PUSH_OPTIMIZE __pragma(optimize("gt", on))
#define POET_POP_OPTIMIZE __pragma(optimize("", on))
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif

// --- C++20 Feature Detection ---
#if POET_CPLUSPLUS >= 202002L
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
/// Do not edit by hand; re-run CMake configure or cmake -P cmake/GenerateVersion.cmake.

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum)
#define POET_VERSION_MAJOR 0
#define POET_VERSION_MINOR 0
#define POET_VERSION_PATCH 2
#define POET_VERSION_STRING "0.0.2"
#define POET_VERSION_FULL "0.0.2-dev.6"
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

/// \file cpu_info.hpp
/// \brief Compile-time CPU register, vector-width, and cache-line queries.
///
/// Every value resolves from the compiler's target predefines; the result
/// describes the compile target, not the machine that runs the binary.

#include <cstddef>
/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet {

enum class instruction_set : unsigned char {
    generic,///< Baseline: no ISA predefines matched
    sse2,///< x86-64 SSE2 (128-bit vectors)
    sse4_2,///< x86-64 SSE4.2 (128-bit vectors)
    avx,///< x86-64 AVX (256-bit vectors)
    avx2,///< x86-64 AVX2 (256-bit vectors, integer ops)
    avx_512,///< x86-64 AVX-512 (512-bit vectors)
    arm_neon,///< ARM NEON (128-bit vectors)
    arm_sve,///< ARM SVE (scalable vectors)
    arm_sve2,///< ARM SVE2 (scalable vectors, enhanced)
    ppc_altivec,///< PowerPC AltiVec (128-bit vectors)
    ppc_vsx,///< PowerPC VSX (128-bit vectors, 64 registers)
    mips_msa,///< MIPS MSA (128-bit vectors)
};

/// \brief Register and vector characteristics for a target ISA.
///
/// Counts are architectural totals: on x86-64 `gp_registers` includes the
/// stack and frame pointers.
struct register_info {
    std::size_t gp_registers;///< Architectural general-purpose registers.
    std::size_t vector_registers;///< Architectural SIMD registers.
    std::size_t vector_width_bits;///< Width of one SIMD register, in bits.
    std::size_t lanes_64bit;///< 64-bit lanes per SIMD register.
    std::size_t lanes_32bit;///< 32-bit lanes per SIMD register.
    instruction_set isa;///< The ISA these numbers describe.
};

/// \brief Cache line sizes used for padding and alignment decisions.
///
/// The `std::hardware_*_interference_size` pair, without requiring C++17
/// library support for them.
struct cache_line_info {
    std::size_t destructive_size;///< Separate to avoid false sharing.
    std::size_t constructive_size;///< Pack within to share a line.
};

namespace detail {

    // Internal linkage: these definitions are preprocessor-selected per TU;
    // at external linkage they would violate ODR in mixed-ISA links.

    /// SVE is scalable: the width is known only when the build pins it with
    /// `-msve-vector-bits=N`. Without a pin it is the 128-bit architectural floor.
#if defined(__ARM_FEATURE_SVE_BITS) && __ARM_FEATURE_SVE_BITS > 0
    [[maybe_unused]] static constexpr std::size_t sve_vector_bits = __ARM_FEATURE_SVE_BITS;
#else
    [[maybe_unused]] static constexpr std::size_t sve_vector_bits = 128;
#endif

    [[maybe_unused]] static POET_CPP20_CONSTEVAL auto detect_instruction_set() noexcept -> instruction_set {
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

        // MSVC defines none of the __SSE*__ / __ARM_NEON predefines. x64 and
        // ARM64 guarantee SSE2 and NEON; 32-bit x86 reports its FP ISA through
        // _M_IX86_FP.
#if defined(_M_X64) || defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        return instruction_set::sse2;
#endif

#ifdef _M_ARM64
        return instruction_set::arm_neon;
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

    // Value, not the inline variable, so callers can carry it in template identity.
    POET_CPP20_CONSTEVAL auto get_register_info(instruction_set isa, std::size_t sve_bits) noexcept -> register_info {
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
            return register_info{
                31,// gp_registers
                32,// vector_registers
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::arm_sve:
        case instruction_set::arm_sve2:
            return register_info{
                31,// gp_registers
                32,// vector_registers
                sve_bits,// vector_width_bits
                sve_bits / 64,// lanes_64bit
                sve_bits / 32,// lanes_32bit
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

    [[maybe_unused]] static POET_CPP20_CONSTEVAL auto detect_cache_line_info() noexcept -> cache_line_info {
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

// Default arguments evaluate per TU, so mixed-ISA links cannot merge the
// specializations (plain inline functions merge arbitrarily in C++17).
// registers_for() and inline wrappers around these calls reintroduce that.

/// \brief The ISA the current translation unit compiles for.
///
/// Returns `instruction_set::generic` when no SIMD ISA is enabled.
template<instruction_set Arch = detail::detect_instruction_set()>
POET_CPP20_CONSTEVAL auto detected_isa() noexcept -> instruction_set {
    return Arch;
}

/// \brief Register information for `detected_isa()`.
template<instruction_set Arch = detail::detect_instruction_set(), std::size_t SVEBits = detail::sve_vector_bits>
POET_CPP20_CONSTEVAL auto available_registers() noexcept -> register_info {
    return detail::get_register_info(Arch, SVEBits);
}

/// \brief Register information for an explicitly named ISA.
/// \param isa The ISA to describe, independent of the build's own target.
///
/// Takes a runtime ISA; not merge-safe across mixed -msve-vector-bits TUs.
POET_CPP20_CONSTEVAL auto registers_for(instruction_set isa) noexcept -> register_info {
    return detail::get_register_info(isa, detail::sve_vector_bits);
}

template<instruction_set Arch = detail::detect_instruction_set(), std::size_t SVEBits = detail::sve_vector_bits>
POET_CPP20_CONSTEVAL auto vector_register_count() noexcept -> std::size_t {
    return available_registers<Arch, SVEBits>().vector_registers;
}

template<instruction_set Arch = detail::detect_instruction_set(), std::size_t SVEBits = detail::sve_vector_bits>
POET_CPP20_CONSTEVAL auto vector_width_bits() noexcept -> std::size_t {
    return available_registers<Arch, SVEBits>().vector_width_bits;
}

template<instruction_set Arch = detail::detect_instruction_set(), std::size_t SVEBits = detail::sve_vector_bits>
POET_CPP20_CONSTEVAL auto vector_lanes_64bit() noexcept -> std::size_t {
    return available_registers<Arch, SVEBits>().lanes_64bit;
}

template<instruction_set Arch = detail::detect_instruction_set(), std::size_t SVEBits = detail::sve_vector_bits>
POET_CPP20_CONSTEVAL auto vector_lanes_32bit() noexcept -> std::size_t {
    return available_registers<Arch, SVEBits>().lanes_32bit;
}

// Value-keyed: __GCC_DESTRUCTIVE_SIZE can differ between same-ISA TUs (aarch64 -mcpu).

template<std::size_t DS = detail::detect_cache_line_info().destructive_size,
  std::size_t CS = detail::detect_cache_line_info().constructive_size>
POET_CPP20_CONSTEVAL auto cache_line() noexcept -> cache_line_info {
    return cache_line_info{ DS, CS };
}

/// \brief Minimum separation that avoids false sharing.
template<std::size_t V = detail::detect_cache_line_info().destructive_size>
POET_CPP20_CONSTEVAL auto destructive_interference_size() noexcept -> std::size_t {
    return V;
}

/// \brief Maximum span that shares one cache line.
template<std::size_t V = detail::detect_cache_line_info().constructive_size>
POET_CPP20_CONSTEVAL auto constructive_interference_size() noexcept -> std::size_t {
    return V;
}

}// namespace poet
// END_FILE: include/poet/core/cpu_info.hpp
/* End inline (angle): include/poet/core/cpu_info.hpp */
/* Begin inline (angle): include/poet/core/dynamic_for.hpp */
// BEGIN_FILE: include/poet/core/dynamic_for.hpp

/// \file dynamic_for.hpp
/// \brief Runtime-bounded loops with a compile-time-unrolled body.
///
/// One `run_loop` template covers every public overload: the stride, the
/// callable form, and the extra by-value arguments are template parameters, so
/// no tag object or dispatch value is passed at run time. The main loop emits
/// fully unrolled blocks of `Unroll` iterations; the tail is a binary
/// decomposition with O(log2 Unroll) branches; a range smaller than `Unroll`
/// is inlined so the lane constants stay visible.
///
/// `dynamic_for` pays off for multi-accumulator work: the lane form
/// (`func(lane_constant, index)`) gives one accumulator per lane, breaking the
/// serial dependence of a plain loop. For element-wise work or one serial
/// chain, a plain `for` loop has less overhead.

#include <cstddef>
#include <limits>
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

/// Binds an lvalue callable as-is; materialises an rvalue into a named local so
/// the loop bodies can take it by reference without a lambda indirection.
template<typename Func>
using callable_storage_t = std::conditional_t<std::is_lvalue_reference_v<Func>, Func, std::remove_reference_t<Func>>;

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

/// Expands `[StartIndex, StartIndex + sizeof...(Is))` of the range as a fold.
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_FORCEINLINE constexpr auto run_block(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr std::ptrdiff_t Base = Begin + (Step * static_cast<std::ptrdiff_t>(StartIndex));
    (func(std::integral_constant<std::ptrdiff_t, Base + (Step * static_cast<std::ptrdiff_t>(Is))>{}), ...);
}

/// Same expansion, outlined so each block gets its own register allocation.
template<typename Func, std::ptrdiff_t Begin, std::ptrdiff_t Step, std::size_t StartIndex, std::size_t... Is>
POET_NOINLINE_FLATTEN constexpr auto run_block_isolated(Func &func, std::index_sequence<Is...> seq) -> void {
    run_block<Func, Begin, Step, StartIndex>(func, seq);
}

template<bool Isolate,
  typename Func,
  std::ptrdiff_t Begin,
  std::ptrdiff_t Step,
  std::size_t BlockSize,
  std::size_t... Is>
POET_FORCEINLINE constexpr auto emit_blocks(Func &func, std::index_sequence<Is...> /*seq*/) -> void {
    constexpr auto block = std::make_index_sequence<BlockSize>{};
    if constexpr (Isolate) {
        (run_block_isolated<Func, Begin, Step, Is * BlockSize>(func, block), ...);
    } else {
        (run_block<Func, Begin, Step, Is * BlockSize>(func, block), ...);
    }
}

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
/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */


namespace poet {

namespace detail {

    // --- Callable form: resolved once per instantiation, never per iteration ---

    /// \brief True if F accepts `(index, args...)` or `(lane_constant, index, args...)`.
    ///
    /// Guards the enable_if on every public overload so a non-callable Func
    /// slot is removed from overload resolution.
    template<typename F, typename T, typename... Args>
    inline constexpr bool is_df_callable_v =
      std::is_invocable_v<F &, T, Args...>
      || std::is_invocable_v<F &, std::integral_constant<std::size_t, 0>, T, Args...>;

    /// \brief True when the callable takes the lane as a leading `integral_constant`.
    /// Given `is_df_callable_v`, "not this" means the index-only form.
    template<typename F, typename T, typename... Args>
    inline constexpr bool wants_lane_v = std::is_invocable_v<F &, std::integral_constant<std::size_t, 0>, T, Args...>;

    template<bool WantsLane, std::size_t Lane, typename Func, typename T, typename... Args>
    POET_FORCEINLINE constexpr void invoke_lane(Func &func, T index, Args... args) {
        if constexpr (WantsLane) {
            func(std::integral_constant<std::size_t, Lane>{}, index, args...);
        } else {
            func(index, args...);
        }
    }

    // --- Stride carrier ---

    /// A stride fixed at compile time: an empty type, so it costs no register
    /// and its value reaches every expression as a literal. A runtime stride is
    /// a plain `T`; one implementation serves both forms.
    template<std::ptrdiff_t Step> using static_stride = std::integral_constant<std::ptrdiff_t, Step>;

    /// Narrows either stride flavour to `T`. The `static_stride` implicit
    /// conversion covers the compile-time case, so one cast serves both.
    template<typename T, typename Stride> POET_FORCEINLINE constexpr auto stride_of(Stride stride) noexcept -> T {
        return static_cast<T>(stride);
    }

    // --- Iteration count ---

    /// True when the stride runs backward. An unsigned `T` wraps a negative
    /// stride into the top half of its range, which also counts as backward.
    /// The `if constexpr` keeps an unsigned `T` from the always-false
    /// `stride < 0` comparison.
    template<typename T> POET_FORCEINLINE constexpr auto is_backward(T stride) noexcept -> bool {
        if constexpr (std::is_signed_v<T>) {
            return stride < 0;
        } else {
            return stride > (std::numeric_limits<T>::max() / 2);
        }
    }

    POET_FORCEINLINE constexpr auto is_power_of_two(std::size_t value) noexcept -> bool {
        return (value & (value - 1)) == 0;
    }

    /// \brief Number of iterations in `[begin, end)` at the given stride.
    ///
    /// With a `static_stride`, the direction and power-of-two tests
    /// constant-fold to the arithmetic of a compile-time-stride loop.
    template<typename T, typename Stride>
    POET_FORCEINLINE constexpr auto iteration_count(T begin, T end, Stride stride_in) -> std::size_t {
        using unsigned_t = std::make_unsigned_t<T>;
        const T stride = stride_of<T>(stride_in);

        // Every public overload asserts `Step != 0`, so only a runtime stride
        // can still be zero here, and dividing by zero below would be UB.
        if constexpr (std::is_integral_v<Stride>) {
            if (POET_UNLIKELY(stride == 0)) { return 0; }
        }

        if (POET_UNLIKELY(is_backward(stride))) {
            if (POET_UNLIKELY(begin <= end)) { return 0; }
            // Negate at T's width, where wrapping is defined: this recovers
            // `2` from both signed `-2` and unsigned `T(-2)`. Written as
            // `0 - x` because MSVC's C4146 flags the deliberate wrap.
            const auto negated = static_cast<unsigned_t>(unsigned_t{ 0 } - static_cast<unsigned_t>(stride));
            const auto magnitude = static_cast<std::size_t>(negated);
            return ((static_cast<std::size_t>(begin - end) + magnitude) - 1) / magnitude;
        }

        if (POET_UNLIKELY(begin >= end)) { return 0; }

        const auto magnitude = static_cast<std::size_t>(stride);
        const std::size_t span = (static_cast<std::size_t>(end - begin) + magnitude) - 1;
        // A power-of-two stride, including the dominant stride==1 case,
        // shifts instead of dividing.
        if (POET_LIKELY(is_power_of_two(magnitude))) { return span >> count_trailing_zeros(magnitude); }
        return span / magnitude;
    }

    // --- Block emission ---

    /// Carried index (`index += stride`) rather than `base + Lane * stride`:
    /// the lane-to-lane dependence keeps GCC's SLP vectorizer from packing the
    /// index computations into a vector; per-lane accumulators stay independent.
    template<bool WantsLane, typename Func, typename T, typename Stride, std::size_t... Lanes, typename... Args>
    POET_FORCEINLINE constexpr void
      emit_lanes(Func &func, T index, Stride stride, std::index_sequence<Lanes...> /*lanes*/, Args... args) {
        ((invoke_lane<WantsLane, Lanes>(func, index, args...), index += stride_of<T>(stride)), ...);
    }

    /// One fully unrolled block of `Count` iterations starting at `index`.
    template<std::size_t Count, bool WantsLane, typename Func, typename T, typename Stride, typename... Args>
    POET_FORCEINLINE constexpr void emit_block(Func &func, T index, Stride stride, Args... args) {
        emit_lanes<WantsLane>(func, index, stride, std::make_index_sequence<Count>{}, args...);
    }

    // --- Binary decomposition tail ---

    /// Largest power of two strictly below `bound` (`bound >= 2`).
    constexpr auto half_below(std::size_t bound) noexcept -> std::size_t {
        std::size_t pow2 = 1;
        while (pow2 * 2 < bound) { pow2 *= 2; }
        return pow2;
    }

    /// \brief Runs the final 0..N-1 iterations by halving the envelope.
    ///
    /// Each level spends one branch on its upper half and emits it as a fully
    /// unrolled block: O(log2 N) branches instead of the O(N) of a cascade.
    /// Lanes restart at 0 in each emitted block, so a tail iteration's lane is
    /// not `index % Unroll`. Per-lane accumulators stay correct; code that
    /// assumes a fixed lane-to-iteration mapping does not.
    template<std::size_t N, bool WantsLane, typename Func, typename T, typename Stride, typename... Args>
    POET_FORCEINLINE void tail_binary(std::size_t count, Func &func, T index, Stride stride, Args... args) {
        if constexpr (N > 1) {
            constexpr std::size_t half = half_below(N);
            const std::size_t rem = (count >= half) ? (count - half) : count;
            tail_binary<half, WantsLane>(rem, func, index, stride, args...);
            if (count >= half) {
                const T offset = static_cast<T>(rem) * stride_of<T>(stride);
                emit_block<half, WantsLane>(func, static_cast<T>(index + offset), stride, args...);
            }
        }
    }

    /// The same tail, kept out of line so its register allocation cannot perturb
    /// the hot loop's. `flatten` stops GCC's ISRA pass from re-outlining each
    /// functor body inside the tail, which would reload loop constants per call.
    template<std::size_t N, bool WantsLane, typename Func, typename T, typename Stride, typename... Args>
    POET_NOINLINE_FLATTEN void
      tail_binary_outlined(std::size_t count, Func &func, T index, Stride stride, Args... args) {
        tail_binary<N, WantsLane>(count, func, index, stride, args...);
    }

    /// \brief Returns `count` in a form the optimizer cannot constant-fold.
    ///
    /// `Unroll == 1` is a contract: without this barrier, a provably constant
    /// trip count lets the compiler re-inflate the loop it must keep rolled.
    /// GNU/clang: an empty asm barrier costs zero instructions. MSVC has no
    /// x64 inline asm, so a `volatile` round-trip (one stack store+load) does
    /// the same job.
    template<typename T> POET_FORCEINLINE auto opaque_count(T count) -> T {
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : "+r"(count));// NOLINT(hicpp-no-assembler)
        return count;
#elif defined(_MSC_VER)
        volatile T laundered = count;
        return laundered;
#else
        return count;
#endif
    }

    POET_PUSH_OPTIMIZE

    // --- Fused implementation ---

    /// \brief The whole of dynamic_for: main unrolled loop plus binary tail.
    ///
    /// `Args...` are loop-invariant "hot" values threaded by value; see the
    /// public `(count, func, args...)` overload for the rationale.
    template<std::size_t Unroll, bool WantsLane, typename T, typename Func, typename Stride, typename... Args>
    POET_HOT_LOOP void run_loop(const T begin, const T end, Stride stride, Func &func, Args... args) {
        const std::size_t count = iteration_count(begin, end, stride);
        if (POET_UNLIKELY(count == 0)) { return; }

        T index = begin;

        if constexpr (Unroll == 1) {
            const std::size_t trips = opaque_count(count);
            for (std::size_t i = 0; i < trips; ++i) {
                invoke_lane<WantsLane, 0>(func, index, args...);
                index += stride_of<T>(stride);
            }
        } else if (POET_UNLIKELY(count < Unroll)) {
            // Tiny range: there is no main loop to run, so inline the tail and
            // keep the lane constants visible.
            tail_binary<Unroll, WantsLane>(count, func, index, stride, args...);
        } else {
            const T block_step = static_cast<T>(Unroll) * stride_of<T>(stride);
            std::size_t remaining = count;
            while (remaining >= Unroll) {
                emit_block<Unroll, WantsLane>(func, index, stride, args...);
                index += block_step;
                remaining -= Unroll;
            }
            if (remaining > 0) { tail_binary_outlined<Unroll, WantsLane>(remaining, func, index, stride, args...); }
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// --- Public API ---

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// Iterates over `[begin, end)` with the given `step`, emitting blocks of
/// `Unroll` iterations. `step == 1` selects the compile-time-stride path.
///
/// \tparam Unroll Iterations per unrolled block. No default: choose per call
///   site. `2` small codegen, `4` balanced, `8` profiled hot loops, `1` plain
///   loop.
/// \param begin Inclusive start bound.
/// \param end Exclusive end bound.
/// \param step Increment per iteration. May be negative.
/// \param func Callable invoked per iteration, in either form:
///   - `func(std::integral_constant<std::size_t, lane>{}, index)`: lane as type
///   - `func(index)`: index only
template<std::size_t Unroll,
  typename T1,
  typename T2,
  typename T3,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::common_type_t<T1, T2, T3>>, int> = 0>
POET_FORCEINLINE void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");

    using T = std::common_type_t<T1, T2, T3>;
    constexpr bool lane = detail::wants_lane_v<std::remove_reference_t<Func>, T>;

    detail::callable_storage_t<Func> callable(std::forward<Func>(func));
    const T stride = static_cast<T>(step);

    if (stride == static_cast<T>(1)) {
        detail::run_loop<Unroll, lane>(
          static_cast<T>(begin), static_cast<T>(end), detail::static_stride<1>{}, callable);
    } else {
        detail::run_loop<Unroll, lane>(static_cast<T>(begin), static_cast<T>(end), stride, callable);
    }
}

/// \brief Executes a runtime-sized loop with a compile-time stride.
///
/// Per-lane stride multiplications become compile-time constants and the
/// direction test in the iteration count folds away.
///
/// \tparam Unroll Iterations emitted per unrolled block.
/// \tparam Step Compile-time stride (must be non-zero).
template<std::size_t Unroll,
  std::ptrdiff_t Step,
  typename T1,
  typename T2,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::common_type_t<T1, T2>>, int> = 0>
POET_FORCEINLINE void dynamic_for(T1 begin, T2 end, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");

    using T = std::common_type_t<T1, T2>;
    detail::callable_storage_t<Func> callable(std::forward<Func>(func));

    detail::run_loop<Unroll, detail::wants_lane_v<std::remove_reference_t<Func>, T>>(
      static_cast<T>(begin), static_cast<T>(end), detail::static_stride<Step>{}, callable);
}

/// \brief Executes a runtime-sized loop, inferring the step direction (+1 when
/// `begin <= end`, -1 otherwise).
template<std::size_t Unroll,
  typename T1,
  typename T2,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::common_type_t<T1, T2>>, int> = 0>
POET_FORCEINLINE void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    const auto first = static_cast<T>(begin);
    const auto last = static_cast<T>(end);
    dynamic_for<Unroll>(first, last, first <= last ? static_cast<T>(1) : static_cast<T>(-1), std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop over `[0, count)`.
template<std::size_t Unroll,
  typename Func,
  std::enable_if_t<detail::is_df_callable_v<std::remove_reference_t<Func>, std::size_t>, int> = 0>
POET_FORCEINLINE void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll, 1>(std::size_t{ 0 }, count, std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop over `[0, count)`, passing loop-invariant
/// "hot" values to the callable by value instead of through a closure.
///
/// GCC does not scalar-replace a capturing lambda's closure that holds large
/// types (AVX-512 zmm values, say): the closure spills to the stack and
/// reloads once per iteration even with full inlining. Named by-value
/// parameters stay in registers.
///
/// This form requires at least one hot argument, so a zero-arg call still
/// selects the `(count, func)` overload.
///
/// \tparam Unroll Iterations per unrolled block.
/// \tparam Step Compile-time stride (must be non-zero).
/// \param count Iteration count, i.e. the range `[0, count)`.
/// \param func Callable `void(T index, HotArgs...)`. Do not also capture the
///   hot values; a capture would reintroduce the closure this form avoids.
/// \param args Loop-invariant values forwarded by value at each level.
template<std::size_t Unroll,
  std::ptrdiff_t Step = 1,
  typename Func,
  typename... Args,
  std::enable_if_t<(sizeof...(Args) >= 1)
                     && detail::is_df_callable_v<std::remove_reference_t<Func>, std::size_t, Args...>,
    int> = 0>
POET_FORCEINLINE void dynamic_for(std::size_t count, Func &&func, Args... args) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");

    detail::callable_storage_t<Func> callable(std::forward<Func>(func));

    detail::run_loop<Unroll, detail::wants_lane_v<std::remove_reference_t<Func>, std::size_t, Args...>>(
      std::size_t{ 0 }, count, detail::static_stride<Step>{}, callable, args...);
}

}// namespace poet


#if POET_CPLUSPLUS >= 202002L
#include <ranges>
#include <tuple>

namespace poet {

/// Holds the user callable for the `range | make_dynamic_for<N>(f)` form.
/// `Func` is deduced, `Unroll` is not, hence the ordering.
template<typename Func, std::size_t Unroll> struct dynamic_for_adaptor {
    Func func;
    constexpr explicit dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

/// Runs the callable over the range's elements.
///
/// Random access is required because the unrolled body indexes off `begin`
/// rather than advancing an iterator, which is also what keeps the lane
/// constants compile-time.
template<typename Func, std::size_t Unroll, std::ranges::random_access_range Range>
void operator|(Range &&r, dynamic_for_adaptor<Func, Unroll> const &ad) {
    const auto first = std::ranges::begin(r);
    const auto at = [first](std::size_t pos) -> decltype(auto) {
        return first[static_cast<std::ranges::range_difference_t<Range>>(pos)];
    };
    // O(1) when the sentinel can be subtracted, which random access usually
    // implies; `ranges::size` would reject views like `iota(0) | take(n)`.
    const auto count = static_cast<std::size_t>(std::ranges::distance(r));

    if constexpr (detail::wants_lane_v<Func, std::ranges::range_reference_t<Range>>) {
        dynamic_for<Unroll>(count, [&](auto lane, std::size_t pos) { ad.func(lane, at(pos)); });
    } else {
        dynamic_for<Unroll>(count, [&](std::size_t pos) { ad.func(at(pos)); });
    }
}

/// Tuple-like `(begin, end, step)` source.
template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
    const auto [b, e, s] = t;
    poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

/// Deduces `Func` so only `Unroll` has to be spelled out.
template<std::size_t U, typename F> constexpr auto make_dynamic_for(F &&f) -> dynamic_for_adaptor<std::decay_t<F>, U> {
    return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

}// namespace poet
#endif// POET_CPLUSPLUS >= 202002L
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

/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */
/* Begin inline (angle): include/poet/core/mdspan_utils.hpp */
// BEGIN_FILE: include/poet/core/mdspan_utils.hpp

/// \file mdspan_utils.hpp
/// \brief Row-major stride computation for the N-D dispatch in dispatch.hpp.

#include <array>
#include <cstddef>
/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet::detail {

/// stride[i] = product of dims[i+1..N-1].
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

    /// Payload for void-returning dispatch, so a match is still an engaged optional.
    struct void_result {};

    template<typename T> using result_holder = std::optional<std::conditional_t<std::is_void_v<T>, void_result, T>>;

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
            // Every runtime slot must equal its compile-time candidate.
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                if constexpr (std::is_void_v<ResultType>) {
                    std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                    res = void_result{};
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

    template<typename V, V Start, V... Is>
    auto inclusive_range_impl(std::integer_sequence<V, Is...>)
      -> std::integer_sequence<V, static_cast<V>(Start + Is)...>;

}// namespace detail

/// \brief Inclusive integer sequence `[Start, End]`, in `Start`'s own value type.
template<auto Start, decltype(Start) End>
using inclusive_range = decltype(detail::inclusive_range_impl<decltype(Start), Start>(
  std::make_integer_sequence<decltype(Start), End - Start + 1>{}));

/// \brief Runtime value paired with the compile-time candidates to probe.
template<typename Seq> struct dispatch_param {
    using seq_type = Seq;
    /// The sequence's own value type, so brace-init rejects a narrowing runtime
    /// value instead of silently truncating.
    using value_type = typename Seq::value_type;
    value_type runtime_val;
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

    template<typename Sequence> struct sequence_first;

    template<typename V, V First, V... Rest>
    struct sequence_first<std::integer_sequence<V, First, Rest...>> : std::integral_constant<V, First> {};

    /// `high == low + 1`, phrased as a guarded difference so an unsigned value type
    /// cannot wrap a descending pair into a spurious unit step.
    template<typename V> constexpr auto steps_up(V low, V high) noexcept -> bool {
        return high > low && high - low == 1;
    }

    /// True when the values form a unit-stride run, ascending or descending.
    ///
    /// `seq_lookup` resolves runs by `position == distance from First`, so a
    /// span equal to the value count is not enough: a permutation such as
    /// `{2, 0, 1}` matches the span but not the positions.
    template<typename V, V... Values> POET_CPP20_CONSTEVAL auto is_unit_stride() noexcept -> bool {
        constexpr std::size_t count = sizeof...(Values);
        if constexpr (count < 2) {
            return true;
        } else {
            constexpr std::array<V, count> values = { Values... };
            constexpr bool ascending = steps_up(values[0], values[1]);
            if constexpr (!ascending && !steps_up(values[1], values[0])) {
                return false;
            } else {
                for (std::size_t i = 2; i < count; ++i) {
                    const bool unit =
                      ascending ? steps_up(values[i - 1], values[i]) : steps_up(values[i], values[i - 1]);
                    if (!unit) { return false; }
                }
                return true;
            }
        }
    }

    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<typename V, V First, V... Rest>
    struct is_contiguous_sequence<std::integer_sequence<V, First, Rest...>>
      : std::bool_constant<is_unit_stride<V, First, Rest...>()> {};

    template<typename Seq> struct sparse_index;

    template<typename V, V... Values> struct sparse_index<std::integer_sequence<V, Values...>> {
        using seq_type = std::integer_sequence<V, Values...>;
        using value_type = V;
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<V, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        // Insertion sort that carries original positions alongside keys, so the dispatch table
        // preserves user-declared slot order while lookups can use ordered search (binary/strided).
        static constexpr sorted_data_t sorted_data = []() constexpr -> sorted_data_t {
            sorted_data_t out{};
            out.sorted_keys = std::array<V, value_count>{ Values... };
            for (std::size_t i = 0; i < value_count; ++i) { out.sorted_indices[i] = i; }
            for (std::size_t i = 1; i < value_count; ++i) {
                const V current_key = out.sorted_keys[i];
                const std::size_t current_index = out.sorted_indices[i];
                std::size_t insert_pos = i;
                // Shift larger keys and their original-position tags right in
                // lockstep until the position of the first key not greater
                // than `current_key`.
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

        static constexpr std::array<V, unique_count> keys = []() constexpr -> std::array<V, unique_count> {
            std::array<V, unique_count> out{};
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

    /// Maps a runtime value to its slot in `Seq`.
    ///
    /// `find` returns a slot in `[0, count)` on a hit and some value `>= count`
    /// on a miss, by contract rather than by a fixed sentinel: the contiguous
    /// finder's raw unsigned difference underflows out of range on its own, so
    /// a hit costs one subtraction and no select. Callers test `idx < count`.
    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct seq_lookup;

    template<typename V, V... Values> struct seq_lookup<std::integer_sequence<V, Values...>, true> {
        static constexpr V first = sequence_first<std::integer_sequence<V, Values...>>::value;
        static constexpr std::size_t len = sizeof...(Values);
        static constexpr bool ascending = (first == std::min({ Values... }));

        static constexpr std::size_t count = len;

        static POET_FORCEINLINE auto find(V value) -> std::size_t {
            // Unsigned subtraction sends "below first" far above `count`, so the
            // caller's `idx < count` test covers underflow and overflow alike.
            // The width must be the value type's own, or a 64-bit miss could alias
            // back into range after a 32-bit truncation.
            using U = std::make_unsigned_t<V>;
            const auto lhs = static_cast<U>(ascending ? value : first);
            const auto rhs = static_cast<U>(ascending ? first : value);
            return static_cast<std::size_t>(static_cast<U>(lhs - rhs));
        }
    };

    // Non-contiguous sequences: detect a uniform positive stride at compile time and
    // specialize `find` to a div/mod (strided) instead of a binary search (truly sparse).
    template<typename V, V... Values> struct seq_lookup<std::integer_sequence<V, Values...>, false> {
        using sparse_data = sparse_index<std::integer_sequence<V, Values...>>;

        static constexpr bool is_strided = []() constexpr -> bool {
            if constexpr (sparse_data::unique_count < 2) {
                return false;
            } else {
                // Reject non-positive strides up front so `find` can use unsigned math.
                // Keys are sorted and unique, so the gap is positive in any value type.
                constexpr V stride0 = static_cast<V>(sparse_data::keys[1] - sparse_data::keys[0]);
                if constexpr (stride0 == 0) { return false; }
                // When any later gap differs from `stride0`, `find` uses binary search.
                // cppcheck-suppress syntaxError ; cppcheck cannot parse a loop inside if constexpr
                for (std::size_t i = 2; i < sparse_data::unique_count; ++i) {
                    if (static_cast<V>(sparse_data::keys[i] - sparse_data::keys[i - 1]) != stride0) { return false; }
                }
                return true;
            }
        }();

        static constexpr std::size_t count = sparse_data::value_count;

        /// `indices` is a permutation of `[0, count)`, but GCC and Clang cannot
        /// see that through the table load and re-check the bound the caller
        /// already applies. Stating the invariant drops the duplicate compare.
        static POET_FORCEINLINE auto bounded(std::size_t slot) -> std::size_t {
            if (slot >= count) { POET_UNREACHABLE(); }
            return slot;
        }

        static POET_FORCEINLINE auto find(V value) -> std::size_t {
            if constexpr (is_strided) {
                using U = std::make_unsigned_t<V>;
                static constexpr V first = sparse_data::keys[0];
                static constexpr V stride = static_cast<V>(sparse_data::keys[1] - sparse_data::keys[0]);
                // Unsigned arithmetic: "below first" wraps past the upper bound,
                // so the two range ends collapse into the single `slot >=` test
                // below. Keys are sorted, so `stride` is positive and the
                // division is a shift.
                const auto diff = static_cast<U>(static_cast<U>(value) - static_cast<U>(first));
                if (diff % static_cast<U>(stride) != 0) { return count; }
                const auto slot = static_cast<std::size_t>(diff / static_cast<U>(stride));
                if (slot >= sparse_data::unique_count) { return count; }
                // Map the sorted position back to the slot the user declared.
                return bounded(sparse_data::indices[slot]);
            } else {
                // Keys are sorted, so a binary search finds `value`; `indices`
                // maps the found position back to the declared slot.
                const auto pos = std::lower_bound(sparse_data::keys.begin(), sparse_data::keys.end(), value);
                if (pos == sparse_data::keys.end() || *pos != value) { return count; }
                return bounded(sparse_data::indices[static_cast<std::size_t>(pos - sparse_data::keys.begin())]);
            }
        }
    };

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

    /// Row-major flat index of the runtime coordinate, or `dispatch_npos` on a miss.
    ///
    /// `seq_lookup::find` already specializes each per-dimension lookup, so one
    /// flattening path serves every sequence shape.
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/) -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        using lookup = std::tuple<seq_lookup<typename std::tuple_element_t<Idx, P>::seq_type>...>;

        const std::array<std::size_t, sizeof...(Idx)> found = { std::tuple_element_t<Idx, lookup>::find(
          std::get<Idx>(params).runtime_val)... };

        // Bitwise-AND fold, not logical: no dimension's range test becomes a
        // branch, and the offset is summed unconditionally alongside the test
        // because a miss discards the sum anyway.
        const unsigned hit = ((static_cast<unsigned>(found[Idx] < std::tuple_element_t<Idx, lookup>::count)) & ...);
        const std::size_t flat = ((found[Idx] * strides[Idx]) + ...);

        return (hit != 0) ? flat : dispatch_npos;
    }

    template<typename ParamTuple> POET_FORCEINLINE auto extract_flat_index(const ParamTuple &params) -> std::size_t {
        return flat_index(params, std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
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

    /// A `dispatch_param` carries either one sequence or (via `dispatch_set`) a
    /// tuple of them; both flatten into one sequence tuple.
    template<typename S> struct as_seq_tuple {
        using type = std::tuple<S>;
    };
    template<typename... Ts> struct as_seq_tuple<std::tuple<Ts...>> {
        using type = std::tuple<Ts...>;
    };

    template<typename Tuple, std::size_t... Indices>
    POET_CPP20_CONSTEVAL auto extract_sequences_impl(std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        return std::tuple_cat(
          typename as_seq_tuple<typename std::tuple_element_t<Indices, TupleType>::seq_type>::type{}...);
    }

    template<typename Tuple> POET_CPP20_CONSTEVAL auto extract_sequences() {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl<TupleType>(std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

    // Computes the functor's return type by probing the two calling conventions:
    // `func(integral_constant<int, V>{}, args...)` (value form) and
    // `func.template operator()<V>(args...)` (template form). The value form is
    // preferred when viable.
    template<typename Functor, typename... Seq> struct dispatch_result_helper {
        template<typename... Args>
        static auto compute_impl(std::true_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>()(sequence_first<Seq>{}..., std::declval<Args>()...));

        template<typename... Args>
        static auto compute_impl(std::false_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>().template operator()<sequence_first<Seq>::value...>(
            std::declval<Args>()...));

        template<typename... Args>
        static auto compute() -> decltype(compute_impl<Args...>(
          std::integral_constant<bool, std::is_invocable_v<Functor &, sequence_first<Seq>..., Args...>>{}));
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

    // Picks the per-arg calling convention in the function-pointer table. A
    // small trivially-copyable rvalue or const-lvalue goes by value, which is
    // cheaper than synthesising a reference. Any other arg keeps its original
    // reference category.
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

    /// `IC` is the `integral_constant` the functor would receive, value type included.
    template<typename Functor, typename IC, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, typename IC, typename... Args>
    struct can_use_value_form<Functor, IC, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, IC, Args &&...>> {};

    template<typename Functor, typename ArgPack, typename R, typename V, V... Values> struct table_builder;

    template<typename Functor, typename... Args, typename R, typename V, V... Values>
    struct table_builder<Functor, arg_pack<Args...>, R, V, Values...> {
        static constexpr V first_value = sequence_first<std::integer_sequence<V, Values...>>::value;

        template<V Value> static POET_FORCEINLINE auto call(Functor &func, pass_t<Args &&>... args) -> R {
            using ic = std::integral_constant<V, Value>;
            if constexpr (can_use_value_form<Functor, ic, arg_pack<Args...>>::value) {
                return func(ic{}, std::forward<Args>(args)...);
            } else {
                return func.template operator()<Value>(std::forward<Args>(args)...);
            }
        }

        // Stateless functors are default-constructed inside the thunk; stateful
        // functors arrive by reference, so every table entry has one signature.
        // Two overloads, not one `if constexpr` with two returns: nvcc reports
        // the `if constexpr` form as a missing return. This note is referenced
        // from nd_table_builder below.
        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry(std::true_type /*stateless*/) {
            return +[](pass_t<Args &&>... args) -> R {
                Functor func{};
                return call<Value>(func, std::forward<Args>(args)...);
            };
        }

        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry(std::false_type /*stateless*/) {
            return +[](Functor &func, pass_t<Args &&>... args) -> R {
                return call<Value>(func, std::forward<Args>(args)...);
            };
        }

        using stateless_tag = std::bool_constant<is_stateless_v<Functor>>;

        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry() { return make_entry<Value>(stateless_tag{}); }

        static POET_CPP20_CONSTEVAL auto make() {
            using fn_type = decltype(make_entry<first_value>());
            return std::array<fn_type, sizeof...(Values)>{ make_entry<Values>()... };
        }
    };

    template<typename Functor, typename ArgPack, typename R, typename V, V... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<V, Values...> /*seq*/) {
        return table_builder<Functor, ArgPack, R, V, Values...>::make();
    }

    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq> struct nd_table_builder;

    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct nd_table_builder<Functor, arg_pack<Args...>, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        static constexpr std::array<std::size_t, sizeof...(Seqs)> dims_ = { sequence_size<Seqs>::value... };
        static constexpr std::array<std::size_t, sizeof...(Seqs)> strides_ = compute_strides(dims_);

        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, typename V, V... Values>
        struct get_sequence_value<I, std::integer_sequence<V, Values...>> {
            static constexpr std::array<V, sizeof...(Values)> values = { Values... };
            static constexpr V value = values[I];
        };

        template<std::size_t FlatIdx, std::size_t DimIdx>
        static constexpr std::size_t dim_index_v = FlatIdx / strides_[DimIdx] % dims_[DimIdx];

        // Exposes each dimension of a flat index as `ic<N>`, which is what the
        // functor receives. Dimensions have distinct value types, so no shared
        // array can hold them.
        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            template<std::size_t N> using seq_at = std::tuple_element_t<N, std::tuple<Seqs...>>;

            template<std::size_t N>
            using ic = std::integral_constant<typename seq_at<N>::value_type,
              get_sequence_value<dim_index_v<FlatIdx, N>, seq_at<N>>::value>;
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

        // Two overloads for the nvcc reason stated at table_builder::make_entry.
        template<typename R> static constexpr auto make_table(std::true_type /*stateless*/) {
            using fn_type = decltype(&nd_index_caller<0>::template call_stateless<R>);
            return std::array<fn_type, sizeof...(FlatIndices)>{
                &nd_index_caller<FlatIndices>::template call_stateless<R>...
            };
        }

        template<typename R> static constexpr auto make_table(std::false_type /*stateless*/) {
            using fn_type = decltype(&nd_index_caller<0>::template call<R>);
            return std::array<fn_type, sizeof...(FlatIndices)>{ &nd_index_caller<FlatIndices>::template call<R>... };
        }

        template<typename R> static constexpr auto make_table() {
            return make_table<R>(std::bool_constant<is_stateless_v<Functor>>{});
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
///
/// Where a tuple of `dispatch_param`s probes the full cartesian product,
/// `dispatch_set` enumerates only the combinations that exist:
///
/// ```cpp
/// using Shapes = poet::dispatch_set<int, poet::tuple_<2, 2>, poet::tuple_<4, 4>>;
/// poet::dispatch(MatMul{}, Shapes{rows, cols}, a, b, c);
/// ```
///
/// \note Unlike the `dispatch_param` overloads, this path calls **only** the
/// template form `functor.template operator()<Values...>(args...)`. A functor
/// taking `std::integral_constant` parameters will not compile here.
///
/// \tparam ValueType Type every tuple element is converted to.
/// \tparam Tuples The allowed combinations, as `tuple_<...>`. All must have the
///   same arity and be distinct.
template<typename ValueType, typename... Tuples> struct dispatch_set {
  private:
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<tuple_<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

  public:
    /// The allowed tuples as `std::integer_sequence`s; what `dispatch` matches against.
    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;

    /// Number of values in every allowed tuple.
    static constexpr std::size_t tuple_arity = detail::sequence_size<std::tuple_element_t<0, seq_type>>::value;

  private:
    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    std::array<ValueType, tuple_arity> runtime_val;

    static_assert(sizeof...(Tuples) >= 1, "dispatch_set requires at least one allowed tuple");

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in dispatch_set must have the same arity");

    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "dispatch_set contains duplicate allowed tuples");

  public:
    /// \brief Binds the runtime values to probe. Requires exactly `tuple_arity` of them.
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit dispatch_set(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_on_no_match{};

/// \brief Thrown when a `throw_on_no_match` dispatch has no matching specialization.
struct no_match_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

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
        const auto runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = seq_lookup<Seq>::find(runtime_val);

        if (idx < seq_lookup<Seq>::count) {
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
        const std::size_t flat_idx = extract_flat_index(params);
        if (POET_LIKELY(flat_idx != dispatch_npos)) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table =
              make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(decltype(extract_sequences<ParamTuple>()){});
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
        static_assert(param_count >= 1, "poet::dispatch requires at least one dispatch_param");
        using sequences_t = decltype(extract_sequences<ParamTuple>());
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        }
    }

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
        // A reference-tuple view over the whole pack permits two indexing passes
        // without copies.
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        // Copy the leading dispatch_params into a value tuple; each holds one
        // runtime int, so the copy is trivial.
        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        // `std::move(all_refs)` moves only the tuple; the references inside keep
        // the remaining entries' value categories.
        return dispatch_impl<ThrowOnNoMatch>(functor,
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    // Splits the pack into leading dispatch_params and trailing args: count
    // dispatch_param types until the first non-param; the rest are plain args.
    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor &functor, FirstParam &&first_param, Rest &&...rest)
      -> decltype(auto) {
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
/// Accepts leading `dispatch_param` arguments, followed by any remaining
/// arguments, which are forwarded to `functor` untouched. `functor` is invoked
/// in whichever form it provides, with the value form preferred when both are
/// viable (which is what makes a generic lambda work):
///
/// - `functor(std::integral_constant<V, Value>{}..., args...)`: values
/// - `functor.template operator()<Value...>(args...)`: template parameters
///
/// \warning On a miss this overload is **silent**: it returns a
/// default-constructed result (or nothing, for `void`) and never calls
/// `functor`. Prefix the call with `poet::throw_on_no_match` to get a
/// `no_match_error` instead.
///
/// \param functor The callable to specialize. Bound by reference, so a
///   stateful functor's mutations stay visible to the caller.
/// \param first_param First `dispatch_param`.
/// \param rest Further `dispatch_param`s (consecutive ones form a cartesian
///   product), then the arguments to forward.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
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
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
  ParamTuple const &params,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<false>(functor, params, std::forward<Args>(args)...);
}

namespace detail {
    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor &functor,
      TupleList const & /*tl*/,
      const RuntimeTuple &runtime_tuple,
      Args &&...args)// NOLINT(cppcoreguidelines-missing-std-forward) forwarded inside short-circuiting fold
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type = typename seq_call_result<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

        result_holder<result_type> out;

        // By reference, not by copy: a stateful functor's mutations must be
        // visible to the caller, exactly as on the dispatch_param path.
        using FunctorT = std::decay_t<Functor>;
        FunctorT &functor_ref = functor;

        const bool matched = std::apply(
          [&](auto... seqs) POET_ALWAYS_INLINE_LAMBDA -> bool {
              return ([&](auto &seq) POET_ALWAYS_INLINE_LAMBDA -> bool {
                  using SeqType = std::decay_t<decltype(seq)>;
                  auto result = seq_matcher<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
                    runtime_tuple, functor_ref, std::forward<Args>(args)...);

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
template<typename Functor, typename ValueType, typename... Tuples, typename... Args>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
  const dispatch_set<ValueType, Tuples...> &set,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(functor,
      typename dispatch_set<ValueType, Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `dispatch_set` dispatch.
template<typename Functor, typename ValueType, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
  const dispatch_set<ValueType, Tuples...> &set,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(functor,
      typename dispatch_set<ValueType, Tuples...>::seq_type{},
      set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing `dispatch_param` overload.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
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
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
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
/* Skipped already inlined: include/poet/core/for_utils.hpp */
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
        // Isolate the blocks only when there is more than one: a lone block has
        // no sibling to contend with for registers, so outlining it only costs a
        // call.
        if constexpr (FullBlocks > 0) {
            emit_blocks<(FullBlocks > 1), Callable, Begin, Step, BlockSize>(
              callable, std::make_index_sequence<FullBlocks>{});
        }

        if constexpr (Remainder > 0) {
            run_block<Callable, Begin, Step, FullBlocks * BlockSize>(callable, std::make_index_sequence<Remainder>{});
        }
    }

    /// \brief True when `Callable` accepts the loop index as an integral_constant.
    ///
    /// Narrower than `is_invocable` on purpose: detects only the direct `func(ic)`
    /// call, the only call `static_for` performs.
    template<typename Callable, std::ptrdiff_t I>
    constexpr auto detect_takes_index(int /*rank*/) noexcept
      -> decltype(std::declval<Callable &>()(std::integral_constant<std::ptrdiff_t, I>{}), true) {
        return true;
    }

    template<typename Callable, std::ptrdiff_t I> constexpr auto detect_takes_index(long /*rank*/) noexcept -> bool {
        return false;
    }

    template<typename Callable, std::ptrdiff_t I>
    inline constexpr bool takes_index_v = detect_takes_index<Callable, I>(0);

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
    detail::callable_storage_t<Func> callable(std::forward<Func>(func));

    if constexpr (detail::takes_index_v<callable_t, Begin>) {
        detail::run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder>(callable);
    } else {
        // `template <auto I> operator()()` form: adapt it to the
        // integral_constant call the block emitters use.
        using invoker_t = detail::template_invoker<callable_t>;
        invoker_t invoker{ callable };
        detail::run_blocks<invoker_t, Begin, Step, BlockSize, full_blocks, remainder>(invoker);
    }
}

/// \brief Convenience overload for `static_for<0, End>(func)`.
template<std::ptrdiff_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}

}// namespace poet
// END_FILE: include/poet/core/static_for.hpp
/* End inline (angle): include/poet/core/static_for.hpp */
/* Begin inline (angle): include/poet/core/undef_macros.hpp */
// BEGIN_FILE: include/poet/core/undef_macros.hpp
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
// END_FILE: include/poet/core/undef_macros.hpp
/* End inline (angle): include/poet/core/undef_macros.hpp */
// NOLINTEND(llvm-include-order)
// clang-format on
// END_FILE: include/poet/poet.hpp

#endif // POET_SINGLE_HEADER_GOLDBOT_HPP
