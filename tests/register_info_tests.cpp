#include <catch2/catch_test_macros.hpp>
#include <poet/core/register_info.hpp>
#include <type_traits>

#ifdef POET_HAS_HW_DETECTION
#include <poet_detected_isa.hpp>
#endif

#ifdef __linux__
#include <fstream>
#include <string>
#endif

using namespace poet;

// ============================================================================
// Compile-time structural invariants for every ISA
// ============================================================================

template<instruction_set ISA> constexpr bool validate_register_info() {
    auto r = registers_for(ISA);
    return r.isa == ISA && r.gp_registers > 0 && r.vector_registers > 0 && r.vector_width_bits >= 128
           && (r.vector_width_bits & (r.vector_width_bits - 1)) == 0// power of 2
           && r.lanes_32bit >= r.lanes_64bit && r.lanes_64bit == r.vector_width_bits / 64
           && r.lanes_32bit == r.vector_width_bits / 32;
}

static_assert(validate_register_info<instruction_set::generic>());
static_assert(validate_register_info<instruction_set::sse2>());
static_assert(validate_register_info<instruction_set::sse4_2>());
static_assert(validate_register_info<instruction_set::avx>());
static_assert(validate_register_info<instruction_set::avx2>());
static_assert(validate_register_info<instruction_set::avx_512>());
static_assert(validate_register_info<instruction_set::arm_neon>());
static_assert(validate_register_info<instruction_set::arm_sve>());
static_assert(validate_register_info<instruction_set::arm_sve2>());
static_assert(validate_register_info<instruction_set::ppc_altivec>());
static_assert(validate_register_info<instruction_set::ppc_vsx>());
static_assert(validate_register_info<instruction_set::mips_msa>());

// ============================================================================
// Compile-time: validate against hardware ground truth from /proc/cpuinfo
// ============================================================================
// CMake reads /proc/cpuinfo at configure time and populates POET_HW_* macros.
// This test is compiled with -march=native so POET detects the full hardware
// ISA. The static_asserts verify POET agrees with the OS-reported hardware.

#ifdef POET_HAS_HW_DETECTION

static_assert(static_cast<int>(detected_isa()) == POET_HW_ISA,
  "POET detected_isa() disagrees with /proc/cpuinfo hardware ISA");

static_assert(available_registers().gp_registers == POET_HW_GP_REGISTERS, "POET gp_registers disagrees with hardware");

static_assert(available_registers().vector_registers == POET_HW_VECTOR_REGISTERS,
  "POET vector_registers disagrees with hardware");

static_assert(available_registers().vector_width_bits == POET_HW_VECTOR_WIDTH_BITS,
  "POET vector_width_bits disagrees with hardware");

static_assert(available_registers().lanes_64bit == POET_HW_LANES_64BIT, "POET lanes_64bit disagrees with hardware");

static_assert(available_registers().lanes_32bit == POET_HW_LANES_32BIT, "POET lanes_32bit disagrees with hardware");

#endif// POET_HAS_HW_DETECTION

// ============================================================================
// Runtime: Catch2 tests
// ============================================================================

TEST_CASE("instruction_set enum properties") {
    static_assert(std::is_enum_v<instruction_set>);
    static_assert(sizeof(instruction_set) <= 1);
}

TEST_CASE("convenience functions match available_registers") {
    constexpr auto regs = available_registers();

    static_assert(vector_register_count() == regs.vector_registers);
    static_assert(vector_width_bits() == regs.vector_width_bits);
    static_assert(vector_lanes_64bit() == regs.lanes_64bit);
    static_assert(vector_lanes_32bit() == regs.lanes_32bit);
}

#ifdef POET_HAS_HW_DETECTION
TEST_CASE("detected ISA matches hardware (/proc/cpuinfo)") {
    REQUIRE(static_cast<int>(detected_isa()) == POET_HW_ISA);
    auto regs = available_registers();
    REQUIRE(regs.gp_registers == POET_HW_GP_REGISTERS);
    REQUIRE(regs.vector_registers == POET_HW_VECTOR_REGISTERS);
    REQUIRE(regs.vector_width_bits == POET_HW_VECTOR_WIDTH_BITS);
    REQUIRE(regs.lanes_64bit == POET_HW_LANES_64BIT);
    REQUIRE(regs.lanes_32bit == POET_HW_LANES_32BIT);
}
#endif

// ============================================================================
// Linux runtime: validate against /proc/cpuinfo directly
// ============================================================================

#ifdef __linux__

static std::string read_cpuinfo_flags() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.rfind("flags", 0) == 0 || line.rfind("Features", 0) == 0) {
            auto colon = line.find(':');
            if (colon != std::string::npos) { return " " + line.substr(colon + 1) + " "; }
        }
    }
    return "";
}

static bool has_flag(const std::string &flags, const char *flag) {
    std::string needle = std::string(" ") + flag + " ";
    return flags.find(needle) != std::string::npos;
}

TEST_CASE("detected_isa does not overclaim vs /proc/cpuinfo", "[linux]") {
    auto flags = read_cpuinfo_flags();
    REQUIRE_FALSE(flags.empty());

    constexpr auto isa = detected_isa();
    constexpr auto regs = available_registers();

    // Verify POET never claims an ISA the CPU doesn't actually support.
    if (has_flag(flags, "sse2") || has_flag(flags, "avx") || has_flag(flags, "avx2")) {
        switch (isa) {
        case instruction_set::avx_512:
            REQUIRE(has_flag(flags, "avx512f"));
            REQUIRE(regs.vector_width_bits == 512);
            REQUIRE(regs.vector_registers == 32);
            break;
        case instruction_set::avx2:
            REQUIRE(has_flag(flags, "avx2"));
            REQUIRE(regs.vector_width_bits == 256);
            REQUIRE(regs.vector_registers == 16);
            break;
        case instruction_set::avx:
            REQUIRE(has_flag(flags, "avx"));
            REQUIRE(regs.vector_width_bits == 256);
            REQUIRE(regs.vector_registers == 16);
            break;
        case instruction_set::sse4_2:
            REQUIRE(has_flag(flags, "sse4_2"));
            REQUIRE(regs.vector_width_bits == 128);
            REQUIRE(regs.vector_registers == 16);
            break;
        case instruction_set::sse2:
            REQUIRE(has_flag(flags, "sse2"));
            REQUIRE(regs.vector_width_bits == 128);
            REQUIRE(regs.vector_registers == 16);
            break;
        case instruction_set::generic:
            break;
        default:
            FAIL("Detected non-x86 ISA on an x86 machine");
        }
    }

    if (has_flag(flags, "neon") || has_flag(flags, "asimd")) {
        switch (isa) {
        case instruction_set::arm_sve2:
            REQUIRE(has_flag(flags, "sve2"));
            REQUIRE(regs.vector_registers == 32);
            break;
        case instruction_set::arm_sve:
            REQUIRE(has_flag(flags, "sve"));
            REQUIRE(regs.vector_registers == 32);
            break;
        case instruction_set::arm_neon:
            REQUIRE((has_flag(flags, "neon") || has_flag(flags, "asimd")));
            REQUIRE(regs.vector_width_bits == 128);
            REQUIRE(regs.vector_registers == 32);
            break;
        case instruction_set::generic:
            break;
        default:
            FAIL("Detected non-ARM ISA on an ARM machine");
        }
    }
}

#endif// __linux__
