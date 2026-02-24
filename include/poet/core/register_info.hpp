#ifndef POET_CORE_REGISTER_INFO_HPP
#define POET_CORE_REGISTER_INFO_HPP

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
                .gp_registers = 16,
                .vector_registers = 16,// XMM0-XMM15 (128-bit)
                .vector_width_bits = 128,
                .lanes_64bit = 2,
                .lanes_32bit = 4,
                .isa = isa,
            };

        case instruction_set::avx:
        case instruction_set::avx2:
            // AVX/AVX2: 16 YMM registers (256-bit each)
            // AVX2 adds integer vector ops but same register count
            return register_info{
                .gp_registers = 16,
                .vector_registers = 16,// YMM0-YMM15 (256-bit)
                .vector_width_bits = 256,
                .lanes_64bit = 4,
                .lanes_32bit = 8,
                .isa = isa,
            };

        case instruction_set::avx_512:
            // AVX-512: 32 ZMM registers (512-bit each)
            // Also includes 8 mask registers (k0-k7), but we count vector regs
            return register_info{
                .gp_registers = 16,
                .vector_registers = 32,// ZMM0-ZMM31 (512-bit AVX-512)
                .vector_width_bits = 512,
                .lanes_64bit = 8,
                .lanes_32bit = 16,
                .isa = isa,
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
                .gp_registers = 31,
                .vector_registers = 32,// V0-V31 / Z0-Z31
                .vector_width_bits = 128,// Minimum guaranteed
                .lanes_64bit = 2,
                .lanes_32bit = 4,
                .isa = isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // PowerPC Family
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::ppc_altivec:
            // PowerPC AltiVec: 32 vector registers (128-bit each)
            // Also 32 general-purpose registers
            return register_info{
                .gp_registers = 32,
                .vector_registers = 32,// V0-V31 (128-bit AltiVec registers)
                .vector_width_bits = 128,
                .lanes_64bit = 2,
                .lanes_32bit = 4,
                .isa = isa,
            };

        case instruction_set::ppc_vsx:
            // PowerPC VSX: extends AltiVec with 64 vector registers (128-bit each)
            // or 32 registers with doubled width (256-bit nominal)
            return register_info{
                .gp_registers = 32,
                .vector_registers = 64,// VSX0-VSX63 (128-bit per register)
                .vector_width_bits = 128,
                .lanes_64bit = 2,
                .lanes_32bit = 4,
                .isa = isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // MIPS Family
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::mips_msa:
            // MIPS MSA: 32 vector registers (128-bit each)
            // Also 32 general-purpose registers
            return register_info{
                .gp_registers = 32,
                .vector_registers = 32,// W0-W31 (128-bit MSA registers)
                .vector_width_bits = 128,
                .lanes_64bit = 2,
                .lanes_32bit = 4,
                .isa = isa,
            };

            // ────────────────────────────────────────────────────────────────────
            // Generic/Unknown
            // ────────────────────────────────────────────────────────────────────

        case instruction_set::generic:
        default:
            // Conservative fallback: assume x86-64 SSE2-like baseline
            return register_info{
                .gp_registers = 16,
                .vector_registers = 16,
                .vector_width_bits = 128,
                .lanes_64bit = 2,
                .lanes_32bit = 4,
                .isa = instruction_set::generic,
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

#endif// POET_CORE_REGISTER_INFO_HPP
