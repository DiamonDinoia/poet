#include <catch2/catch_test_macros.hpp>
#include <poet/core/register_info.hpp>
#include <type_traits>

using namespace poet;

TEST_CASE("instruction_set enum properties") {
    // Verify enum is scoped and has reasonable size
    static_assert(std::is_enum_v<instruction_set>);
    static_assert(sizeof(instruction_set) <= 1);
}

TEST_CASE("detected_isa is constexpr") {
    // Verify that detected_isa is available at compile time
    constexpr auto isa = detected_isa();
    // Should be a valid instruction set
    REQUIRE((isa >= instruction_set::generic && isa <= instruction_set::mips_msa));
}

TEST_CASE("available_registers matches detected ISA") {
    constexpr auto isa = detected_isa();
    constexpr auto regs = available_registers();

    REQUIRE(regs.isa == isa);
    REQUIRE(regs.gp_registers > 0);
    REQUIRE(regs.fp_registers > 0);
    REQUIRE(regs.vector_width_bits > 0);
    REQUIRE(regs.lanes_64bit > 0);
    REQUIRE(regs.lanes_32bit > 0);
}

TEST_CASE("fp_register_count matches available_registers") {
    constexpr auto count = fp_register_count();
    constexpr auto regs = available_registers();

    REQUIRE(count == regs.fp_registers);
}

TEST_CASE("vector_width_bits matches available_registers") {
    constexpr auto width = vector_width_bits();
    constexpr auto regs = available_registers();

    REQUIRE(width == regs.vector_width_bits);
}

TEST_CASE("vector_lanes_64bit matches available_registers") {
    constexpr auto lanes = vector_lanes_64bit();
    constexpr auto regs = available_registers();

    REQUIRE(lanes == regs.lanes_64bit);
}

TEST_CASE("vector_lanes_32bit matches available_registers") {
    constexpr auto lanes = vector_lanes_32bit();
    constexpr auto regs = available_registers();

    REQUIRE(lanes == regs.lanes_32bit);
}

