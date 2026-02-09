#ifndef POET_CORE_MACROS_HPP
#define POET_CORE_MACROS_HPP

/// \file macros.hpp
/// \brief Compiler-specific macros and attributes for portability and optimization.
///
/// This header provides portable macros for compiler-specific features including:
/// - Unreachable code markers for optimization
/// - Branch prediction hints
/// - Forced inlining
///
/// Supports: GCC, Clang, MSVC, and other C++17-compliant compilers.

// ============================================================================
// POET_UNREACHABLE: Mark code paths as unreachable for optimization
// ============================================================================
/// \def POET_UNREACHABLE
/// \brief Indicates that a code path is unreachable, enabling aggressive optimization.
///
/// This macro tells the compiler that a particular code path cannot be reached at runtime,
/// allowing it to:
/// - Eliminate dead code
/// - Optimize surrounding control flow
/// - Improve register allocation
/// - Generate more efficient branch code
///
/// **Warning**: Using this macro on a reachable code path results in undefined behavior.
/// Only use when you can prove the path is truly unreachable (e.g., after exhaustive
/// switch cases, following static_assert conditions that cover all possibilities).
///
/// Example:
/// ```cpp
/// int get_sign(int x) {
///     if (x > 0) return 1;
///     if (x < 0) return -1;
///     if (x == 0) return 0;
///     POET_UNREACHABLE();  // All cases covered
/// }
/// ```

#if defined(__GNUC__) || defined(__clang__)
    // GCC and Clang: Use __builtin_unreachable()
    // Available since GCC 4.5 and Clang 3.0
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
    // Microsoft Visual C++: Use __assume(false)
    // Available since MSVC 2008 (VS 9.0)
    #define POET_UNREACHABLE() __assume(false)

#else
    // Fallback for other compilers: no-op
    // The code will still be correct, just potentially less optimized
    #define POET_UNREACHABLE() \
        do {                   \
        } while (false)

#endif

// ============================================================================
// POET_FLATTEN: Request flattening of callees into this function
// ============================================================================
/// \def POET_FLATTEN
/// \brief Request that all callees be inlined into this function (GCC/Clang).
///
/// **What is flattening?**
/// Unlike `inline` (which asks to inline THIS function into callers), `flatten`
/// asks to inline ALL callees INTO this function. This creates a single, large
/// function body containing all sub-calls.
///
/// **Benefits:**
/// - **Unified register allocation**: Compiler sees entire computation at once,
///   enabling optimal register allocation across what would be multiple function calls
/// - **Cross-function optimization**: Enables optimizations that span function boundaries
///   (dead code elimination, constant propagation, common subexpression elimination)
/// - **Reduced call overhead**: Eliminates function call instructions entirely
/// - **Better loop optimization**: Inlined code can be analyzed for vectorization and unrolling
///
/// **Costs:**
/// - **Code size**: Can significantly increase binary size if applied broadly
/// - **Compile time**: Increases compilation time proportionally to inlined code size
/// - **Instruction cache**: Larger functions may reduce I-cache effectiveness
///
/// **Use cases in POET:**
/// - Public API entry points (`dynamic_for`, `static_for`) for cross-TU inlining
/// - Hot loops with multiple helper functions (enables unified register allocation)
/// - Template instantiation points to reduce bloat from deeply nested calls
///
/// **Example:**
/// ```cpp
/// // Without POET_FLATTEN: 3 function calls, register spills between calls
/// inline void process(int* data, size_t n) {
///     validate(data, n);  // Call 1
///     transform(data, n); // Call 2
///     finalize(data, n);  // Call 3
/// }
///
/// // With POET_FLATTEN: All inlined, unified register allocation
/// POET_FLATTEN
/// inline void process(int* data, size_t n) {
///     validate(data, n);  // Inlined
///     transform(data, n); // Inlined
///     finalize(data, n);  // Inlined
///     // Compiler sees full picture, optimizes as single function
/// }
/// ```
///
/// **Note**: Only available on GCC/Clang. No-op on other compilers.
#if defined(__GNUC__) || defined(__clang__)
    #define POET_FLATTEN __attribute__((flatten))
