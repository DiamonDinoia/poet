/* Auto-generated single-header. Do not edit directly. */

#ifndef POET_SINGLE_HEADER_GOLDBOT_HPP
#define POET_SINGLE_HEADER_GOLDBOT_HPP

// BEGIN_FILE: include/poet/poet.hpp

/// \file poet.hpp
/// \brief Umbrella header aggregating the public POET API surface.
///
/// This header centralizes includes for the public headers found under
/// include/poet/ so that downstream projects can simply include <poet/poet.hpp>
/// to access the stable API surface.

// clang-format off
// IMPORTANT: Include order matters! macros.hpp must come first, undef_macros.hpp must come last
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
#define POET_PUSH_OPTIMIZE __pragma(optimize("gt", on))
#define POET_POP_OPTIMIZE __pragma(optimize("", on))
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

// END_FILE: include/poet/core/macros.hpp
/* End inline (angle): include/poet/core/macros.hpp */
/* Begin inline (angle): include/poet/core/register_info.hpp */
// BEGIN_FILE: include/poet/core/register_info.hpp

#include <cstddef>

namespace poet {

// ============================================================================
// Detected Instruction Set
// ============================================================================

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

// ============================================================================
// Register Count Information
// ============================================================================

/// Information about available registers and vector capabilities for a given ISA.
struct register_info {
    /// Number of general-purpose integer registers available
    size_t gp_registers;

    /// Number of SIMD/vector registers available
    size_t vector_registers;

    /// Vector width in bits (e.g., 128 for SSE2, 256 for AVX, 512 for AVX-512)
    size_t vector_width_bits;

    /// Number of elements per vector for 64-bit operations
    /// Example: 128-bit vector with 64-bit elements = 2 lanes
    size_t lanes_64bit;

    /// Number of elements per vector for 32-bit operations
    size_t lanes_32bit;

    /// Instruction set this info describes
    instruction_set isa;
};

// ============================================================================
// Compile-Time Instruction Set Detection
// ============================================================================

namespace detail {

    /// Detect the highest-level instruction set available at compile time.
    /// This is automatically detected from compiler defines.
    POET_CPP20_CONSTEVAL instruction_set detect_instruction_set() noexcept {
        // AVX-512 (highest priority - includes F, CD, BW, DQ extensions)
#if defined(__AVX512F__)
        return instruction_set::avx_512;
#endif

        // AVX2 (includes AVX)
#if defined(__AVX2__)
        return instruction_set::avx2;
#endif

        // AVX
#if defined(__AVX__)
        return instruction_set::avx;
#endif

        // SSE4.2
#if defined(__SSE4_2__)
        return instruction_set::sse4_2;
#endif

        // SSE2
#if defined(__SSE2__)
        return instruction_set::sse2;
#endif

        // ARM NEON (auto-enabled on ARM64)
#if defined(__ARM_NEON__)
        return instruction_set::arm_neon;
#endif

        // ARM SVE2 (newer scalable vectors)
#if defined(__ARM_FEATURE_SVE2__)
        return instruction_set::arm_sve2;
#endif

        // ARM SVE (scalable vectors)
#if defined(__ARM_FEATURE_SVE__)
        return instruction_set::arm_sve;
#endif

        // PowerPC VSX
#if defined(__VSX__)
        return instruction_set::ppc_vsx;
#endif

        // PowerPC AltiVec
#if defined(__ALTIVEC__)
        return instruction_set::ppc_altivec;
#endif

        // MIPS MSA
#if defined(__mips_msa)
        return instruction_set::mips_msa;
#endif

        // Default fallback
        return instruction_set::generic;
    }

