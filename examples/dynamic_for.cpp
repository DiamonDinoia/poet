// Example: poet::dynamic_for — runtime loops emitted as compile-time blocks.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_dynamic_for
//   ./build/examples/example_dynamic_for

#include <array>
#include <cstddef>
#include <cstdio>
#include <vector>

#include <poet/poet.hpp>

int main() {
    // `volatile n` keeps the loop bound runtime-only so the compiler
    // can't constant-fold the whole iteration on Compiler Explorer.
    volatile std::size_t vol_n = 17;
    const std::size_t n = vol_n;
    std::vector<int> out(n);

    // Basic form: unroll factor 4, single-arg callback.
    poet::dynamic_for<4>(std::size_t{ 0 }, n, [&](std::size_t i) { out[i] = static_cast<int>(i * i); });
    std::printf("dynamic_for<4>: out[16] = %d\n", out[16]);

    // Lane-aware callback for multi-accumulator ILP.
    constexpr std::size_t lanes = 4;
    std::array<double, lanes> acc{};
    poet::dynamic_for<lanes>(
      std::size_t{ 0 }, n, [&](auto lane, std::size_t i) { acc[lane] += static_cast<double>(i); });
    double total = 0.0;
    for (double v : acc) total += v;
    std::printf("lane-aware sum [0,%zu) = %.0f\n", n, total);

    // Compile-time step.
    volatile int vol_end = 16;
    int seen = 0;
    poet::dynamic_for<4, 2>(0, vol_end, [&](int) { ++seen; });
    std::printf("compile-time step=2 over [0,%d): %d iterations\n", vol_end, seen);

    return 0;
}