#else
    #define POET_FLATTEN
#endif

// ============================================================================
// POET_FORCEINLINE: Force function inlining across compilers
// ============================================================================
/// \def POET_FORCEINLINE
/// \brief Forces the compiler to inline a function regardless of heuristics.
///
/// Use this for small, performance-critical functions where inlining is essential.
/// The compiler's inline heuristics may refuse to inline certain functions,
/// especially in debug builds or when functions are too large. This macro
/// overrides those heuristics.
///
/// **Warning**: Overuse can increase code size and compilation time. Use sparingly
/// for hot-path functions where profiling shows inlining improves performance.
///
/// Example:
/// ```cpp
/// POET_FORCEINLINE int add(int a, int b) {
///     return a + b;
/// }
/// ```

#ifdef _MSC_VER
    // MSVC: __forceinline
    #define POET_FORCEINLINE __forceinline

#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: __attribute__((always_inline)) inline
    #define POET_FORCEINLINE inline __attribute__((always_inline))

#else
    // Fallback: standard inline (compiler may still ignore it)
    #define POET_FORCEINLINE inline

#endif

// ============================================================================
// POET_RESTRICT: portable no-alias hint
// ============================================================================
/// \def POET_RESTRICT
/// \brief Compiler-portable `restrict` keyword for pointer/reference parameters.
///
/// **What is restrict?**
/// The `restrict` qualifier (from C99, adopted by C++ compilers as extensions)
/// tells the compiler that a pointer is the ONLY way to access the underlying
/// object during the pointer's lifetime. No other pointer aliases the same memory.
///
/// **Enabled optimizations:**
/// - **Register allocation**: Values can be cached in registers without reload checks
/// - **Reordering**: Memory operations can be reordered freely (no aliasing conflicts)
/// - **Vectorization**: Loop vectorizer can assume no overlap between arrays
/// - **Store forwarding**: Stores can be forwarded directly to subsequent loads
///
/// **Safety requirements (CRITICAL):**
/// - Only use when you can GUARANTEE no aliasing exists
/// - Violating this produces undefined behavior (silent data corruption)
/// - Safe for: function-local arrays, distinct function parameters
/// - Unsafe for: class members, returned pointers, overlapping buffers
///
/// **Example:**
/// ```cpp
/// // SAFE: Separate arrays, guaranteed non-overlapping
/// void add_arrays(float* POET_RESTRICT dst,
///                 const float* POET_RESTRICT src1,
///                 const float* POET_RESTRICT src2, size_t n) {
///     for (size_t i = 0; i < n; ++i) {
///         dst[i] = src1[i] + src2[i];  // Vectorizes freely
///     }
/// }
///
/// // UNSAFE: dst might alias src (e.g., dst == src + 1)
/// void shift_array(float* POET_RESTRICT dst,
///                  const float* POET_RESTRICT src, size_t n) {
///     for (size_t i = 0; i < n; ++i) {
///         dst[i] = src[i];  // UB if dst and src overlap!
///     }
/// }
/// ```
///
/// **POET usage:**
/// Used on function pointers in `dynamic_for_impl` to enable better register
/// allocation for the callable object, reducing register spills in hot loops.
///
/// **Portability:**
/// - GCC/Clang: `__restrict__`
/// - MSVC: `__restrict`
/// - Other compilers: No-op (code still correct, just potentially slower)
//
#ifdef _MSC_VER
    #define POET_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define POET_RESTRICT __restrict__
#else
    #define POET_RESTRICT
#endif

