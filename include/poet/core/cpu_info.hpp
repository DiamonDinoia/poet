#pragma once

/// \file cpu_info.hpp
/// \brief Compile-time CPU register, vector-width, and cache-line queries.
///
/// Every value resolves from the compiler's target predefines; the result
/// describes the compile target, not the machine that runs the binary.

#include <cstddef>
#include <poet/core/macros.hpp>

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
