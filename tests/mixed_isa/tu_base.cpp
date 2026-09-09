// Baseline TU: no arch flags, detects SSE2.

#include <poet/core/cpu_info.hpp>

static_assert(poet::detected_isa() == poet::instruction_set::sse2, "baseline TU must detect sse2");
static_assert(poet::vector_register_count() == 16, "baseline TU must see 16 vector registers");
static_assert(poet::vector_width_bits() == 128, "baseline TU must see 128-bit vectors");

auto regs_base() -> std::size_t {
    static std::size_t (*volatile query)() = &poet::vector_register_count<>;
    return query();
}

auto width_base() -> std::size_t {
    static std::size_t (*volatile query)() = &poet::vector_width_bits<>;
    return query();
}