// ============================================================================
// POET_NOINLINE: Prevent function inlining
// ============================================================================
/// \def POET_NOINLINE
/// \brief Prevent the compiler from inlining this function.
///
/// **Use cases:**
/// 1. **Reduce code bloat**: Move large template instantiations out-of-line to
///    avoid duplicating code at every call site
/// 2. **Instruction cache pressure**: Keep hot paths compact by outlining cold
///    paths (error handling, rarely-executed branches)
/// 3. **Debugging**: Preserve call stack for better profiling/debugging
/// 4. **Binary size**: Control binary size in size-sensitive applications
///
/// **POET usage:**
/// Applied to helper functions that wrap template `operator()` calls in
/// `static_for_impl`. This reduces caller bloat while still enabling the
/// inner template operator to be fully inlined where it matters.
///
/// **Trade-offs:**
/// - Benefits: Smaller binary, better I-cache utilization, faster compile times
/// - Costs: Function call overhead (typically 1-3ns for simple functions)
///
/// **Example:**
/// ```cpp
/// // Without NOINLINE: This large function duplicated at every call site
/// template<typename T>
/// inline void complex_error_handler(T&& data) {
///     // 200+ lines of error handling code...
/// }
///
/// // With NOINLINE: Single copy in binary, called when needed
/// template<typename T>
/// POET_NOINLINE void complex_error_handler(T&& data) {
///     // 200+ lines of error handling code...
///     // Called rarely, so overhead acceptable
/// }
/// ```
///
/// **Pattern in POET:**
/// ```cpp
/// // Public API: inlined template with heavy instantiation
/// template<typename Func>
/// POET_FLATTEN inline void static_for(..., Func&& func) {
///     // Lots of template code
///     static_for_helper(std::forward<Func>(func));  // Call helper
/// }
///
/// // Helper: noinline to avoid duplicating instantiation
/// template<typename Func>
/// POET_NOINLINE void static_for_helper(Func& func) {
///     func.template operator()<N>();  // Actual work
/// }
/// ```
#ifdef _MSC_VER
    #define POET_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define POET_NOINLINE __attribute__((noinline))
#else
    #define POET_NOINLINE
#endif

// ============================================================================
// POET_LIKELY / POET_UNLIKELY: Branch prediction hints
// ============================================================================
/// \def POET_LIKELY
/// \brief Hints to the compiler that a condition is likely to be true.
///
/// This macro maps to C++20's [[likely]] attribute when available, or
/// compiler-specific built-ins on older standards. It helps the compiler:
/// - Arrange code layout for better instruction cache utilization
/// - Optimize branch prediction
/// - Improve pipeline efficiency
///
/// Use this for conditions that are true in the common case (>95% of the time).
///
/// Example:
/// ```cpp
/// if POET_LIKELY(ptr != nullptr) {
///     // Common path: pointer is valid
/// }
/// ```

/// \def POET_UNLIKELY
/// \brief Hints to the compiler that a condition is unlikely to be true.
///
/// This macro maps to C++20's [[unlikely]] attribute when available, or
/// compiler-specific built-ins on older standards. Use this for rare conditions
/// like error paths, edge cases, or exceptional situations (<5% of the time).
///
/// Example:
/// ```cpp
/// if POET_UNLIKELY(error_occurred) {
///     // Rare path: handle error
/// }
/// ```

#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: Use __builtin_expect
    // Available since GCC 3.0 and Clang 1.0
    // Returns x, but tells the compiler that x is likely/unlikely to be true
    // Note: We don't use C++20 [[likely]]/[[unlikely]] attributes here because
    // they cannot be portably wrapped in macros that work with if statements.
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_LIKELY(x) __builtin_expect(!!(x), 1)
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNLIKELY(x) __builtin_expect(!!(x), 0)

#else
    // Fallback for compilers without __builtin_expect: no hints
    // The code will still be correct, just potentially less optimized
    #define POET_LIKELY(x) (x)
    #define POET_UNLIKELY(x) (x)

#endif

