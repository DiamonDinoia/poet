// AVX-512 TU: compiled with -mavx512f.

#include <poet/core/cpu_info.hpp>

static_assert(poet::detected_isa() == poet::instruction_set::avx_512, "avx512 TU must detect avx_512");
static_assert(poet::vector_register_count() == 32, "avx512 TU must see 32 vector registers");
static_assert(poet::vector_width_bits() == 512, "avx512 TU must see 512-bit vectors");

auto regs_avx512() -> std::size_t {
    static std::size_t (*volatile query)() = &poet::vector_register_count<>;
    return query();
}

auto width_avx512() -> std::size_t {
    static std::size_t (*volatile query)() = &poet::vector_width_bits<>;
    return query();
}
