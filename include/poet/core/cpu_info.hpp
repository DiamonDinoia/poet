#pragma once

/// \file cpu_info.hpp
/// \brief Compile-time CPU register, vector-width, and cache-line queries.
///
/// Every value resolves from the compiler's target predefines. The result
/// describes the compile target, not the machine that runs the binary. Build
/// with `-march=native` or an explicit `-m<isa>` to get anything above the
/// baseline.

#include <cstddef>
#include <poet/core/macros.hpp>

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
    ppc_vsx,///< PowerPC VSX (128-bit vectors, 64 registers)
    mips_msa,///< MIPS MSA (128-bit vectors)
};

/// \brief Register and vector characteristics for a target ISA.
///
/// Counts are the architectural totals, not the number free for a given
/// function: on x86-64 `gp_registers` includes the stack and frame pointers.
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

    /// SVE is scalable: a width is known only when the build pins one through
    /// `-msve-vector-bits=N`, which sets the same macro macros.hpp locks the
    /// hot paths to. Without a pin, the width is the 128-bit floor the
    /// architecture guarantees.
#if defined(__ARM_FEATURE_SVE_BITS) && __ARM_FEATURE_SVE_BITS > 0
    inline constexpr std::size_t sve_vector_bits = __ARM_FEATURE_SVE_BITS;
#else
    inline constexpr std::size_t sve_vector_bits = 128;
#endif

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

        // MSVC defines none of the __SSE*__ / __ARM_NEON predefines. x64 and
        // ARM64 guarantee SSE2 and NEON respectively, and 32-bit x86 reports its
        // floating-point ISA through _M_IX86_FP. Without these branches, every
        // MSVC build below /arch:AVX would report `generic`.
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
                sve_vector_bits,// vector_width_bits
                sve_vector_bits / 64,// lanes_64bit
                sve_vector_bits / 32,// lanes_32bit
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

/// \brief The ISA the current translation unit compiles for.
///
/// Returns `instruction_set::generic` when no SIMD ISA is enabled.
POET_CPP20_CONSTEVAL auto detected_isa() noexcept -> instruction_set { return detail::detect_instruction_set(); }

/// \brief Register information for `detected_isa()`.
POET_CPP20_CONSTEVAL auto available_registers() noexcept -> register_info {
    return detail::get_register_info(detected_isa());
}

/// \brief Register information for an explicitly named ISA.
/// \param isa The ISA to describe, independent of the build's own target.
POET_CPP20_CONSTEVAL auto registers_for(instruction_set isa) noexcept -> register_info {
    return detail::get_register_info(isa);
}

/// \brief SIMD register count for the detected ISA.
POET_CPP20_CONSTEVAL auto vector_register_count() noexcept -> std::size_t {
    return available_registers().vector_registers;
}

/// \brief SIMD register width in bits for the detected ISA.
POET_CPP20_CONSTEVAL auto vector_width_bits() noexcept -> std::size_t {
    return available_registers().vector_width_bits;
}

/// \brief 64-bit lanes per SIMD register for the detected ISA.
POET_CPP20_CONSTEVAL auto vector_lanes_64bit() noexcept -> std::size_t { return available_registers().lanes_64bit; }

/// \brief 32-bit lanes per SIMD register for the detected ISA.
POET_CPP20_CONSTEVAL auto vector_lanes_32bit() noexcept -> std::size_t { return available_registers().lanes_32bit; }

/// \brief Cache line sizes for the detected target.
POET_CPP20_CONSTEVAL auto cache_line() noexcept -> cache_line_info { return detail::detect_cache_line_info(); }

/// \brief Minimum separation that avoids false sharing.
POET_CPP20_CONSTEVAL auto destructive_interference_size() noexcept -> std::size_t {
    return cache_line().destructive_size;
}

/// \brief Maximum span that shares one cache line.
POET_CPP20_CONSTEVAL auto constructive_interference_size() noexcept -> std::size_t {
    return cache_line().constructive_size;
}

}// namespace poet