// ============================================================================
// poet_count_trailing_zeros: Portable count trailing zeros (CTZ) operation
// ============================================================================
/// \brief Counts the number of trailing zero bits in an unsigned integer.
///
/// This function provides a portable implementation of the "count trailing zeros"
/// bit operation, which is essential for power-of-2 optimizations. It returns the
/// number of consecutive zero bits starting from the least significant bit.
///
/// Examples:
/// - poet_count_trailing_zeros(1)  = 0  (binary: ...0001)
/// - poet_count_trailing_zeros(2)  = 1  (binary: ...0010)
/// - poet_count_trailing_zeros(4)  = 2  (binary: ...0100)
/// - poet_count_trailing_zeros(8)  = 3  (binary: ...1000)
/// - poet_count_trailing_zeros(12) = 2  (binary: ...1100)
///
/// **Warning**: Calling this function with value 0 results in undefined behavior.
/// Some compiler intrinsics return the bit width (32 or 64), while others are
/// undefined. Always ensure the input is non-zero.
///
/// \param value An unsigned integer (must be non-zero)
/// \return Number of trailing zero bits
///
/// Implementation notes:
/// - C++20: Uses std::countr_zero from <bit> header (standardized)
/// - GCC/Clang: Uses __builtin_ctz() (single instruction: BSF on x86, CLZ on ARM)
/// - MSVC: Uses _BitScanForward() intrinsic (BSF instruction on x86)
/// - Fallback: Software implementation for other compilers

#if __cplusplus >= 202002L
// C++20: Use std::countr_zero from <bit> header (standardized implementation)
#include <bit>

constexpr auto poet_count_trailing_zeros(unsigned int value) noexcept -> unsigned int {
    return static_cast<unsigned int>(std::countr_zero(value));
}

// NOLINTNEXTLINE(google-runtime-int)
constexpr auto poet_count_trailing_zeros(unsigned long value) noexcept -> unsigned int {
    return static_cast<unsigned int>(std::countr_zero(value));
}

constexpr auto poet_count_trailing_zeros(unsigned long long value) noexcept -> unsigned int {
    return static_cast<unsigned int>(std::countr_zero(value));
}

#elif defined(__GNUC__) || defined(__clang__)

// GCC/Clang: Use __builtin_ctz family
// These map to single instructions on most architectures (BSF, CLZ, etc.)
constexpr auto poet_count_trailing_zeros(unsigned int value) noexcept -> unsigned int {
    return static_cast<unsigned int>(__builtin_ctz(value));
}

// NOLINTNEXTLINE(google-runtime-int)
constexpr auto poet_count_trailing_zeros(unsigned long value) noexcept -> unsigned int {
    return static_cast<unsigned int>(__builtin_ctzl(value));
}

// NOLINTNEXTLINE(google-runtime-int)
constexpr auto poet_count_trailing_zeros(unsigned long long value) noexcept -> unsigned int {
    return static_cast<unsigned int>(__builtin_ctzll(value));
}

#elif defined(_MSC_VER)

// MSVC: Use _BitScanForward intrinsics
// These are not constexpr, but still map to efficient BSF/BSF64 instructions
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

// Overload for unsigned int (maps to unsigned long on MSVC)
inline unsigned int poet_count_trailing_zeros(unsigned int value) noexcept {
    return poet_count_trailing_zeros(static_cast<unsigned long>(value));
}

#else

// Fallback: Software implementation using DeBruijn sequence
// This is a fast software algorithm for CTZ, used when compiler intrinsics
// are unavailable. It uses a multiply-shift lookup table approach.
// Reference: https://graphics.stanford.edu/~seander/bithacks.html

namespace detail {
    constexpr unsigned char debruijn_ctz_table[32] = {
        0,  1,  28, 2,  29, 14, 24, 3,  30, 22, 20, 15, 25, 17, 4,  8,
        31, 27, 13, 23, 21, 19, 16, 7,  26, 12, 18, 6,  11, 5,  10, 9
    };
}

