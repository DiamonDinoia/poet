#pragma once

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
