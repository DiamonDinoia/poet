// Example: poet::available_registers and poet::cache_line — compile-time CPU info.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_cpu_info
//   ./build/examples/example_cpu_info

#include <cstdio>

#include <poet/poet.hpp>

int main() {
    constexpr auto regs = poet::available_registers();
    constexpr auto line = poet::cache_line();

    std::printf("ISA enum                : %d\n", static_cast<int>(regs.isa));
    std::printf("GP registers            : %zu\n", regs.gp_registers);
    std::printf("Vector registers        : %zu\n", regs.vector_registers);
    std::printf("Vector width (bits)     : %zu\n", regs.vector_width_bits);
    std::printf("64-bit lanes            : %zu\n", regs.lanes_64bit);
    std::printf("32-bit lanes            : %zu\n", regs.lanes_32bit);
    std::printf("Cache line destructive  : %zu\n", line.destructive_size);
    std::printf("Cache line constructive : %zu\n", line.constructive_size);

    return 0;
}