inline constexpr unsigned int poet_count_trailing_zeros(unsigned int value) noexcept {
    // DeBruijn multiply-shift algorithm for 32-bit integers
    // Isolate the rightmost 1-bit, multiply by magic constant, shift and lookup
    return detail::debruijn_ctz_table[((value & -value) * 0x077CB531U) >> 27];
}

inline constexpr unsigned int poet_count_trailing_zeros(unsigned long value) noexcept {
    return poet_count_trailing_zeros(static_cast<unsigned int>(value));
}

inline constexpr unsigned int poet_count_trailing_zeros(unsigned long long value) noexcept {
    // For 64-bit, check lower 32 bits first
    const auto lower = static_cast<unsigned int>(value);
    if (lower != 0) {
        return poet_count_trailing_zeros(lower);
    }
    // If lower 32 bits are zero, count in upper 32 bits and add 32
    const auto upper = static_cast<unsigned int>(value >> 32);
    return 32 + poet_count_trailing_zeros(upper);
}

#endif

// ============================================================================
// Optimization level detection
// ============================================================================
// Detect if we're building with aggressive optimization (-O3 or equivalent)
// This allows us to conditionally enable extra optimizations on top of -O3

#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
    // GCC/Clang: __OPTIMIZE__ is defined for -O1/-O2/-O3, but not for -Os
    // We assume -O2 or higher (can't distinguish -O2 from -O3 reliably)
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_HIGH_OPTIMIZATION 1
#elif defined(_MSC_VER) && !defined(_DEBUG) && defined(NDEBUG)
    // MSVC: Release mode with NDEBUG defined
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_HIGH_OPTIMIZATION 1
#else
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_HIGH_OPTIMIZATION 0
#endif

// ============================================================================
// POET_HOT_LOOP: Hot path optimization with enhanced register utilization
// ============================================================================
/// \def POET_HOT_LOOP
/// \brief Marks a function as performance-critical with extra register optimization.
///
/// This attribute optimizes for:
/// - Maximum inlining (reduces call overhead, keeps values in registers)
/// - Hot path code layout (better instruction cache utilization)
/// - Enhanced register allocation (when building with -O3)
///
/// **When building with -O3 or equivalent:**
/// Applies additional hints on top of -O3 for better register utilization:
/// - Omit frame pointer (frees up a register)
/// - Aggressive inlining (reduces register spills across calls)
///
/// **Safe and portable**: No fast-math, no CPU-specific instructions, no LTO.
///
/// Use this on functions that:
/// 1. Are called millions of times (profiling-identified hotspots)
/// 2. Have tight loops with high register pressure
/// 3. Benefit from eliminating call overhead
///
/// Example:
/// ```cpp
/// template<typename Func, size_t N>
/// POET_HOT_LOOP
/// inline void execute_block(Func&& f) {
///     // Hot execution path with optimal register usage
/// }
/// ```

#if defined(__GNUC__) && !defined(__clang__)
    // GCC: When optimizing, add extra register-focused hints
    #if POET_HIGH_OPTIMIZATION
        // With -O3: Add loop unrolling on top (not included in -O3 by default)
        // Note: -frename-registers and -fomit-frame-pointer are already in -O2+ by default
        // Note: -funroll-loops is NOT in -O3, so we add it for hot paths
        // Hot-loop attribute: keep as a hint but do NOT force optimize flags here.
        // Use scoped `POET_PUSH_OPTIMIZE` / `POET_POP_OPTIMIZE` around hot helpers
        // to apply IRA tuning where needed.
        #define POET_HOT_LOOP inline __attribute__((hot, always_inline))
    #else
        // Without -O3: Just basic hot path hints
        #define POET_HOT_LOOP inline __attribute__((hot, always_inline))
    #endif

#elif defined(__clang__)
    // Clang: hot + always_inline
    // Note: Clang's greedy register allocator at -O2+ already handles register allocation
    //       optimally, and -fomit-frame-pointer is enabled by default at -O1+ on x86-64
    // Clang doesn't support per-function optimize() attribute with arbitrary flags
    // Users should compile with -funroll-loops at command-line for loop unrolling
    #define POET_HOT_LOOP inline __attribute__((hot, always_inline))

