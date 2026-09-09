// Mixed-ISA link regression: baseline and AVX-512 TUs must observe their own
// cpu_info answers at runtime. Volatile pointers defeat constant folding --
// the C++17 ODR merge only showed when the call was not folded.

#include <cstdio>

auto regs_base() -> std::size_t;
auto width_base() -> std::size_t;
auto regs_avx512() -> std::size_t;
auto width_avx512() -> std::size_t;

auto main() -> int {
    const auto br = regs_base();
    const auto bw = width_base();
    const auto ar = regs_avx512();
    const auto aw = width_avx512();
    std::printf("baseline TU: regs=%zu width=%zu (expect 16/128)\n", br, bw);
    std::printf("avx512  TU: regs=%zu width=%zu (expect 32/512)\n", ar, aw);
    if (br != 16 || bw != 128 || ar != 32 || aw != 512) {
        std::puts("FAIL: cpu_info answers merged across mixed-ISA TUs");
        return 1;
    }
    std::puts("PASS: per-TU cpu_info answers survived the mixed-ISA link");
    return 0;
}