    /// Retrieve register information for a specific instruction set.
    POET_CPP20_CONSTEVAL register_info get_register_info(instruction_set isa) noexcept {
        switch (isa) {
            // ────────────────────────────────────────────────────────────────────
            // x86-64 Family
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::sse2:
        case instruction_set::sse4_2:
            // x86-64: 16 GP regs, 16 XMM regs (128-bit each)
            return register_info{
                16,// gp_registers
                16,// vector_registers: XMM0-XMM15 (128-bit)
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::avx:
        case instruction_set::avx2:
            // AVX/AVX2: 16 YMM registers (256-bit each)
            // AVX2 adds integer vector ops but same register count
            return register_info{
                16,// gp_registers
                16,// vector_registers: YMM0-YMM15 (256-bit)
                256,// vector_width_bits
                4,// lanes_64bit
                8,// lanes_32bit
                isa,
            };

        case instruction_set::avx_512:
            // AVX-512: 32 ZMM registers (512-bit each)
            // Also includes 8 mask registers (k0-k7), but we count vector regs
            return register_info{
                16,// gp_registers
                32,// vector_registers: ZMM0-ZMM31 (512-bit AVX-512)
                512,// vector_width_bits
                8,// lanes_64bit
                16,// lanes_32bit
                isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // ARM Family
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::arm_neon:
        case instruction_set::arm_sve:
        case instruction_set::arm_sve2:
            // AArch64 NEON: 32 V registers (128-bit each, V0-V31)
            // ARM SVE/SVE2: 32 Z registers (scalable, min 128-bit)
            // Conservative estimate uses 128-bit minimum for SVE.
            // Also 31 general-purpose registers on ARM64 (x0-x30)
            return register_info{
                31,// gp_registers
                32,// vector_registers: V0-V31 / Z0-Z31
                128,// vector_width_bits: minimum guaranteed
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // PowerPC Family
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::ppc_altivec:
            // PowerPC AltiVec: 32 vector registers (128-bit each)
            // Also 32 general-purpose registers
            return register_info{
                32,// gp_registers
                32,// vector_registers: V0-V31 (128-bit AltiVec registers)
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

        case instruction_set::ppc_vsx:
            // PowerPC VSX: extends AltiVec with 64 vector registers (128-bit each)
            // or 32 registers with doubled width (256-bit nominal)
            return register_info{
                32,// gp_registers
                64,// vector_registers: VSX0-VSX63 (128-bit per register)
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // MIPS Family
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::mips_msa:
            // MIPS MSA: 32 vector registers (128-bit each)
            // Also 32 general-purpose registers
            return register_info{
                32,// gp_registers
                32,// vector_registers: W0-W31 (128-bit MSA registers)
                128,// vector_width_bits
                2,// lanes_64bit
                4,// lanes_32bit
                isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // Generic/Unknown
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::generic:
        default:
            // Conservative fallback: assume x86-64 SSE2-like baseline
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

}// namespace detail

// ============================================================================
// Public API
// ============================================================================

/// Detect the instruction set available at compile time.
/// This is a compile-time constant determined from compiler defines.
POET_CPP20_CONSTEVAL instruction_set detected_isa() noexcept { return detail::detect_instruction_set(); }

/// Get register information for the detected instruction set.
/// This is a compile-time constant based on the host compiler's target.
POET_CPP20_CONSTEVAL register_info available_registers() noexcept { return detail::get_register_info(detected_isa()); }

/// Get register information for a specific instruction set.
POET_CPP20_CONSTEVAL register_info registers_for(instruction_set isa) noexcept {
    return detail::get_register_info(isa);
}

/// Number of SIMD/vector registers available.
POET_CPP20_CONSTEVAL size_t vector_register_count() noexcept { return available_registers().vector_registers; }

/// Vector width in bits for the detected instruction set.
POET_CPP20_CONSTEVAL size_t vector_width_bits() noexcept { return available_registers().vector_width_bits; }

/// Number of 64-bit lanes in a vector for the detected instruction set.
POET_CPP20_CONSTEVAL size_t vector_lanes_64bit() noexcept { return available_registers().lanes_64bit; }

/// Number of 32-bit lanes in a vector for the detected instruction set.
POET_CPP20_CONSTEVAL size_t vector_lanes_32bit() noexcept { return available_registers().lanes_32bit; }

}// namespace poet

// END_FILE: include/poet/core/register_info.hpp
/* End inline (angle): include/poet/core/register_info.hpp */
/* Begin inline (angle): include/poet/core/dynamic_for.hpp */
// BEGIN_FILE: include/poet/core/dynamic_for.hpp

/// \file dynamic_for.hpp
/// \brief Provides runtime loop execution with compile-time unrolling.
///
/// This header defines `dynamic_for`, a utility that allows executing a loop
/// where the range is determined at runtime, but the body is unrolled at
/// compile-time. This is achieved by emitting unrolled blocks and a runtime
/// loop over those blocks.
///
/// ## Architecture Overview
///
/// `dynamic_for` uses a three-tier execution strategy to balance performance,
/// code size, and compile-time cost:
///
/// ### 1. Main Loop (Unrolled Blocks)
/// For ranges larger than the unroll factor, the loop executes in chunks of
/// `Unroll` iterations. Each chunk is fully unrolled at compile-time via
/// `static_for`, while the outer loop iterates at runtime. This provides:
/// - Amortized loop overhead (one branch per `Unroll` iterations)
/// - Better instruction-level parallelism (ILP) from unrolling
/// - Compiler visibility for vectorization and optimization
///
/// ### 2. Tail Dispatch (Compile-Time Dispatch Table)
/// After the main loop, 0 to `Unroll-1` iterations may remain. Instead of a
/// runtime loop, we use `poet::dispatch` over `DispatchParam<make_range<1, Unroll-1>>`
/// to route directly to a compile-time block specialization.
/// This keeps tail handling branch-free at call sites and maps each tail size
/// to a dedicated block implementation.
///
/// ### 3. Tiny Range Fast Path
/// For ranges smaller than `Unroll`, we skip the main runtime loop and dispatch
/// directly to a specialized tail block. This keeps lane information available
/// as compile-time constants and avoids extra loop-control overhead.
///
/// ## Optimization Design
///
/// The implementation minimizes abstraction layers between user code and the
/// actual loop body:
///
/// - **Callable form introspection**: The callable's signature (lane-by-value
///   or index-only) is detected once at template instantiation via
///   `detect_callable_form`, not per-iteration via `if constexpr`.
///
/// - **Fused block emission**: Unrolled blocks delegate to `static_for`
///   with lightweight invoker structs, reusing its register-pressure-aware
///   block isolation for large unroll factors.
///
/// - **Two-tier stride handling**: When the stride is known at compile time
///   (including the common stride=1 case), per-lane multiplication uses
///   compile-time constants and tail dispatch eliminates the stride argument.
///   Runtime strides fall back to a general path with runtime arithmetic.
///
/// - **Inline callable materialization**: Rvalue callables are materialized
///   directly in the public API without lambda indirection.
///
/// ## When to use dynamic_for vs a plain for loop
///
/// `dynamic_for` is designed for **multi-accumulator patterns** where
/// compile-time lane indices enable independent chains that break serial
/// dependencies.  Use the lane-by-value callable form
/// (`func(lane_constant, index)`) with separate per-lane accumulators —
/// this is where `dynamic_for` **outperforms** a naive for loop.
///
/// For simple **index-only** element-wise work (`out[i] = f(i)`), a plain
/// `for` loop is typically faster.  `dynamic_for` expands each block via
/// `static_for` template expansion, which on GCC can cause the SLP
/// vectorizer to over-allocate vector registers and spill on wide ISAs
/// (AVX2/AVX-512).  A plain `for` loop avoids this because the compiler's
/// loop vectorizer handles packing naturally with a single index register.
///
/// `dynamic_for` also does **not** help with serial dependency chains
/// (`acc += work(i)`).  Unrolling a serial chain adds instructions without
/// improving ILP.  Use lane callbacks with separate accumulators instead.
///
/// ## Choosing the Unroll Factor
///
/// `Unroll` is a required template parameter. Larger values increase code size
/// and compile-time work, while potentially reducing loop overhead in hot paths.
/// Typical starting points:
/// - `Unroll=4` for balanced behavior in general code.
/// - `Unroll=2` when compile time or code size is the primary constraint.
/// - `Unroll=8` for profiled hot loops with simple bodies.
/// - `Unroll=1` compiles to a plain loop with no dispatch overhead.

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */
/* Begin inline (angle): include/poet/core/static_dispatch.hpp */
// BEGIN_FILE: include/poet/core/static_dispatch.hpp

/// \file static_dispatch.hpp
/// \brief Runtime-to-compile-time dispatch via function pointer tables.
///
/// Maps runtime integers (or tuples of integers) to compile-time template parameters
/// using compile-time generated function pointer arrays with O(1) runtime indexing.
///
/// Dispatch modes:
/// - **Contiguous ranges**: O(1) arithmetic lookup via `sequence_runtime_lookup`.
/// - **Sparse sequences**: O(log N) binary search on sorted keys.
/// - **N-D ranges**: Flattened to a single function pointer array with row-major strides.
/// - **DispatchSet**: Explicit tuple matching for non-cartesian combinations.
///
/// Key optimizations:
/// - Stateless functors eliminate the functor pointer from table entries.
/// - Parameter type introspection passes small trivial types by value.

#include <algorithm>
#include <array>
#include <cassert>
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
/// the N-D function-pointer-table dispatch in static_dispatch.hpp.

#include <array>
#include <cstddef>
/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet::detail {

/// Total size (product of all dimensions).
template<std::size_t N>
POET_FORCEINLINE POET_CPP23_CONSTEXPR auto compute_total_size(const std::array<std::size_t, N> &dims) -> std::size_t {
    std::size_t total = 1;
    for (std::size_t i = 0; i < N; ++i) { total *= dims[i]; }
    return total;
}

/// Compute row-major strides. stride[i] = product of dims[i+1..N-1].
template<std::size_t N>
POET_FORCEINLINE POET_CPP23_CONSTEXPR auto compute_strides(const std::array<std::size_t, N> &dims)
  -> std::array<std::size_t, N> {
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

/// \brief Helper for concise tuple syntax in DispatchSet
///
/// Use `T<1, 2>` to represent a tuple `(1, 2)` inside a `DispatchSet`.
template<auto... Vs> struct T {};

namespace detail {

    template<typename T>
    using result_holder = std::conditional_t<std::is_void_v<T>, std::optional<std::monostate>, std::optional<T>>;

    /// \brief Helper: call functor with the integer pack from an integer_sequence
    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args> struct seq_matcher_helper;

    /// \brief Specialization of seq_matcher_helper for std::integer_sequence.
    /// Matches runtime values against compile-time sequence V... and invokes functor if they match.
    template<typename ValueType,
      ValueType... V,
      typename ResultType,
      typename RuntimeTuple,
      typename Functor,
      typename... Args>
    struct seq_matcher_helper<std::integer_sequence<ValueType, V...>, ResultType, RuntimeTuple, Functor, Args...> {
        template<std::size_t... Idx, typename F>
        static auto
          impl(std::index_sequence<Idx...> /*idx_seq*/, const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
            -> result_holder<ResultType> {
            result_holder<ResultType> res;
            // N compile-time values (V...) and N runtime values in runtime_tuple.
            // We expand the fold expression ((std::get<Idx>(runtime_tuple) == V) && ...)
            // to check if *every* runtime value matches its corresponding compile-time value V.
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                // Match found! Invoke the functor with the sequence V... as template arguments.
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

    /// \brief Helper to compute result type for tuple-sequence calls.
    template<typename Seq, typename Functor, typename... Args> struct result_of_seq_call;

    template<typename ValueType, ValueType... V, typename Functor, typename... Args>
    struct result_of_seq_call<std::integer_sequence<ValueType, V...>, Functor, Args...> {
        using type = decltype(std::declval<Functor>().template operator()<V...>(std::declval<Args>()...));
    };

    template<int Start, int... Is>
    auto make_range_impl(std::integer_sequence<int, Is...>) -> std::integer_sequence<int, (Start + Is)...>;

}// namespace detail

/// \brief Generates an inclusive integer sequence `[Start, End]`.
template<int Start, int End>
using make_range = decltype(detail::make_range_impl<Start>(std::make_integer_sequence<int, End - Start + 1>{}));

/// \brief Wraps runtime dispatch parameters and their candidate sequences.
///
/// Each runtime value is paired with a `std::integer_sequence` that describes
/// the compile-time options that should be probed when dispatching.
/// The dispatcher compares the runtime inputs against all combinations of the
/// provided sequences and invokes the functor when a match is found.
template<typename Seq> struct DispatchParam {
    int runtime_val;
    using seq_type = Seq;
};

namespace detail {
    // Trait to detect DispatchParam types
    template<typename T> struct is_dispatch_param : std::false_type {};
    template<typename Seq> struct is_dispatch_param<DispatchParam<Seq>> : std::true_type {};

    template<typename T> inline constexpr bool is_dispatch_param_v = is_dispatch_param<std::decay_t<T>>::value;

    // Trait to detect if a type is a tuple of DispatchParams
    template<typename T> struct is_dispatch_param_tuple : std::false_type {};

    template<typename... Ts>
    struct is_dispatch_param_tuple<std::tuple<Ts...>> : std::bool_constant<(is_dispatch_param_v<Ts> && ...)> {};

    template<typename T>
    inline constexpr bool is_dispatch_param_tuple_v = is_dispatch_param_tuple<std::decay_t<T>>::value;
}// namespace detail

namespace detail {

    // Helper to compute the size of a sequence
    template<typename Sequence> struct sequence_size;

    template<typename T, T... Values>
    struct sequence_size<std::integer_sequence<T, Values...>>
      : std::integral_constant<std::size_t, sizeof...(Values)> {};

    // Helper to check if a value exists in a sequence
    template<int Value, typename Sequence> struct sequence_contains;

    template<int Value, int... Values>
    struct sequence_contains<Value, std::integer_sequence<int, Values...>>
      : std::bool_constant<((Value == Values) || ...)> {};

    // Helper to check that all values in a sequence are unique
    template<typename Sequence> struct sequence_unique;

    template<> struct sequence_unique<std::integer_sequence<int>> : std::true_type {};

    template<int First, int... Rest>
    struct sequence_unique<std::integer_sequence<int, First, Rest...>>
      : std::bool_constant<!sequence_contains<First, std::integer_sequence<int, Rest...>>::value
                           && sequence_unique<std::integer_sequence<int, Rest...>>::value> {};

    // Compile-time predicate: true when sequence is contiguous ascending and unique.
    // A contiguous ascending sequence has max - first + 1 == count.
    // We intentionally use First (not min) so that descending sequences like
    // <6,5,4,3,2,1,0> are NOT marked contiguous — the O(1) arithmetic lookup
    // (value - first) only works when the sequence is ascending from first.
    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<int First, int... Rest>
    struct is_contiguous_sequence<std::integer_sequence<int, First, Rest...>>
      : std::bool_constant<(std::max({ First, Rest... }) - First + 1 == static_cast<int>(1 + sizeof...(Rest)))
                           && sequence_unique<std::integer_sequence<int, First, Rest...>>::value> {};

    /// \brief True when every sequence in a tuple of DispatchParams is contiguous.
    template<typename ParamTuple, typename = std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>>
    struct all_contiguous;

    template<typename ParamTuple, std::size_t... Idx>
    struct all_contiguous<ParamTuple, std::index_sequence<Idx...>>
      : std::bool_constant<(
          is_contiguous_sequence<typename std::tuple_element_t<Idx, std::decay_t<ParamTuple>>::seq_type>::value
          && ...)> {};

    template<typename ParamTuple> inline constexpr bool all_contiguous_v = all_contiguous<ParamTuple>::value;

    template<typename Sequence> struct sequence_first;

    // Compact sparse lookup metadata: sorted (value, first_index) pairs.
    template<typename Seq> struct sparse_sequence_index_data;

    template<int... Values> struct sparse_sequence_index_data<std::integer_sequence<int, Values...>> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<int, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        static constexpr sorted_data_t sorted_data = []() constexpr -> sorted_data_t {
            sorted_data_t out{};
            out.sorted_keys = std::array<int, value_count>{ Values... };
            for (std::size_t i = 0; i < value_count; ++i) { out.sorted_indices[i] = i; }
            // Stable insertion sort by key; duplicates keep source-order (first index first).
            for (std::size_t i = 1; i < value_count; ++i) {
                const int current_key = out.sorted_keys[i];
                const std::size_t current_index = out.sorted_indices[i];
                std::size_t insert_pos = i;
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

        static constexpr std::array<int, unique_count> keys = []() constexpr {
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

        static constexpr std::array<std::size_t, unique_count> indices = []() constexpr {
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

    /// Sentinel value returned by sequence_runtime_lookup::find() on miss.
    inline constexpr std::size_t dispatch_npos = static_cast<std::size_t>(-1);

    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct sequence_runtime_lookup;

    // O(1) arithmetic lookup for contiguous ascending sequences.
    template<int... Values> struct sequence_runtime_lookup<std::integer_sequence<int, Values...>, true> {
        using seq_type = std::integer_sequence<int, Values...>;
        static constexpr int first = sequence_first<seq_type>::value;
        static constexpr std::size_t len = sizeof...(Values);

        static POET_FORCEINLINE auto find(int value) -> std::size_t {
            const auto idx =
              static_cast<std::size_t>(static_cast<unsigned int>(value) - static_cast<unsigned int>(first));
            if (POET_LIKELY(idx < len)) { return idx; }
            return dispatch_npos;
        }
    };

    // O(log N) compact lookup for sparse/non-contiguous sequences.
    template<int... Values> struct sequence_runtime_lookup<std::integer_sequence<int, Values...>, false> {
        using seq_type = std::integer_sequence<int, Values...>;
        using sparse_data = sparse_sequence_index_data<seq_type>;

        static POET_FORCEINLINE auto find(int value) -> std::size_t {
            const auto it = std::lower_bound(sparse_data::keys.begin(), sparse_data::keys.end(), value);
            if (it != sparse_data::keys.end() && *it == value) {
                return sparse_data::indices[static_cast<std::size_t>(it - sparse_data::keys.begin())];
            }
            return dispatch_npos;
        }
    };

    template<int First, int... Rest>
    struct sequence_first<std::integer_sequence<int, First, Rest...>> : std::integral_constant<int, First> {};

    // Note: Empty sequence (std::integer_sequence<int>) is intentionally not specialized.
    // Attempting to use it will result in a compile-time "incomplete type" error.

    /// \brief Extract dimensions from a tuple of DispatchParam sequences.
    ///
    /// For a tuple of sequences like (make_range<0,3>, make_range<0,4>),
    /// extracts the dimensions [4, 5] (sizes of each sequence).
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE POET_CPP20_CONSTEVAL auto extract_dimensions_impl(std::index_sequence<Idx...> /*idxs*/)
      -> std::array<std::size_t, sizeof...(Idx)> {
        using P = std::decay_t<ParamTuple>;
        return std::array<std::size_t, sizeof...(Idx)>{
            sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value...
        };
    }

    template<typename ParamTuple>
    POET_FORCEINLINE POET_CPP20_CONSTEVAL auto extract_dimensions()
      -> std::array<std::size_t, std::tuple_size_v<std::decay_t<ParamTuple>>> {
        return extract_dimensions_impl<ParamTuple>(
          std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
    }

    /// \brief Combined index validation and flattening for N-D dispatch.
    /// Returns the flat index directly, or dispatch_npos on miss.
    /// Avoids std::optional overhead from extract_runtime_indices.
    ///
    /// Uses explicit arrays with pack-expansion in braced initializer lists to
    /// avoid [&] lambda mutable-capture patterns that produce poor GCC codegen.
    /// Step 1: all lookups (no captures, no mutation).
    /// Step 2: miss check via fold-and (short-circuit is implicit in the check,
    ///         but all lookups run unconditionally — acceptable for sparse paths
    ///         because sequence_runtime_lookup is cheap).
    /// Step 3: flat accumulation via fold-add (only reached on all-hit).
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto extract_flat_index_impl(const ParamTuple &params,
      const std::array<std::size_t, sizeof...(Idx)> &strides,
      std::index_sequence<Idx...> /*idxs*/) -> std::size_t {
        using P = std::decay_t<ParamTuple>;

        // Step 1: per-dimension mapped indices (pure expansion, no lambdas)
        const std::size_t indices[sizeof...(Idx)] = {
            sequence_runtime_lookup<typename std::tuple_element_t<Idx, P>::seq_type>::find(
              std::get<Idx>(params).runtime_val)...
        };

        // Step 2: miss check — any npos → early return
        const bool all_hit = ((indices[Idx] != dispatch_npos) && ...);
        if (POET_UNLIKELY(!all_hit)) return dispatch_npos;

        // Step 3: flat index via fold-add (all hits guaranteed)
        return ((indices[Idx] * strides[Idx]) + ...);
    }

    /// \brief Fused speculative index computation for all-contiguous N-D dispatch.
    /// Computes all indices unconditionally and checks OOB once at the end,
    /// allowing the CPU to pipeline index computations across dimensions.
    ///
    /// Uses explicit arrays with pack-expansion in braced initializer lists to
    /// avoid lambda/mutable-capture patterns that produce poor GCC codegen.
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto extract_flat_index_fused_impl(const ParamTuple &params,
      const std::array<std::size_t, sizeof...(Idx)> &strides,
      std::index_sequence<Idx...> /*idxs*/) -> std::size_t {
        using P = std::decay_t<ParamTuple>;

        // Step 1: per-dimension mapped indices (pure expansion, no lambdas)
        const std::size_t mapped[sizeof...(Idx)] = { static_cast<std::size_t>(
          static_cast<unsigned int>(std::get<Idx>(params).runtime_val)
          - static_cast<unsigned int>(sequence_first<typename std::tuple_element_t<Idx, P>::seq_type>::value))... };

        // Step 2: OOB check via fold-or (no mutable captures)
        const std::size_t oob = (static_cast<std::size_t>(
                                   mapped[Idx] >= sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value)
                                 | ...);

        // Step 3: flat index via fold-add
        const std::size_t flat = ((mapped[Idx] * strides[Idx]) + ...);

        return POET_LIKELY(oob == 0) ? flat : dispatch_npos;
    }

    template<typename ParamTuple>
    POET_FORCEINLINE auto extract_flat_index(const ParamTuple &params,
      const std::array<std::size_t, std::tuple_size_v<std::decay_t<ParamTuple>>> &strides) -> std::size_t {
        if constexpr (all_contiguous_v<ParamTuple>) {
            return extract_flat_index_fused_impl(
              params, strides, std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
        } else {
            return extract_flat_index_impl(
              params, strides, std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
        }
    }

    // Helper: compare two integer_sequences for element-wise equality
    template<typename A, typename B> struct seq_equal;
    template<typename T, T... A, T... B>
    struct seq_equal<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>>
      : std::bool_constant<((A == B) && ...)> {};

    // Helper: check uniqueness among a pack of sequences
    template<typename... S> struct unique_helper;
    template<> struct unique_helper<> : std::true_type {};
    template<typename Head, typename... Rest>
    struct unique_helper<Head, Rest...>
      : std::bool_constant<(!(seq_equal<Head, Rest>::value || ...) && unique_helper<Rest...>::value)> {};

    // Helper trait: detect std::tuple specializations
    template<typename T> struct is_tuple : std::false_type {};
    template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    template<typename S> POET_CPP20_CONSTEVAL auto as_seq_tuple(S /*seq*/) {
        if constexpr (is_tuple<S>::value) {
            return S{};// already a tuple of sequences
        } else {
            return std::tuple<S>{ S{} };// wrap single sequence into a tuple
        }
    }

    template<typename Tuple, std::size_t... Indices>
    auto extract_sequences_impl(const Tuple & /*tuple*/, std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        // For each param's seq_type (which may itself be a std::tuple of sequences),
        // produce a tuple and concatenate them into a single flat tuple of sequences.
        return std::tuple_cat(as_seq_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{})...);
    }

    template<typename Tuple> auto extract_sequences(const Tuple &tuple) {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl(tuple, std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

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

    // ============================================================================
    // Function pointer table-based dispatch (C++17/20)
    // ============================================================================

    /// \brief Function pointer type for dispatch (works for all return types including void).
    template<typename R, typename... Args> using dispatch_function_ptr = R (*)(Args...);

    template<typename... Args> struct arg_pack {};

    /// \brief True when a functor can be default-constructed inside table entries.
    /// Requires both is_empty (no state) and is_default_constructible (C++20 lambdas).
    template<typename T>
    inline constexpr bool is_stateless_v = std::is_empty_v<T> && std::is_default_constructible_v<T>;

    /// \brief Detects if a functor accepts a type by-value for a given template parameter.
    ///
    /// This trait checks whether `Functor::operator()<V>(U)` is callable where U is
    /// the by-value version of T. If the functor accepts by-value, we can safely
    /// optimize the function pointer signature to pass by-value even if the caller
    /// passed a non-const lvalue reference (since by-value proves no output semantics).
    ///
    /// \tparam Functor The functor type to introspect
    /// \tparam V The template parameter value
    /// \tparam T The caller's argument type (possibly a reference)
    template<typename Functor, int V, typename T> struct functor_accepts_by_value {
        using raw = std::remove_cv_t<std::remove_reference_t<T>>;

        // Only consider small trivially copyable types as optimization candidates
        static constexpr bool is_candidate = std::is_trivially_copyable_v<raw> && sizeof(raw) <= 2 * sizeof(void *);

        // Test template form: Functor::template operator()<V>(U)
        template<typename F,
          typename U,
          typename = decltype(std::declval<F>().template operator()<V>(std::declval<U>()))>
        static std::true_type test_template(int);

        template<typename F, typename U> static std::false_type test_template(...);

        // Test value form: Functor::operator()(integral_constant<int, V>, U)
        template<typename F,
          typename U,
          typename = decltype(std::declval<F>()(std::integral_constant<int, V>{}, std::declval<U>()))>
        static std::true_type test_value(int);

        template<typename F, typename U> static std::false_type test_value(...);

        static constexpr bool value =
          is_candidate
          && (decltype(test_template<Functor, raw>(0))::value || decltype(test_value<Functor, raw>(0))::value);
    };

    /// \brief Multi-dimensional version: checks if functor accepts type by-value with multiple template params.
    ///
    /// For N-D dispatch, the functor is called as operator()<V0, V1, V2, ...>(args...).
    /// This trait checks if the functor accepts by-value parameters for a given combination
    /// of template values.
    ///
    /// \tparam Functor The functor type
    /// \tparam T The caller's argument type
    /// \tparam Vs The template parameter values (one per dimension)
    template<typename Functor, typename T, int... Vs> struct functor_accepts_by_value_multi {
        using raw = std::remove_cv_t<std::remove_reference_t<T>>;

        static constexpr bool is_candidate = std::is_trivially_copyable_v<raw> && sizeof(raw) <= 2 * sizeof(void *);

        // Test template form: Functor::template operator()<Vs...>(U)
        template<typename F,
          typename U,
          typename = decltype(std::declval<F>().template operator()<Vs...>(std::declval<U>()))>
        static std::true_type test_template(int);

        template<typename F, typename U> static std::false_type test_template(...);

        // Test value form: Functor::operator()(integral_constant<int, Vs>..., U)
        template<typename F,
          typename U,
          typename = decltype(std::declval<F>()(std::integral_constant<int, Vs>{}..., std::declval<U>()))>
        static std::true_type test_value(int);

        template<typename F, typename U> static std::false_type test_value(...);

        static constexpr bool value =
          is_candidate
          && (decltype(test_template<Functor, raw>(0))::value || decltype(test_value<Functor, raw>(0))::value);
    };

    /// \brief Optimizes argument passing for dispatch function pointer tables.
    /// Small trivially-copyable types are passed by value to keep them in
    /// registers instead of spilling to stack. Applies to:
    /// - Rvalue references (safe: caller doesn't observe the move)
    /// - Const lvalue references (safe: const guarantees no output-parameter semantics)
    /// - Non-const lvalue references IF functor accepts by-value (safe: proves no output semantics)
    template<typename T, typename Functor = void, int V = 0> struct dispatch_arg_pass {
        using raw = std::remove_reference_t<T>;
        using raw_unqual = std::remove_cv_t<raw>;
        static constexpr bool is_small_trivial =
          std::is_trivially_copyable_v<raw_unqual> && (sizeof(raw_unqual) <= 2 * sizeof(void *));

        // Original conditions: caller explicitly allows copying
        static constexpr bool caller_allows_copy =
          std::is_rvalue_reference_v<T> || (std::is_lvalue_reference_v<T> && std::is_const_v<raw>);

        // New condition: functor parameter type introspection
        // If functor accepts by-value, it cannot have output semantics
        static constexpr bool functor_allows_copy =
          !std::is_void_v<Functor> && functor_accepts_by_value<Functor, V, T>::value;

        static constexpr bool by_value = is_small_trivial && (caller_allows_copy || functor_allows_copy);

        using type = std::conditional_t<by_value, raw_unqual, T>;
    };

    template<typename T, typename Functor, int V>
    using dispatch_pass_opt_t = typename dispatch_arg_pass<T, Functor, V>::type;

    /// \brief N-D version of dispatch_arg_pass for multi-dimensional dispatch.
    template<typename T, typename Functor, int... Vs> struct dispatch_arg_pass_nd {
        using raw = std::remove_reference_t<T>;
        using raw_unqual = std::remove_cv_t<raw>;
        static constexpr bool is_small_trivial =
          std::is_trivially_copyable_v<raw_unqual> && (sizeof(raw_unqual) <= 2 * sizeof(void *));

        static constexpr bool caller_allows_copy =
          std::is_rvalue_reference_v<T> || (std::is_lvalue_reference_v<T> && std::is_const_v<raw>);

        static constexpr bool functor_allows_copy = functor_accepts_by_value_multi<Functor, T, Vs...>::value;

        static constexpr bool by_value = is_small_trivial && (caller_allows_copy || functor_allows_copy);

        using type = std::conditional_t<by_value, raw_unqual, T>;
    };

    template<typename T, typename Functor, int... Vs>
    using dispatch_pass_nd_t = typename dispatch_arg_pass_nd<T, Functor, Vs...>::type;

    /// \brief Helper to detect value-argument form invocability
    template<typename Functor, int Value, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, int Value, typename... Args>
    struct can_use_value_form<Functor, Value, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, std::integral_constant<int, Value>, Args &&...>> {};

    /// \brief Generate function pointer table for contiguous 1D dispatch.
    ///
    /// Creates a compile-time array of function pointers for O(1) dispatch.
    /// Each entry corresponds to one value in the sequence. Works for both
    /// void and non-void return types via `return void_expr;` idiom.
    ///
    /// \tparam Functor User callable type.
    /// \tparam ArgPack arg_pack<Args...> with runtime argument types.
    /// \tparam R Return type (may be void).
    /// \tparam Values Compile-time integer values in the sequence.
    template<typename Functor, typename ArgPack, typename R, int... Values> struct dispatch_table_builder;

    template<typename Functor, typename... Args, typename R, int... Values>
    struct dispatch_table_builder<Functor, arg_pack<Args...>, R, Values...> {
        static constexpr int first_value = sequence_first<std::integer_sequence<int, Values...>>::value;

        // Helper to create a single lambda for a specific V value
        template<int V> struct lambda_maker {
            static POET_CPP20_CONSTEVAL auto make_stateless() {
                return +[](dispatch_pass_opt_t<Args &&, Functor, first_value>... args) -> R {
                    Functor func{};
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return func(std::integral_constant<int, V>{}, static_cast<Args &&>(args)...);
                    } else {
                        return func.template operator()<V>(static_cast<Args &&>(args)...);
                    }
                };
            }

            static POET_CPP20_CONSTEVAL auto make_stateful() {
                return +[](Functor *func, dispatch_pass_opt_t<Args &&, Functor, first_value>... args) -> R {
                    POET_ASSUME_NOT_NULL(func);
                    constexpr bool use_value_form = can_use_value_form<Functor, V, arg_pack<Args...>>::value;
                    if constexpr (use_value_form) {
                        return (*func)(std::integral_constant<int, V>{}, static_cast<Args &&>(args)...);
                    } else {
                        return func->template operator()<V>(static_cast<Args &&>(args)...);
                    }
                };
            }
        };

        static POET_CPP20_CONSTEVAL auto make() {
            if constexpr (is_stateless_v<Functor>) {
                return std::array<dispatch_function_ptr<R, dispatch_pass_opt_t<Args &&, Functor, first_value>...>,
                  sizeof...(Values)>{ lambda_maker<Values>::make_stateless()... };
            } else {
                return std::array<
                  dispatch_function_ptr<R, Functor *, dispatch_pass_opt_t<Args &&, Functor, first_value>...>,
                  sizeof...(Values)>{ lambda_maker<Values>::make_stateful()... };
            }
        }
    };

    template<typename Functor, typename ArgPack, typename R, int... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<int, Values...> /*seq*/) {
        return dispatch_table_builder<Functor, ArgPack, R, Values...>::make();
    }

    // ============================================================================
    // Multidimensional dispatch table generation (N-D -> 1D flattening)
    // ============================================================================

    /// \brief Helper to generate all N-dimensional index combinations recursively.
    ///
    /// For dimensions [D0, D1, D2], generates function pointers for all combinations:
    /// (0,0,0), (0,0,1), ..., (D0-1,D1-1,D2-1)
    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq>
    struct make_nd_dispatch_table_helper;

    /// \brief Recursively generate function pointer for each flattened index.
    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct make_nd_dispatch_table_helper<Functor,
      arg_pack<Args...>,
      std::tuple<Seqs...>,
      std::index_sequence<FlatIndices...>> {

        /// \brief Get the Ith value from a sequence at compile-time.
        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, int... Values> struct get_sequence_value<I, std::integer_sequence<int, Values...>> {
            // Select the I-th value from the integer pack without creating a temporary
            // std::array (which may emit calls in some libstdc++/libc++ builds).
            template<std::size_t J, int First, int... Rest> struct select_impl {
                static constexpr int value = select_impl<J - 1, Rest...>::value;
            };

            template<int First, int... Rest> struct select_impl<0, First, Rest...> {
                static constexpr int value = First;
            };

            static constexpr int value = select_impl<I, Values...>::value;
        };

        /// \brief Compute the index for a specific dimension from a flat index.
        template<std::size_t FlatIdx, std::size_t DimIdx, std::size_t... Dims> struct compute_dim_index {
            static constexpr std::size_t value = []() constexpr -> std::size_t {
                constexpr std::array<std::size_t, sizeof...(Dims)> dimensions = { Dims... };
                constexpr std::array<std::size_t, sizeof...(Dims)> strides = compute_strides(dimensions);
                return FlatIdx / strides[DimIdx] % dimensions[DimIdx];
            }();
        };

        /// \brief Extract the value for a specific dimension at compile-time.
        template<std::size_t FlatIdx, std::size_t DimIdx, std::size_t... Dims>
        static constexpr auto extract_value_for_dim() -> int {
            constexpr std::size_t idx_in_seq = compute_dim_index<FlatIdx, DimIdx, Dims...>::value;
            using Seq = std::tuple_element_t<DimIdx, std::tuple<Seqs...>>;
            return get_sequence_value<idx_in_seq, Seq>::value;
        }

        /// \brief Helper to build integral_constants from extracted values.
        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            static constexpr std::array<int, sizeof...(SeqIdx)> values = {
                extract_value_for_dim<FlatIdx, SeqIdx, sequence_size<Seqs>::value...>()...
            };

            template<std::size_t N> using ic = std::integral_constant<int, values[N]>;
        };

        /// \brief Helper struct to call functor with values computed from flat index.
        template<std::size_t FlatIdx> struct nd_index_caller {
            // Helper to build value_extractor type with proper index sequence
            template<std::size_t... Is>
            static auto make_ve(std::index_sequence<Is...>) -> value_extractor<FlatIdx, Is...>;

            using VE = decltype(make_ve(std::make_index_sequence<sizeof...(Seqs)>{}));

            // Expand index sequence to build parameter types using VE::values
            template<typename T, std::size_t... Is>
            static auto make_opt_type_impl(std::index_sequence<Is...>)
              -> dispatch_pass_nd_t<T, Functor, VE::values[Is]...>;

            template<typename T>
            using opt_type = decltype(make_opt_type_impl<T>(std::make_index_sequence<sizeof...(Seqs)>{}));

            template<typename R, typename VE_inner, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto call_value_form(Functor *func, Args &&...args) -> R {
                POET_ASSUME_NOT_NULL(func);
                if constexpr (std::is_void_v<R>) {
                    (*func)(typename VE_inner::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                    return;
                } else {
                    return (*func)(typename VE_inner::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                }
            }

            template<typename R, typename VE_inner, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto call_template_form(Functor *func, Args &&...args) -> R {
                POET_ASSUME_NOT_NULL(func);
                if constexpr (std::is_void_v<R>) {
                    func->template operator()<VE_inner::template ic<SeqIdx>::value...>(std::forward<Args>(args)...);
                    return;
                } else {
                    return func->template operator()<VE_inner::template ic<SeqIdx>::value...>(
                      std::forward<Args>(args)...);
                }
            }

            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto call_impl(Functor *func, Args &&...args) -> R {
                using VE_local = value_extractor<FlatIdx, SeqIdx...>;

                // Detect which form the functor supports: template parameters or integral_constant values
                constexpr bool use_value_form =
                  std::is_invocable_v<Functor &, typename VE_local::template ic<SeqIdx>..., Args &&...>;

                if constexpr (use_value_form) {
                    return call_value_form<R, VE_local, SeqIdx...>(func, std::forward<Args>(args)...);
                } else {
                    return call_template_form<R, VE_local, SeqIdx...>(func, std::forward<Args>(args)...);
                }
            }

            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto
              call_with_indices(Functor *func, std::index_sequence<SeqIdx...> /*indices*/, Args &&...args) -> R {
                return call_impl<R, SeqIdx...>(func, std::forward<Args>(args)...);
            }

            // Helper to build optimized type for a single argument using representative values (FlatIdx=0)
            template<typename T, typename IndexSeq> struct repr_opt_type_helper_impl;

            template<typename T, std::size_t... Is> struct repr_opt_type_helper_impl<T, std::index_sequence<Is...>> {
                // Extract values from FlatIdx=0
                using type =
                  dispatch_pass_nd_t<T, Functor, extract_value_for_dim<0, Is, sequence_size<Seqs>::value...>()...>;
            };

            template<typename T>
            using repr_opt_type =
              typename repr_opt_type_helper_impl<T, std::make_index_sequence<sizeof...(Seqs)>>::type;

            template<typename R> static POET_FORCEINLINE auto call(Functor *func, repr_opt_type<Args &&>... args) -> R {
                return call_with_indices<R>(
                  func, std::make_index_sequence<sizeof...(Seqs)>{}, static_cast<Args &&>(args)...);
            }

            template<typename R> static POET_FORCEINLINE auto call_stateless(repr_opt_type<Args &&>... args) -> R {
                Functor func{};
                return call_with_indices<R>(
                  &func, std::make_index_sequence<sizeof...(Seqs)>{}, static_cast<Args &&>(args)...);
            }
        };

        /// \brief Generate function pointer table for all N-D combinations.
        /// Works for both void and non-void return types.
        template<typename R> static constexpr auto make_table() {
            // Use first flat index (0) for uniform signature
            using representative_caller = nd_index_caller<0>;

            if constexpr (is_stateless_v<Functor>) {
                return std::array<
                  dispatch_function_ptr<R, typename representative_caller::template repr_opt_type<Args &&>...>,
                  sizeof...(FlatIndices)>{ &nd_index_caller<FlatIndices>::template call_stateless<R>... };
            } else {
                return std::array<dispatch_function_ptr<R,
                                    Functor *,
                                    typename representative_caller::template repr_opt_type<Args &&>...>,
                  sizeof...(FlatIndices)>{ &nd_index_caller<FlatIndices>::template call<R>... };
            }
        }
    };

    /// \brief Generate N-D dispatch table for contiguous multidimensional dispatch.
    template<typename Functor, typename ArgPack, typename R, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table(std::tuple<Seqs...> /*seqs*/) {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return make_nd_dispatch_table_helper<Functor,
          ArgPack,
          std::tuple<Seqs...>,
          std::make_index_sequence<total_size>>::template make_table<R>();
    }

}// namespace detail

/// \brief Represents a discrete set of allowed compile-time tuples.
///
/// Defines a strict set of allowed parameter combinations for dispatch.
/// Unlike cartesian product dispatching, `DispatchSet` allows specifying exact
/// combinations (e.g., (1,2) and (3,4) allowed, but (1,4) disallowed).
///
/// \tparam ValueType The shared type of values in the tuples (e.g., int).
/// \tparam Tuples... A pack of `poet::T<...>` types defining valid tuples.
template<typename ValueType, typename... Tuples> struct DispatchSet {
    // Helper to convert T<Vs...> to std::integer_sequence<ValueType, Vs...>
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<T<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;
    using first_t = std::tuple_element_t<0, seq_type>;

    static_assert(sizeof...(Tuples) >= 1, "DispatchSet requires at least one allowed tuple");

    // Ensure all tuples have the same arity (reuses detail::sequence_size)
    static constexpr std::size_t tuple_arity = detail::sequence_size<first_t>::value;

    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in DispatchSet must have the same arity");

    // Ensure no duplicate tuples (compile-time equality)
    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "DispatchSet contains duplicate allowed tuples");

    // runtime stored tuple to compare against the allowed tuples (array sized by arity)
    using runtime_array_t = std::array<ValueType, tuple_arity>;

  private:
    runtime_array_t runtime_val;

  public:
    // Construct from variadic arguments
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit DispatchSet(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    // Produce a runtime std::tuple<ValueType,...> from the stored array
    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_t{};

namespace detail {
    inline constexpr const char *k_no_match_error =
      "poet::dispatch: no matching compile-time combination for runtime inputs";

    POET_PUSH_OPTIMIZE

    /// \brief 1D dispatch through a function-pointer table.
    /// O(1) for contiguous sequences, O(log N) for sparse — selected by sequence_runtime_lookup.
    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_1d(Functor &&functor, ParamTuple const &params, Args &&...args) -> R {
        using FirstParam = std::tuple_element_t<0, std::remove_reference_t<ParamTuple>>;
        using Seq = typename FirstParam::seq_type;
        const int runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = sequence_runtime_lookup<Seq>::find(runtime_val);

        if (POET_LIKELY(idx != dispatch_npos)) {
            using FunctorT = std::decay_t<Functor>;

            static constexpr auto table = make_dispatch_table<FunctorT, arg_pack<Args...>, R>(Seq{});
            if constexpr (is_stateless_v<FunctorT>) {
                if constexpr (std::is_void_v<R>) {
                    table[idx](std::forward<Args>(args)...);
                    return;
                } else {
                    return table[idx](std::forward<Args>(args)...);
                }
            } else {
                auto &&fwd = std::forward<Functor>(functor);
                if constexpr (std::is_void_v<R>) {
                    table[idx](std::addressof(static_cast<FunctorT &>(fwd)), std::forward<Args>(args)...);
                    return;
                } else {
                    return table[idx](std::addressof(static_cast<FunctorT &>(fwd)), std::forward<Args>(args)...);
                }
            }
        }
        if constexpr (ThrowOnNoMatch) {
            throw std::runtime_error(k_no_match_error);
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    /// \brief N-D dispatch through a flattened function-pointer table.
    /// Uses fused speculative indexing for all-contiguous dimensions,
    /// short-circuit validation for mixed contiguous/sparse.
    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_nd(Functor &&functor, ParamTuple const &params, Args &&...args) -> R {
        constexpr auto dimensions = extract_dimensions<ParamTuple>();
        constexpr auto strides = compute_strides(dimensions);
        constexpr std::size_t table_size = compute_total_size(dimensions);

        const std::size_t flat_idx = extract_flat_index(params, strides);
        if (POET_LIKELY(flat_idx != dispatch_npos)) {
            POET_ASSUME(flat_idx < table_size);

            using sequences_t = decltype(extract_sequences(std::declval<ParamTuple>()));
            static constexpr sequences_t sequences{};

            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(sequences);
            if constexpr (is_stateless_v<FunctorT>) {
                if constexpr (std::is_void_v<R>) {
                    table[flat_idx](std::forward<Args>(args)...);
                    return;
                } else {
                    return table[flat_idx](std::forward<Args>(args)...);
                }
            } else {
                auto &&fwd = std::forward<Functor>(functor);
                if constexpr (std::is_void_v<R>) {
                    table[flat_idx](std::addressof(static_cast<FunctorT &>(fwd)), std::forward<Args>(args)...);
                    return;
                } else {
                    return table[flat_idx](std::addressof(static_cast<FunctorT &>(fwd)), std::forward<Args>(args)...);
                }
            }
        }
        if constexpr (ThrowOnNoMatch) {
            throw std::runtime_error(k_no_match_error);
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    /// \brief Internal implementation for dispatch with compile-time error handling policy.
    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_impl(Functor &&functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
        constexpr std::size_t param_count = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        using sequences_t = decltype(extract_sequences(std::declval<ParamTuple>()));
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(
              std::forward<Functor>(functor), params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(
              std::forward<Functor>(functor), params, std::forward<Args>(args)...);
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// DispatchParam-based API is variadic:
//   dispatch(func, DispatchParam<Seq1>{v1}, DispatchParam<Seq2>{v2}, args...)

namespace detail {
    // Helper to count leading DispatchParam arguments
    template<typename... Ts> struct count_leading_dispatch_params;

    template<> struct count_leading_dispatch_params<> {
        static constexpr std::size_t value = 0;
    };

    template<typename First, typename... Rest> struct count_leading_dispatch_params<First, Rest...> {
        static constexpr std::size_t value =
          is_dispatch_param_v<First> ? (1 + count_leading_dispatch_params<Rest...>::value) : 0;
    };

    // Split a variadic call into leading DispatchParams and trailing runtime args.
    template<bool ThrowOnNoMatch, typename Functor, std::size_t... ParamIdx, std::size_t... ArgIdx, typename... All>
    POET_FORCEINLINE auto dispatch_multi_impl(Functor &&functor,
      std::index_sequence<ParamIdx...> /*p*/,
      std::index_sequence<ArgIdx...> /*a*/,
      All &&...all) -> decltype(auto) {

        constexpr std::size_t num_params = sizeof...(ParamIdx);
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);

        // Extract DispatchParams into dedicated tuple (copy — DispatchParam is trivially copyable).
        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        // Forward remaining runtime arguments preserving value category.
        // std::move(all_refs) in a pack expansion only casts to rvalue ref; each get() accesses a disjoint index.
        return dispatch_impl<ThrowOnNoMatch>(std::forward<Functor>(functor),
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor &&functor, FirstParam &&first_param, Rest &&...rest)
      -> decltype(auto) {
        // Count leading DispatchParams to split params from runtime arguments.
        constexpr std::size_t num_params = 1 + count_leading_dispatch_params<Rest...>::value;
        constexpr std::size_t num_args = sizeof...(Rest) + 1 - num_params;

        if constexpr (num_args == 0) {
            auto params = std::make_tuple(std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
            return dispatch_impl<ThrowOnNoMatch>(std::forward<Functor>(functor), params);
        } else {
            return dispatch_multi_impl<ThrowOnNoMatch>(std::forward<Functor>(functor),
              std::make_index_sequence<num_params>{},
              std::make_index_sequence<num_args>{},
              std::forward<FirstParam>(first_param),
              std::forward<Rest>(rest)...);
        }
    }
}// namespace detail

/// \brief Dispatches runtime integers to compile-time template parameters.
///
/// The functor is invoked with the template parameter combination whose
/// values match the supplied runtime inputs. When no match exists, the functor
/// is never invoked and a default-constructed result is returned for
/// non-void functors.
///
/// This implementation uses array-based lookup tables for O(1) contiguous
/// dispatch and O(log N) sparse lookup.
///
/// Example usage:
///   // 1D dispatch
///   dispatch(func, DispatchParam<make_range<0, 7>>{runtime_val});
///
///   // 1D dispatch with arguments
///   dispatch(func, DispatchParam<make_range<0, 7>>{runtime_val}, arg1, arg2);
///
///   // 2D dispatch
///   dispatch(func,
///     DispatchParam<make_range<0, 3>>{x},
///     DispatchParam<make_range<0, 3>>{y});
///
/// \param functor Callable exposing `template <int...>` call operators or accepting
///                std::integral_constant parameters.
/// \param first_param First DispatchParam (required to distinguish this overload).
/// \param rest Remaining DispatchParams followed by runtime arguments.
/// \return The functor's result when non-void; otherwise `void`.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(Functor &&functor, FirstParam &&first_param, Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_variadic_impl<false>(
      std::forward<Functor>(functor), std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Dispatch using a tuple of DispatchParams.
///
/// This overload accepts a std::tuple of DispatchParam objects directly,
/// which is convenient when the parameters are built dynamically.
///
/// Example:
///   auto params = std::make_tuple(
///     DispatchParam<make_range<0, 3>>{x},
///     DispatchParam<make_range<0, 3>>{y});
///   dispatch(functor, params, runtime_args...);
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(Functor &&functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
    return detail::dispatch_impl<false>(std::forward<Functor>(functor), params, std::forward<Args>(args)...);
}

namespace detail {
    /// \brief Internal implementation for dispatch_tuples with compile-time error handling policy.
    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor &&functor,
      TupleList const & /*unused*/,
      const RuntimeTuple &runtime_tuple,
      Args &&...args)// NOLINT(cppcoreguidelines-missing-std-forward) forwarded inside short-circuiting fold
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type = typename result_of_seq_call<first_seq, std::decay_t<Functor>, std::decay_t<Args>...>::type;

        result_holder<result_type> out;

        using FunctorT = std::decay_t<Functor>;
        FunctorT functor_copy(std::forward<Functor>(functor));

        // Expand over all allowed tuple sequences using || fold for true short-circuiting.
        const bool matched = std::apply(
          [&](auto... seqs) -> bool {
              return ([&](auto &seq) -> bool {
                  using SeqType = std::decay_t<decltype(seq)>;
                  auto result =
                    seq_matcher_helper<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
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
            throw std::runtime_error("poet::dispatch_tuples: no matching compile-time tuple for runtime inputs");
        } else if constexpr (!std::is_void_v<result_type>) {
            return result_type{};
        }
    }
}// namespace detail

/// \brief Dispatches using a specific `DispatchSet`.
///
/// Routes the call to the internal tuple dispatch using the set's allowed combinations.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(Functor &&functor, const DispatchSet<Tuples...> &dispatch_set, Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<false>(std::forward<Functor>(functor),
      typename DispatchSet<Tuples...>::seq_type{},
      dispatch_set.runtime_tuple(),
      std::forward<Args>(args)...);
}

/// \brief Throwing overload for `DispatchSet` dispatch.
template<typename Functor, typename... Tuples, typename... Args>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,
  const DispatchSet<Tuples...> &dispatch_set,
  Args &&...args) -> decltype(auto) {
    return detail::dispatch_tuples_impl<true>(std::forward<Functor>(functor),
      typename DispatchSet<Tuples...>::seq_type{},
      dispatch_set.runtime_tuple(),
      std::forward<Args>(args)...);
}

// Note: nothrow is the default behavior of `dispatch` (no explicit tag).

/// \brief Dispatch with exception on no match.
///
/// This overload throws `std::runtime_error` when no matching dispatch is found.
/// Uses the same dispatch path as the non-throwing version.
///
/// Example:
///   dispatch(throw_t, func, DispatchParam<Seq1>{v1}, DispatchParam<Seq2>{v2}, arg1, arg2)
///
/// \param tag throw_on_no_match_t tag to request exception on mismatch.
/// \param functor Callable exposing `template <int...>` call operators.
/// \param first_param First DispatchParam.
/// \param rest Remaining DispatchParams followed by runtime arguments.
/// \return The functor's result.
/// \throws std::runtime_error if no matching dispatch is found.
template<typename Functor,
  typename FirstParam,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_param_v<FirstParam>, int> = 0>
auto dispatch(throw_on_no_match_t tag, Functor &&functor, FirstParam &&first_param, Rest &&...rest) -> decltype(auto) {
    (void)tag;
    return detail::dispatch_variadic_impl<true>(
      std::forward<Functor>(functor), std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
}

/// \brief Dispatch using a tuple of DispatchParams with exception on no match.
template<typename Functor,
  typename ParamTuple,
  typename... Args,
  std::enable_if_t<detail::is_dispatch_param_tuple_v<ParamTuple>, int> = 0>
auto dispatch(throw_on_no_match_t /*tag*/, Functor &&functor, ParamTuple const &params, Args &&...args)
  -> decltype(auto) {
    return detail::dispatch_impl<true>(std::forward<Functor>(functor), params, std::forward<Args>(args)...);
}

}// namespace poet

// END_FILE: include/poet/core/static_dispatch.hpp
/* End inline (angle): include/poet/core/static_dispatch.hpp */
/* Begin inline (angle): include/poet/core/static_for.hpp */
// BEGIN_FILE: include/poet/core/static_for.hpp

/// \file static_for.hpp
/// \brief Provides a compile-time loop unrolling utility.
///
/// This header defines `static_for`, which unrolls loops at compile-time.
/// It is useful for iterating over template parameter packs, integer sequences,
/// or performing other meta-programming tasks where the iteration count is known
/// at compile-time.

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/for_utils.hpp */
// BEGIN_FILE: include/poet/core/for_utils.hpp

/// \file for_utils.hpp
/// \brief Internal utilities for compile-time loop unrolling and range computation.
///
/// This header provides core building blocks for `static_for` and `dynamic_for`.
/// It includes mechanisms for:
/// - Calculating iteration counts for compile-time ranges.
/// - Recursively emitting blocks of unrolled code to avoid compiler limits.
/// - Defining limits on unrolling depth (recursion depth).

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

/* Begin inline (angle): include/poet/core/macros.hpp */
/* Skipped already inlined: include/poet/core/macros.hpp */
/* End inline (angle): include/poet/core/macros.hpp */

namespace poet::detail {

/// \brief Maximum chunk size for static loops to limit template instantiation depth.
/// Defines the maximum number of iterations or blocks processed in a single
/// recursive step. Reduced on MSVC to mitigate potential compiler stack overflows.
#if defined(_MSC_VER) && !defined(__clang__)
inline constexpr std::size_t kMaxStaticLoopBlock = 128;
#else
// Use a conservative upper bound for non-MSVC compilers to limit template
// instantiation depth and keep compile times reasonable for tests.
//
// Capped at 256 to avoid excessive unrolling and long compile times.
inline constexpr std::size_t kMaxStaticLoopBlock = 256;
#endif

/// \brief Computes the number of iterations for a compile-time range.
///
/// \tparam Begin Start of the range.
/// \tparam End End of the range (exclusive).
/// \tparam Step Iteration step (must be non-zero).
/// \return The number of steps required to traverse from Begin to End.
template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
[[nodiscard]] POET_CPP20_CONSTEVAL auto compute_range_count() noexcept -> std::size_t {
    static_assert(Step != 0, "static_for requires a non-zero step");

    if constexpr (Step > 0) {
        static_assert(Begin <= End, "static_for with a positive step requires Begin <= End");
        if constexpr (Begin == End) { return 0; }
        const auto distance = End - Begin;
        return static_cast<std::size_t>((distance + Step - 1) / Step);
    } else {
        static_assert(Begin >= End, "static_for with a negative step requires Begin >= End");
        if constexpr (Begin == End) { return 0; }
        const auto distance = Begin - End;
        const auto magnitude = -Step;
        return static_cast<std::size_t>((distance + magnitude - 1) / magnitude);
    }
}

/// \brief Executes a single block of unrolled loop iterations (always-inline variant).
///
/// Expands the provided indices pack into a sequence of function calls.
/// Each call receives a `std::integral_constant` corresponding to the
/// computed loop index.  Used for single-block loops and remainder
/// iterations where the block is small enough that full inlining into the
/// caller is desirable.
///
/// \tparam Func User callable type.
/// \tparam Begin Range start value.
/// \tparam Step Range step value.
/// \tparam StartIndex The flat index offset for this block.
/// \tparam Is Index sequence for unrolling (0, 1, ..., BlockSize-1).
template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t StartIndex, std::size_t... Is>
POET_FORCEINLINE constexpr void static_loop_impl_block(Func &func, std::index_sequence<Is...> /*indices*/) {
    constexpr std::intmax_t Base = Begin + (Step * static_cast<std::intmax_t>(StartIndex));
    (func(std::integral_constant<std::intmax_t, Base + (Step * static_cast<std::intmax_t>(Is))>{}), ...);
}

/// \brief Executes a single block with register-pressure isolation (noinline variant).
///
/// Used for multi-block loops where POET_FORCEINLINE would cause the
/// compiler to see all iterations simultaneously, resulting in excessive
/// register spills.  Each noinline block gets its own register allocation
/// scope — the compiler fully optimises within the block but cannot
/// interleave computations across block boundaries.
POET_PUSH_OPTIMIZE

template<typename Func, std::intmax_t Begin, std::intmax_t Step, std::size_t StartIndex, std::size_t... Is>
POET_NOINLINE constexpr void static_loop_impl_block_isolated(Func &func, std::index_sequence<Is...> /*indices*/) {
    constexpr std::intmax_t Base = Begin + (Step * static_cast<std::intmax_t>(StartIndex));
    (func(std::integral_constant<std::intmax_t, Base + (Step * static_cast<std::intmax_t>(Is))>{}), ...);
}

/// \brief Emits a chunk of full blocks (always-inline variant).
template<typename Func,
  std::intmax_t Begin,
  std::intmax_t Step,
  std::size_t BlockSize,
  std::size_t Offset,
  std::size_t... Is>
POET_FORCEINLINE constexpr void emit_block_chunk(Func &func, std::index_sequence<Is...> /*indices*/) {
    (static_loop_impl_block<Func, Begin, Step, (Offset + Is) * BlockSize>(func, std::make_index_sequence<BlockSize>{}),
      ...);
}

/// \brief Emits a chunk of register-isolated blocks (noinline variant).
template<typename Func,
  std::intmax_t Begin,
  std::intmax_t Step,
  std::size_t BlockSize,
  std::size_t Offset,
  std::size_t... Is>
POET_FORCEINLINE constexpr void emit_block_chunk_isolated(Func &func, std::index_sequence<Is...> /*indices*/) {
    (static_loop_impl_block_isolated<Func, Begin, Step, (Offset + Is) * BlockSize>(
       func, std::make_index_sequence<BlockSize>{}),
      ...);
}

POET_POP_OPTIMIZE

template<typename Func,
  std::intmax_t Begin,
  std::intmax_t Step,
  std::size_t BlockSize,
  std::size_t Offset,
  std::size_t Remaining>
POET_FORCEINLINE constexpr void emit_all_blocks(Func &func) {
    if constexpr (Remaining > 0) {
        constexpr auto chunk_size = Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;

        emit_block_chunk<Func, Begin, Step, BlockSize, Offset>(func, std::make_index_sequence<chunk_size>{});

        emit_all_blocks<Func, Begin, Step, BlockSize, Offset + chunk_size, Remaining - chunk_size>(func);
    }
}

template<typename Func,
  std::intmax_t Begin,
  std::intmax_t Step,
  std::size_t BlockSize,
  std::size_t Offset,
  std::size_t Remaining>
POET_FORCEINLINE constexpr void emit_all_blocks_isolated(Func &func) {
    if constexpr (Remaining > 0) {
        constexpr auto chunk_size = Remaining < kMaxStaticLoopBlock ? Remaining : kMaxStaticLoopBlock;

        emit_block_chunk_isolated<Func, Begin, Step, BlockSize, Offset>(func, std::make_index_sequence<chunk_size>{});

        emit_all_blocks_isolated<Func, Begin, Step, BlockSize, Offset + chunk_size, Remaining - chunk_size>(func);
    }
}

template<typename Functor> struct template_static_loop_invoker {
    Functor *functor;

    // Adapter operator: receives std::integral_constant<Value> and unpacks it
    // to call the user's template operator<Value>().
    // Force inline to eliminate adapter overhead and enable the compiler to
    // fully optimize the per-iteration body in unrolled loops.
    template<std::intmax_t Value>
    POET_FORCEINLINE constexpr void operator()(
      std::integral_constant<std::intmax_t, Value> /*integral_constant*/) const {
        POET_ASSUME_NOT_NULL(functor);
        (*functor).template operator()<Value>();
    }
};

}// namespace poet::detail

// END_FILE: include/poet/core/for_utils.hpp
/* End inline (angle): include/poet/core/for_utils.hpp */

namespace poet {

namespace detail {

    template<typename Callable,
      std::intmax_t Begin,
      std::intmax_t Step,
      std::size_t BlockSize,
      std::size_t FullBlocks,
      std::size_t Remainder,
      bool Isolate>
    POET_FORCEINLINE constexpr void static_loop_run_blocks(Callable &callable) {
        if constexpr (FullBlocks > 0) {
            // When BlockSize is explicitly tuned, use register-isolated (noinline)
            // blocks for multi-block loops.  This prevents the compiler from
            // interleaving computations across blocks, which would cause excessive
            // register spills on x86-64.
            //
            // When BlockSize is the default (Isolate == false), always inline —
            // the user hasn't opted into register-pressure tuning, so prefer
            // maximum inlining to let the compiler optimise freely.
            if constexpr (Isolate && FullBlocks > 1) {
                emit_all_blocks_isolated<Callable, Begin, Step, BlockSize, 0, FullBlocks>(callable);
            } else {
                emit_all_blocks<Callable, Begin, Step, BlockSize, 0, FullBlocks>(callable);
            }
        }

        if constexpr (Remainder > 0) {
            // Remainder is always small (< BlockSize), so always inline it.
            static_loop_impl_block<Callable, Begin, Step, FullBlocks * BlockSize>(
              callable, std::make_index_sequence<Remainder>{});
        }
    }

    /// \brief Computes a safe default block size for loop unrolling.
    ///
    /// - For small loops, returns the total iteration count.
    /// - For large loops, clamps the size to `kMaxStaticLoopBlock` to prevent
    ///   massive symbol names and excessive compile times.
    ///
    /// \tparam Begin Range start.
    /// \tparam End Range end.
    /// \tparam Step Range step.
    /// \return Optimized block size.
    template<std::intmax_t Begin, std::intmax_t End, std::intmax_t Step>
    POET_CPP20_CONSTEVAL auto compute_default_static_loop_block_size() noexcept -> std::size_t {
        constexpr auto count = detail::compute_range_count<Begin, End, Step>();
        if constexpr (count == 0) {
            return 1;
        } else if constexpr (count > detail::kMaxStaticLoopBlock) {
            return detail::kMaxStaticLoopBlock;
        } else {
            return count;
        }
    }

}// namespace detail

/// \brief Compile-time unrolled loop over the half-open range `[Begin, End)`.
///
/// Executes a compile-time unrolled loop using the specified `Step`, where
/// `Step` can be positive or negative. The iteration space is partitioned
/// into blocks of `BlockSize` elements to manage template instantiation depth.
///
/// Callables are supported in two forms:
/// - A callable that accepts a `std::integral_constant<std::intmax_t, I>`
///   argument. This form is constexpr-friendly and lets the implementation
///   forward the index as a compile-time value.
/// - A functor exposing `template <auto I>` call operators. In this case,
///   `static_for` internally adapts the functor to the integral-constant based
///   machinery.
///
/// The default `BlockSize` spans the entire range, clamped to the internal
/// `detail::kMaxStaticLoopBlock` cap (currently `256`), to balance unrolling
/// with compile-time cost.  With the default block size, **all iterations are
/// fully inlined** so the compiler can optimise freely across the entire loop.
/// Lvalue callables are preserved by reference, while rvalues are copied into
/// a local instance for the duration of the loop.
///
/// ## Tuning `BlockSize` for register pressure
///
/// By default every iteration is fully inlined into the caller.  When the
/// per-iteration body is heavy (hashing, FP chains, etc.) this can cause
/// the compiler to interleave many iterations and spill registers.  Passing
/// an explicit `BlockSize` partitions the loop into noinline blocks, each
/// with its own register-allocation scope:
///
/// \code
///   // Default: all 64 iterations fully inlined (compiler optimises freely)
///   poet::static_for<0, 64>(func);
///
///   // Tuned: 8 noinline blocks of 8 (reduces register spills)
///   poet::static_for<0, 64, 1, 8>(func);
/// \endcode
///
/// GCC vectorizes the loop body `lanes_64`-wide: each vector group packs
/// `lanes_64` iterations into one SIMD register, plus the FMA chain needs
/// ~`fma_depth + 1` constant/temporary registers as shared overhead.  The
/// spill-free limit is therefore:
///
///   `max_bs = (vec_regs − (fma_depth + 1)) × lanes_64`
///
/// Measured spill cliffs (GCC 15, -O3, 5-deep FMA chain):
///
///   | ISA     | vec_regs | lanes_64 | spill-free max | first spills |
///   |---------|----------|----------|----------------|--------------|
///   | SSE2    |    16    |    2     | >128 (scalar)  | N/A          |
///   | AVX2    |    16    |    4     |  40 accs       | 44 accs      |
///   | AVX-512 |    32    |    8     | >256 (batched) | N/A          |
///
/// In practice icache pressure caps the useful block size well below the
/// spill limit.  A good starting point:
///
///   `optimal_bs ≈ vec_regs × lanes_64 / 2`
///   SSE2 → 16,  AVX2 → 32,  AVX-512 → 128
///
/// Use the `poet_bench_static_for_native` benchmark to measure the effect
/// of different block sizes on your workload.
///
/// \tparam Begin Initial value of the range.
/// \tparam End Exclusive terminator of the range.
/// \tparam Step Increment applied between iterations (defaults to `1`).
/// \tparam BlockSize Number of iterations expanded per block (defaults to the
///                   total iteration count, clamped to `1` for empty ranges and
///                   to `detail::kMaxStaticLoopBlock` for large ranges).
/// \tparam Func Callable type.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t Begin,
  std::intmax_t End,
  std::intmax_t Step = 1,
  std::size_t BlockSize = detail::compute_default_static_loop_block_size<Begin, End, Step>(),
  typename Func>
POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_assert(BlockSize > 0, "static_for requires BlockSize > 0");

    constexpr auto count = detail::compute_range_count<Begin, End, Step>();
    if constexpr (count == 0) { return; }

    constexpr auto full_blocks = count / BlockSize;
    constexpr auto remainder = count % BlockSize;

    // Register isolation is only applied when the user explicitly tuned
    // BlockSize.  The default path fully inlines everything to let the
    // compiler optimise without artificial register-scope boundaries.
    constexpr bool isolate = (BlockSize != detail::compute_default_static_loop_block_size<Begin, End, Step>());

    // Callable storage is handled inline (rather than via a helper) to
    // avoid introducing an indirection that compilers may refuse to
    // inline across the block emission pipeline.
    using callable_t = std::remove_reference_t<Func>;

    if constexpr (std::is_invocable_v<Func, std::integral_constant<std::intmax_t, Begin>>) {
        if constexpr (std::is_lvalue_reference_v<Func>) {
            detail::static_loop_run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(func);
        } else {
            callable_t callable(std::forward<Func>(func));
            detail::static_loop_run_blocks<callable_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(
              callable);
        }
    } else {
        using invoker_t = detail::template_static_loop_invoker<callable_t>;
        if constexpr (std::is_lvalue_reference_v<Func>) {
            const invoker_t invoker{ &func };
            detail::static_loop_run_blocks<const invoker_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(
              invoker);
        } else {
            callable_t callable(std::forward<Func>(func));
            const invoker_t invoker{ &callable };
            detail::static_loop_run_blocks<const invoker_t, Begin, Step, BlockSize, full_blocks, remainder, isolate>(
              invoker);
        }
    }
}

/// \brief Convenience overload for `static_for` iterating from 0 to `End`.
///
/// This is equivalent to `static_for<0, End>(func)`.
///
/// \tparam End Exclusive upper bound of the range.
/// \param func Callable instance invoked once per iteration.
template<std::intmax_t End, typename Func> POET_FORCEINLINE constexpr void static_for(Func &&func) {
    static_for<0, End>(std::forward<Func>(func));
}

}// namespace poet

// END_FILE: include/poet/core/static_for.hpp
/* End inline (angle): include/poet/core/static_for.hpp */

namespace poet {

namespace detail {

    // ========================================================================
    // Callable form detection — detect signature once, not per iteration
    // ========================================================================

    template<typename...> inline constexpr bool always_false_v = false;

    /// \brief Tag types for callable form dispatch.
    ///
    /// Each tag represents one of the two callable signatures. The tag is
    /// selected once at template instantiation via `detect_callable_form` and
    /// threaded through as a type parameter, enabling the compiler to resolve
    /// the correct invocation path via overloading — no enum or switch needed.
    struct lane_by_value_tag {};///< func(integral_constant<size_t, Lane>{}, index)
    struct index_only_tag {};///< func(index)

    /// \brief Detects the callable form at template instantiation time.
    ///
    /// Returns the appropriate tag type. Uses Lane=0 as representative —
    /// the form is the same for all lanes.
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

    /// \brief Type alias for the detected callable form tag.
    template<typename Func, typename T> using callable_form_t = decltype(detect_callable_form<Func, T>());

    // ========================================================================
    // Specialized invokers — overloaded on tag, zero per-call branching
    // ========================================================================

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(lane_by_value_tag /*tag*/, Func &func, T index) {
        func(std::integral_constant<std::size_t, Lane>{}, index);
    }

    template<std::size_t Lane, typename Func, typename T>
    POET_FORCEINLINE constexpr void invoke_lane(index_only_tag /*tag*/, Func &func, T index) {
        func(index);
    }

    // ========================================================================
    // Block invokers — adapter structs for static_for unrolling
    // ========================================================================

    /// \brief Invoker for unrolled blocks with runtime stride.
    ///
    /// Adapts the tag-dispatched invoke_lane interface for use with static_for.
    /// Each lane computes index = base + Lane * stride.
    template<typename FormTag, typename Callable, typename T> struct block_invoker {
        Callable &callable;
        T base;
        T stride;

        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            invoke_lane<static_cast<std::size_t>(Value)>(FormTag{}, callable, base + static_cast<T>(Value) * stride);
        }
    };

    /// \brief Invoker for unrolled blocks with compile-time stride.
    ///
    /// The stride is baked into the template parameter, so per-lane
    /// multiplication uses compile-time constants (including Step=1
    /// where `Value * 1` is constant-folded away).
    template<std::intmax_t Step, typename FormTag, typename Callable, typename T> struct block_invoker_ct_stride {
        Callable &callable;
        T base;

        template<std::intmax_t Value>
        POET_FORCEINLINE constexpr void operator()(std::integral_constant<std::intmax_t, Value> /*ic*/) const {
            invoke_lane<static_cast<std::size_t>(Value)>(FormTag{}, callable, base + static_cast<T>(Value * Step));
        }
    };

    // ========================================================================
    // Block execution — delegates to static_for for compile-time unrolling
    // ========================================================================

    /// \brief Executes an unrolled block with runtime stride.
    template<typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void execute_block([[maybe_unused]] FormTag /*tag*/,
      [[maybe_unused]] Callable &callable,
      [[maybe_unused]] T base,
      [[maybe_unused]] T stride) {
        if constexpr (Count > 0) {
            block_invoker<FormTag, Callable, T> invoker{ callable, base, stride };
            static_for<0, static_cast<std::intmax_t>(Count)>(invoker);
        }
    }

    /// \brief Executes an unrolled block with compile-time stride.
    template<std::intmax_t Step, typename FormTag, typename Callable, typename T, std::size_t Count>
    POET_FORCEINLINE constexpr void execute_block_ct_stride([[maybe_unused]] FormTag /*tag*/,
      [[maybe_unused]] Callable &callable,
      [[maybe_unused]] T base) {
        if constexpr (Count > 0) {
            block_invoker_ct_stride<Step, FormTag, Callable, T> invoker{ callable, base };
            static_for<0, static_cast<std::intmax_t>(Count)>(invoker);
        }
    }

    // ========================================================================
    // Tail dispatch — reuses poet::dispatch with fused block emission
    // ========================================================================

    /// \brief Stateless functor for runtime-stride tail dispatch.
    ///
    /// Empty struct (is_stateless_v = true) so dispatch eliminates the functor
    /// pointer from the function pointer table. The callable pointer, index, and
    /// stride are passed as dispatch arguments and stay in registers.
    template<typename FormTag, typename Callable, typename T> struct tail_dispatch_functor {
        template<int N> POET_FORCEINLINE void operator()(Callable *callable, T index, T stride) const {
            if constexpr (N > 0) {
                execute_block<FormTag, Callable, T, static_cast<std::size_t>(N)>(FormTag{}, *callable, index, stride);
            }
        }
    };

    /// \brief Stateless functor for compile-time-stride tail dispatch.
    ///
    /// The stride is baked into the functor type, eliminating the stride
    /// argument from the dispatch table entries.
    template<std::intmax_t Step, typename FormTag, typename Callable, typename T>
    struct tail_dispatch_functor_ct_stride {
        template<int N> POET_FORCEINLINE void operator()(Callable *callable, T index) const {
            if constexpr (N > 0) {
                execute_block_ct_stride<Step, FormTag, Callable, T, static_cast<std::size_t>(N)>(
                  FormTag{}, *callable, index);
            }
        }
    };

    POET_PUSH_OPTIMIZE

    template<typename FormTag, std::size_t Unroll, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_tail(std::size_t count, Callable &callable, T index, T stride) {
        static_assert(Unroll > 1, "dispatch_tail requires Unroll > 1");
        if (count == 0) { return; }
        const tail_dispatch_functor<FormTag, Callable, T> functor{};
        const T c_index = index;
        const T c_stride = stride;
        poet::dispatch(functor,
          poet::DispatchParam<poet::make_range<1, static_cast<int>(Unroll - 1)>>{ static_cast<int>(count) },
          std::addressof(callable),
          c_index,
          c_stride);
    }

    template<std::intmax_t Step, typename FormTag, std::size_t Unroll, typename Callable, typename T>
    POET_FORCEINLINE void dispatch_tail_ct_stride(std::size_t count, Callable &callable, T index) {
        static_assert(Unroll > 1, "dispatch_tail_ct_stride requires Unroll > 1");
        if (count == 0) { return; }
        const tail_dispatch_functor_ct_stride<Step, FormTag, Callable, T> functor{};
        const T c_index = index;
        poet::dispatch(functor,
          poet::DispatchParam<poet::make_range<1, static_cast<int>(Unroll - 1)>>{ static_cast<int>(count) },
          std::addressof(callable),
          c_index);
    }

    POET_POP_OPTIMIZE

    // ========================================================================
    // Iteration count calculation
    // ========================================================================

    /// \brief Calculate iteration count for runtime strides.
    ///
    /// Handles three cases:
    /// 1. Backward iteration (stride < 0 or wrapped negative for unsigned)
    /// 2. Power-of-2 forward stride (uses bit shift instead of division)
    /// 3. General forward stride (uses division)
    template<typename T>
    POET_FORCEINLINE constexpr auto calculate_iteration_count_complex(T begin, T end, T stride) -> std::size_t {
        constexpr bool is_unsigned = !std::is_signed_v<T>;
        constexpr T half_max = std::numeric_limits<T>::max() / 2;
        const bool is_wrapped_negative = is_unsigned && (stride > half_max);

        if (POET_UNLIKELY(stride < 0 || is_wrapped_negative)) {
            if (POET_UNLIKELY(begin <= end)) { return 0; }
            T abs_stride;
            if constexpr (std::is_signed_v<T>) {
                abs_stride = static_cast<T>(-stride);
            } else {
                abs_stride = static_cast<T>(0) - stride;
            }
            auto dist = static_cast<std::size_t>(begin - end);
            auto ustride = static_cast<std::size_t>(abs_stride);
            return (dist + ustride - 1) / ustride;
        }

        if (POET_UNLIKELY(begin >= end)) { return 0; }

        auto dist = static_cast<std::size_t>(end - begin);
        auto ustride = static_cast<std::size_t>(stride);
        const bool is_power_of_2 = (ustride & (ustride - 1)) == 0;

        if (POET_LIKELY(is_power_of_2)) {
            const unsigned int shift = poet_count_trailing_zeros(ustride);
            return (dist + ustride - 1) >> shift;
        }
        return (dist + ustride - 1) / ustride;
    }

    /// \brief Calculate iteration count when stride is known at compile time.
    ///
    /// Uses `if constexpr` on the sign of Step to eliminate runtime branches
    /// for stride direction. The compiler can also constant-fold power-of-2
    /// divisions since the stride is a template parameter.
    template<std::intmax_t Step, typename T>
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

    // ========================================================================
    // Fused implementation: runtime stride
    // ========================================================================

    /// \brief Fused dynamic_for implementation for arbitrary runtime stride.
    ///
    /// Handles all non-unit strides including negative, power-of-2, and general.
    /// The callable form is baked into the template parameter.
    POET_PUSH_OPTIMIZE

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
                dispatch_tail<FormTag, Unroll>(count, callable, index, stride);
                return;
            }

            const T stride_times_unroll = static_cast<T>(Unroll) * stride;
            while (remaining >= Unroll) {
                execute_block<FormTag, Callable, T, Unroll>(tag, callable, index, stride);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            dispatch_tail<FormTag, Unroll>(remaining, callable, index, stride);
        }
    }

    // ========================================================================
    // Fused implementation: compile-time stride (includes stride=1)
    // ========================================================================

    /// \brief Fused dynamic_for implementation for compile-time stride.
    ///
    /// The stride is a template parameter, so:
    /// - Per-lane multiplication uses compile-time constants
    /// - Tail dispatch passes no stride argument (baked into functor type)
    /// - Iteration count eliminates stride-sign branches at compile time
    ///
    /// The runtime stride=1 fast path also routes here (as Step=1), where
    /// `Value * 1` is constant-folded, producing identical codegen to a
    /// hand-written stride-1 loop.
    template<std::intmax_t Step, typename T, typename Callable, std::size_t Unroll, typename FormTag>
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
                dispatch_tail_ct_stride<Step, FormTag, Unroll>(count, callable, index);
                return;
            }

            constexpr T stride_times_unroll = static_cast<T>(static_cast<std::intmax_t>(Unroll) * Step);
            while (remaining >= Unroll) {
                execute_block_ct_stride<Step, FormTag, Callable, T, Unroll>(tag, callable, index);
                index += stride_times_unroll;
                remaining -= Unroll;
            }

            dispatch_tail_ct_stride<Step, FormTag, Unroll>(remaining, callable, index);
        }
    }

    POET_POP_OPTIMIZE

}// namespace detail

// ============================================================================
// Public API
// ============================================================================

/// \brief Executes a runtime-sized loop using compile-time unrolling.
///
/// Iterates over `[begin, end)` with the given `step`. Blocks of `Unroll`
/// iterations are emitted via compile-time unrolling.
///
/// When `step == 1`, the call is routed to a compile-time stride path that
/// eliminates per-lane stride multiplication.
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
///   Choose explicitly per call site — no default is provided.
///   Typical starting points: `2` (small codegen), `4` (balanced), `8` (hot loops).
/// \param begin Inclusive start bound.
/// \param end Exclusive end bound.
/// \param step Increment per iteration. Can be negative.
/// \param func Callable invoked for each iteration. Supports two forms:
///   - `func(std::integral_constant<std::size_t, lane>{}, index)` — lane as type
///   - `func(index)` — without lane info
template<std::size_t Unroll, typename T1, typename T2, typename T3, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, T3 step, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2, T3>;
    const T s = static_cast<T>(step);

    if constexpr (std::is_lvalue_reference_v<Func>) {
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;

        if (s == static_cast<T>(1)) {
            detail::dynamic_for_impl_ct_stride<1, T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), func, form_tag{});
        } else {
            detail::dynamic_for_impl_general<T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), s, func, form_tag{});
        }
    } else {
        std::remove_reference_t<Func> callable(std::forward<Func>(func));
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;

        if (s == static_cast<T>(1)) {
            detail::dynamic_for_impl_ct_stride<1, T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
        } else {
            detail::dynamic_for_impl_general<T, callable_t, Unroll>(
              static_cast<T>(begin), static_cast<T>(end), s, callable, form_tag{});
        }
    }
}

/// \brief Executes a runtime-sized loop with compile-time stride.
///
/// The stride is a template parameter, enabling the compiler to:
/// - Replace per-lane stride multiplication with compile-time constants
/// - Eliminate stride from tail dispatch arguments (one fewer register)
/// - Constant-fold stride-direction branches in iteration count
///
/// \tparam Unroll Number of iterations emitted per unrolled block.
/// \tparam Step Compile-time stride (must be non-zero).
/// \param begin Inclusive start bound.
/// \param end Exclusive end bound.
/// \param func Callable invoked for each iteration.
template<std::size_t Unroll, std::intmax_t Step, typename T1, typename T2, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    static_assert(Unroll > 0, "dynamic_for requires Unroll > 0");
    static_assert(Step != 0, "dynamic_for requires Step != 0");
    static_assert(
      Unroll <= detail::kMaxStaticLoopBlock, "dynamic_for supports unroll factors up to kMaxStaticLoopBlock");

    using T = std::common_type_t<T1, T2>;

    if constexpr (std::is_lvalue_reference_v<Func>) {
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        detail::dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), func, form_tag{});
    } else {
        std::remove_reference_t<Func> callable(std::forward<Func>(func));
        using callable_t = std::remove_reference_t<Func>;
        using form_tag = detail::callable_form_t<callable_t, T>;
        detail::dynamic_for_impl_ct_stride<Step, T, callable_t, Unroll>(
          static_cast<T>(begin), static_cast<T>(end), callable, form_tag{});
    }
}

/// \brief Executes a runtime-sized loop using compile-time unrolling with
/// automatic step detection.
///
/// If `begin <= end`, step is +1.
/// If `begin > end`, step is -1.
template<std::size_t Unroll, typename T1, typename T2, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(T1 begin, T2 end, Func &&func) {
    using T = std::common_type_t<T1, T2>;
    T s_begin = static_cast<T>(begin);
    T s_end = static_cast<T>(end);
    T step = (s_begin <= s_end) ? static_cast<T>(1) : static_cast<T>(-1);

    dynamic_for<Unroll>(s_begin, s_end, step, std::forward<Func>(func));
}

/// \brief Executes a runtime-sized loop from zero using compile-time unrolling.
///
/// This overload iterates over the range `[0, count)`.
template<std::size_t Unroll, typename Func>
POET_FORCEINLINE constexpr void dynamic_for(std::size_t count, Func &&func) {
    dynamic_for<Unroll>(static_cast<std::size_t>(0), count, std::size_t{ 1 }, std::forward<Func>(func));
}

}// namespace poet


#if __cplusplus >= 202002L
#include <ranges>
#include <tuple>

namespace poet {

// Adaptor holds the user callable.
// Template ordering: Func first (deduced), Unroll second (required).
template<typename Func, std::size_t Unroll> struct dynamic_for_adaptor {
    Func func;
    constexpr explicit dynamic_for_adaptor(Func f) : func(std::move(f)) {}
};

// Range overload: accept any std::ranges::range.
// Interprets the range as a sequence of consecutive indices starting at *begin(range).
// This implementation computes the distance by iterating the range (works even when not sized).
template<typename Func, std::size_t Unroll, typename Range>
requires std::ranges::range<Range> void operator|(Range const &r, dynamic_for_adaptor<Func, Unroll> const &ad) {
    auto it = std::ranges::begin(r);
    auto it_end = std::ranges::end(r);

    if (it == it_end) return;// empty range

    using ValT = std::remove_reference_t<decltype(*it)>;
    ValT start = *it;

    std::size_t count = 0;
    for (auto jt = it; jt != it_end; ++jt) ++count;

    // Call dynamic_for with [start, start+count) using step = +1
    poet::dynamic_for<Unroll>(start, static_cast<ValT>(start + static_cast<ValT>(count)), ad.func);
}

// Tuple overload: accept tuple-like (begin, end, step)
template<typename Func, std::size_t Unroll, typename B, typename E, typename S>
void operator|(std::tuple<B, E, S> const &t, dynamic_for_adaptor<Func, Unroll> const &ad) {
    auto [b, e, s] = t;
    poet::dynamic_for<Unroll>(b, e, s, ad.func);
}

// Helper to construct adaptor with type deduction
template<std::size_t U, typename F> constexpr auto make_dynamic_for(F &&f) -> dynamic_for_adaptor<std::decay_t<F>, U> {
    return dynamic_for_adaptor<std::decay_t<F>, U>(std::forward<F>(f));
}

}// namespace poet
#endif// __cplusplus >= 202002L

// END_FILE: include/poet/core/dynamic_for.hpp
/* End inline (angle): include/poet/core/dynamic_for.hpp */
/* Begin inline (angle): include/poet/core/static_dispatch.hpp */
/* Skipped already inlined: include/poet/core/static_dispatch.hpp */
/* End inline (angle): include/poet/core/static_dispatch.hpp */
/* Begin inline (angle): include/poet/core/static_for.hpp */
/* Skipped already inlined: include/poet/core/static_for.hpp */
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
/// - POET_NOINLINE: Prevents function inlining
/// - POET_HOT_LOOP: Hot path optimization with aggressive inlining
/// - POET_LIKELY / POET_UNLIKELY: Branch prediction hints
/// - POET_ASSUME / POET_ASSUME_NOT_NULL: Compiler assumption hints
/// - POET_CPP20_CONSTEVAL / POET_CPP20_CONSTEXPR / POET_CPP23_CONSTEXPR: Feature detection
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
// Undefine POET_HOT_LOOP
// ============================================================================
#ifdef POET_HOT_LOOP
#undef POET_HOT_LOOP
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

// END_FILE: include/poet/core/undef_macros.hpp
/* End inline (angle): include/poet/core/undef_macros.hpp */
// NOLINTEND(llvm-include-order)
// clang-format on
// END_FILE: include/poet/poet.hpp

#endif // POET_SINGLE_HEADER_GOLDBOT_HPP