#elif defined(_MSC_VER)
    // MSVC: Force inlining only
    // Note: MSVC does NOT support per-function optimization attributes like GCC
    // Loop unrolling is automatic at /O2 and cannot be controlled per-function
    // Frame pointer omission is controlled by /Oy flag (enabled in /O2)
    #define POET_HOT_LOOP __forceinline

#else
    // Fallback: Standard inline
    #define POET_HOT_LOOP inline

#endif

// ============================================================================
// POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE: Scoped optimization control
// ============================================================================
/// \def POET_PUSH_OPTIMIZE
/// \brief Enable aggressive optimizations with enhanced register utilization.
///
/// \def POET_POP_OPTIMIZE
/// \brief Restore previous optimization settings.
///
/// These macros enable aggressive optimizations for specific code sections:
/// - **At -O0/-O1/-O2**: Enables -O3 level optimizations
/// - **At -O3**: Adds ONLY extra register-focused flags on top (doesn't re-specify -O3)
///
/// **Extra optimizations applied when already at -O3 (GCC):**
/// - `-funroll-loops` (unroll loops for better ILP and register utilization)
/// - Note: `-frename-registers` and `-fomit-frame-pointer` are already in -O2+ (GCC 7+)
/// - This is a safe, portable flag that works on top of -O3
///
/// **For Clang users:**
/// - Clang's greedy register allocator already optimizes register usage at -O2+
/// - Add `-funroll-loops` to your compile flags for loop unrolling (can't be set per-function)
/// - IMPORTANT: These macros are NO-OPS on Clang (Clang can only disable opts, not enable them)
/// - You MUST compile with `-O3` globally for these optimizations
///
/// **For MSVC users:**
/// - Loop unrolling is automatic at `/O2` and cannot be controlled explicitly
/// - MSVC does not provide flags or pragmas for loop unrolling control
///
/// **Respects user's flags:**
/// - GCC: push/pop saves and restores your global flags - no interference!
/// - If you have `-funroll-loops` globally, it stays enabled after POP
/// - If you don't have it, it's only enabled within PUSH/POP block
///
/// **Important**: If you explicitly use `-fno-omit-frame-pointer` (e.g., for profiling/debugging),
/// do NOT use these macros as they will override your setting.
///
/// **Excluded (safe and portable):**
/// - No fast-math (preserves IEEE semantics)
/// - No march=native (portable binaries)
/// - No LTO (local scope only)
///
/// Example:
/// ```cpp
/// POET_PUSH_OPTIMIZE
/// template<typename T>
/// inline void critical_hotspot(T* data, size_t n) {
///     // Gets full optimization + extra register hints
///     for (size_t i = 0; i < n; ++i) {
///         data[i] = compute(data[i]);
///     }
/// }
/// POET_POP_OPTIMIZE
/// ```

#ifdef __clang__
    // Clang: No-op (Clang's pragma can only DISABLE optimizations, not enable them)
    // #pragma clang optimize on/off can only disable optimizations, not enable them beyond
    // command-line flags. According to LLVM docs: "A stray #pragma clang optimize on does
    // not selectively enable additional optimizations when compiling at low optimization levels."
    // Users must compile with -O3 globally to get optimizations
    #define POET_PUSH_OPTIMIZE
    #define POET_POP_OPTIMIZE