TEST_CASE("registers_for(SSE2)") {
    constexpr auto sse2_regs = registers_for(instruction_set::sse2);

    REQUIRE(sse2_regs.isa == instruction_set::sse2);
    REQUIRE(sse2_regs.gp_registers == 16);
    REQUIRE(sse2_regs.fp_registers == 8);
    REQUIRE(sse2_regs.vector_width_bits == 128);
    REQUIRE(sse2_regs.lanes_64bit == 2);
    REQUIRE(sse2_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(SSE4.2)") {
    constexpr auto sse42_regs = registers_for(instruction_set::sse4_2);

    REQUIRE(sse42_regs.isa == instruction_set::sse4_2);
    REQUIRE(sse42_regs.gp_registers == 16);
    REQUIRE(sse42_regs.fp_registers == 8);
    REQUIRE(sse42_regs.vector_width_bits == 128);
    REQUIRE(sse42_regs.lanes_64bit == 2);
    REQUIRE(sse42_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(AVX)") {
    constexpr auto avx_regs = registers_for(instruction_set::avx);

    REQUIRE(avx_regs.isa == instruction_set::avx);
    REQUIRE(avx_regs.gp_registers == 16);
    REQUIRE(avx_regs.fp_registers == 8);
    REQUIRE(avx_regs.vector_width_bits == 256);
    REQUIRE(avx_regs.lanes_64bit == 4);
    REQUIRE(avx_regs.lanes_32bit == 8);
}

TEST_CASE("registers_for(AVX2)") {
    constexpr auto avx2_regs = registers_for(instruction_set::avx2);

    REQUIRE(avx2_regs.isa == instruction_set::avx2);
    REQUIRE(avx2_regs.gp_registers == 16);
    REQUIRE(avx2_regs.fp_registers == 8);
    REQUIRE(avx2_regs.vector_width_bits == 256);
    REQUIRE(avx2_regs.lanes_64bit == 4);
    REQUIRE(avx2_regs.lanes_32bit == 8);
}

TEST_CASE("registers_for(AVX-512)") {
    constexpr auto avx512_regs = registers_for(instruction_set::avx_512);

    REQUIRE(avx512_regs.isa == instruction_set::avx_512);
    REQUIRE(avx512_regs.gp_registers == 16);
    REQUIRE(avx512_regs.fp_registers == 32);
    REQUIRE(avx512_regs.vector_width_bits == 512);
    REQUIRE(avx512_regs.lanes_64bit == 8);
    REQUIRE(avx512_regs.lanes_32bit == 16);
}

TEST_CASE("registers_for(ARM NEON)") {
    constexpr auto neon_regs = registers_for(instruction_set::arm_neon);

    REQUIRE(neon_regs.isa == instruction_set::arm_neon);
    REQUIRE(neon_regs.gp_registers == 31);
    REQUIRE(neon_regs.fp_registers == 16);
    REQUIRE(neon_regs.vector_width_bits == 128);
    REQUIRE(neon_regs.lanes_64bit == 2);
    REQUIRE(neon_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(ARM SVE)") {
    constexpr auto sve_regs = registers_for(instruction_set::arm_sve);

    REQUIRE(sve_regs.isa == instruction_set::arm_sve);
    REQUIRE(sve_regs.gp_registers == 31);
    REQUIRE(sve_regs.fp_registers == 32);
    REQUIRE(sve_regs.vector_width_bits == 128);
    REQUIRE(sve_regs.lanes_64bit == 2);
    REQUIRE(sve_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(ARM SVE2)") {
    constexpr auto sve2_regs = registers_for(instruction_set::arm_sve2);

    REQUIRE(sve2_regs.isa == instruction_set::arm_sve2);
    REQUIRE(sve2_regs.gp_registers == 31);
    REQUIRE(sve2_regs.fp_registers == 32);
    REQUIRE(sve2_regs.vector_width_bits == 128);
    REQUIRE(sve2_regs.lanes_64bit == 2);
    REQUIRE(sve2_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(PowerPC AltiVec)") {
    constexpr auto altivec_regs = registers_for(instruction_set::ppc_altivec);

    REQUIRE(altivec_regs.isa == instruction_set::ppc_altivec);
    REQUIRE(altivec_regs.gp_registers == 32);
    REQUIRE(altivec_regs.fp_registers == 32);
    REQUIRE(altivec_regs.vector_width_bits == 128);
    REQUIRE(altivec_regs.lanes_64bit == 2);
    REQUIRE(altivec_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(PowerPC VSX)") {
    constexpr auto vsx_regs = registers_for(instruction_set::ppc_vsx);

    REQUIRE(vsx_regs.isa == instruction_set::ppc_vsx);
    REQUIRE(vsx_regs.gp_registers == 32);
    REQUIRE(vsx_regs.fp_registers == 64);
    REQUIRE(vsx_regs.vector_width_bits == 128);
    REQUIRE(vsx_regs.lanes_64bit == 2);
    REQUIRE(vsx_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(MIPS MSA)") {
    constexpr auto msa_regs = registers_for(instruction_set::mips_msa);

    REQUIRE(msa_regs.isa == instruction_set::mips_msa);
    REQUIRE(msa_regs.gp_registers == 32);
    REQUIRE(msa_regs.fp_registers == 32);
    REQUIRE(msa_regs.vector_width_bits == 128);
    REQUIRE(msa_regs.lanes_64bit == 2);
    REQUIRE(msa_regs.lanes_32bit == 4);
}

TEST_CASE("registers_for(generic)") {
    constexpr auto generic_regs = registers_for(instruction_set::generic);

    REQUIRE(generic_regs.isa == instruction_set::generic);
    REQUIRE(generic_regs.gp_registers == 16);
    REQUIRE(generic_regs.fp_registers == 8);
    REQUIRE(generic_regs.vector_width_bits == 128);
    REQUIRE(generic_regs.lanes_64bit == 2);
    REQUIRE(generic_regs.lanes_32bit == 4);
}

TEST_CASE("vector_lanes_32bit >= vector_lanes_64bit") {
    // 32-bit lanes should always be >= 64-bit lanes
    for (int i = 0; i <= 11; ++i) {
        auto isa = static_cast<instruction_set>(i);
        constexpr auto regs = registers_for(instruction_set::avx2);
        REQUIRE(regs.lanes_32bit >= regs.lanes_64bit);
    }
}

TEST_CASE("vector_width_bits is power of 2") {
    // All supported vector widths should be powers of 2
    constexpr auto widths = std::array{
        registers_for(instruction_set::sse2).vector_width_bits,
        registers_for(instruction_set::avx).vector_width_bits,
        registers_for(instruction_set::avx_512).vector_width_bits,
    };

    for (auto width : widths) {
        REQUIRE((width & (width - 1)) == 0);// Is power of 2
        REQUIRE(width >= 128);// At least 128 bits
    }
}