#elif defined(__GNUC__)
    // GCC: Allow disabling scoped push optimizations for A/B testing by
    // defining `POET_DISABLE_PUSH_OPTIMIZE` at compile time.
    #if defined(POET_DISABLE_PUSH_OPTIMIZE)
        #define POET_PUSH_OPTIMIZE
        #define POET_POP_OPTIMIZE
    #else
        // Use pragma GCC optimize with push/pop to preserve user's flags
        // push_options saves current optimization state to a stack
        // pop_options restores from stack (preserves user's global flags!)
        #if POET_HIGH_OPTIMIZATION
            // Building with -O3 already: Apply register allocation tuning and IRA
            // (Integrated Register Allocator) pressure-aware optimizations for hot paths.
            //
            // These flags are split into separate pragmas because some older GCC versions
            // reject combined option strings. Each flag is independent and focuses on
            // register allocation quality.
            //
            // Flag explanations:
            // 1. -fira-hoist-pressure: Enable register-pressure-aware code hoisting
            //    - Normally, GCC hoists loop-invariant code out of loops aggressively
            //    - This flag makes hoisting aware of register pressure inside loops
            //    - Prevents hoisting when it would cause register spills in hot loops
            //    - Trade-off: Slight code duplication vs better register allocation
            //
            // 2. -fno-ira-share-spill-slots: Disable spill slot sharing
            //    - By default, GCC reuses stack slots for different spilled values
            //    - Disabling sharing uses more stack space but enables better ILP
            //      (Instruction Level Parallelism) by reducing false dependencies
            //    - Values can be reloaded in parallel without waiting for stores
            //    - Trade-off: ~16-32 bytes more stack vs better pipeline utilization
            //
            // 3. -frename-registers: Break false register dependencies
            //    - Renames registers in post-RA (Register Allocation) scheduling
            //    - Eliminates false WAR (Write-After-Read) and WAW (Write-After-Write)
            //      dependencies that aren't semantic but limit out-of-order execution
            //    - Particularly effective on x86-64 with many available registers
            //    - Note: This is already enabled by default in -O2+ on GCC 7+, but
            //      we explicitly request it for older GCC versions and for clarity
            #define POET_PUSH_OPTIMIZE _Pragma("GCC push_options") \
                                       _Pragma("GCC optimize(\"-fira-hoist-pressure\")") \
                                       _Pragma("GCC optimize(\"-fno-ira-share-spill-slots\")") \
                                       _Pragma("GCC optimize(\"-frename-registers\")")
            #define POET_POP_OPTIMIZE  _Pragma("GCC pop_options")
        #else
            // Building without -O3: Enable -O3 for this section only
            // The push/pop ensures we don't affect code outside this block
            #define POET_PUSH_OPTIMIZE _Pragma("GCC push_options") \
                                       _Pragma("GCC optimize(\"-O3\")")
            #define POET_POP_OPTIMIZE  _Pragma("GCC pop_options")
        #endif
    #endif

#elif defined(_MSC_VER)
    // MSVC: Use #pragma optimize
    // "gt" = (g)lobal optimizations + (t)ime optimization
    // Note: MSVC automatically unrolls loops at /O2 - no separate flag like GCC's -funroll-loops
    // Note: /Oy (omit frame pointer) is included in /O2 by default
    // This is the most aggressive optimization MSVC allows at function scope
    #define POET_PUSH_OPTIMIZE __pragma(optimize("gt", on))
    #define POET_POP_OPTIMIZE  __pragma(optimize("", on))

#else
    // Fallback: No-op for unknown compilers
    #define POET_PUSH_OPTIMIZE
    #define POET_POP_OPTIMIZE

#endif

// ============================================================================
// POET_UNROLL_LOOP: Loop-specific unrolling hints
// ============================================================================
/// \def POET_UNROLL_LOOP(N)
/// \brief Suggest unrolling the immediately following loop N times.
///
/// Place this pragma immediately before a for/while/do-while loop to request
/// unrolling. The compiler may ignore the hint if unrolling would be detrimental.
///
/// **Parameters**:
/// - N: Unroll factor (positive integer), or:
///   - Use POET_UNROLL_LOOP_FULL to request complete unrolling
///   - Use POET_UNROLL_LOOP_DISABLE to prevent unrolling
///
/// Example:
/// ```cpp
/// POET_UNROLL_LOOP(4)
/// for (int i = 0; i < n; ++i) {
///     process(data[i]);
/// }
/// ```

#ifdef __clang__
    // Clang: Use #pragma clang loop unroll_count(N)
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP(N) _Pragma("clang loop unroll_count(" #N ")")
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP_FULL _Pragma("clang loop unroll(full)")
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP_DISABLE _Pragma("clang loop unroll(disable)")

#elif defined(__GNUC__)
    // GCC: Use #pragma GCC unroll N
    // Note: Available since GCC 8
    #if __GNUC__ >= 8
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
        #define POET_UNROLL_LOOP(N) _Pragma("GCC unroll " #N)
        // GCC doesn't have "full unroll" pragma, use large number
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
        #define POET_UNROLL_LOOP_FULL _Pragma("GCC unroll 32767")
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
        #define POET_UNROLL_LOOP_DISABLE _Pragma("GCC unroll 1")
    #else
        // GCC < 8: No support
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
        #define POET_UNROLL_LOOP(N)
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
        #define POET_UNROLL_LOOP_FULL
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
        #define POET_UNROLL_LOOP_DISABLE
    #endif

#elif defined(_MSC_VER)
    // MSVC: No loop unrolling pragma available
    // MSVC does NOT provide a pragma for loop unrolling (unlike GCC/Clang)
    // #pragma loop only controls parallelization/vectorization, not unrolling
    // Loop unrolling happens automatically at /O2 based on compiler heuristics
    // These macros are no-ops on MSVC
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP(N)
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP_FULL
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP_DISABLE

#else
    // Fallback: No-op
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP(N)
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP_FULL
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define POET_UNROLL_LOOP_DISABLE

#endif

// ============================================================================
// C++20 Feature Detection
// ============================================================================
/// \def POET_CPP20_CONSTEVAL
/// \brief Use `consteval` for C++20, fallback to `constexpr` for C++17.
///
/// **Why consteval matters:**
/// - **Guaranteed compile-time**: `consteval` functions MUST be evaluated at
///   compile-time. Attempting runtime use is a compile error.
/// - **Catches errors earlier**: If a function can't be compile-time evaluated,
///   you get a compile error instead of silent runtime fallback
/// - **Zero runtime cost**: Absolutely no runtime overhead possible
///
/// **C++17 fallback:**
/// Uses `constexpr`, which allows both compile-time and runtime evaluation.
/// The compiler decides which to use. Most uses in POET are in constexpr
/// contexts anyway, so they're evaluated at compile-time in both modes.
///
/// **POET usage context:**
/// Used for dispatch table generation functions in `static_dispatch.hpp`:
/// - `make_dispatch_table()`: Generates function pointer arrays at compile-time
/// - `sequence_value_at()`: Extracts values from integer sequences
/// - Dispatch table builders: Create lookup tables during compilation
///
/// In these cases, we WANT compile-time evaluation (tables in `.rodata`),
/// and C++20's `consteval` enforces this guarantee.
///
/// **Example showing compile-time enforcement:**
/// ```cpp
/// // C++20: consteval - MUST be compile-time
/// POET_CPP20_CONSTEVAL int factorial(int n) {
///     return n <= 1 ? 1 : n * factorial(n - 1);
/// }
///
/// constexpr int x = factorial(5);  // OK: compile-time context
/// int runtime_val = 5;
/// int y = factorial(runtime_val);  // C++20: ERROR! Not compile-time
///                                   // C++17: OK, evaluates at runtime
/// ```
///
/// **Note**: POET functions marked with this are designed to be called in
/// compile-time contexts (template parameters, constexpr variables), so the
/// C++17 fallback still works correctly in practice.

#if __cplusplus >= 202002L
    #define POET_CPP20_CONSTEVAL consteval
    #define POET_CPP20_CONSTEXPR constexpr
#else
    #define POET_CPP20_CONSTEVAL constexpr
    #define POET_CPP20_CONSTEXPR constexpr
#endif

#endif// POET_CORE_MACROS_HPP
