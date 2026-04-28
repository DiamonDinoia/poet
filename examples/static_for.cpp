// Example: poet::static_for — compile-time unrolled loops.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_static_for
//   ./build/examples/example_static_for

#include <array>
#include <cstdio>

#include <poet/poet.hpp>

int main() {
    // Each loop writes to a `volatile` sink inside the lambda, so every
    // unrolled iteration is an observable side effect — the compiler can't
    // collapse the work into a single constant store, and the asm pane on
    // Compiler Explorer shows the full unrolled emission.
    volatile int sink_i = 0;
    volatile long sink_l = 0;

    // Basic form: indices are integral_constant, usable as constant indices.
    std::array<int, 4> a{};
    poet::static_for<0, 4>([&](auto I) {
        a[I] = I * I;
        sink_i = a[I];
    });
    std::printf("static_for<0,4> -> %d %d %d %d\n", a[0], a[1], a[2], a[3]);

    // Step.
    long even_sum = 0;
    poet::static_for<0, 10, 2>([&](auto I) {
        even_sum += I;
        sink_l = even_sum;
    });
    std::printf("sum of evens in [0,10) = %ld\n", even_sum);

    // Block size: split a long body into smaller outlined blocks.
    long total = 0;
    poet::static_for<0, 64, 1, 8>([&](auto I) {
        total += I;
        sink_l = total;
    });
    std::printf("sum [0,64) with block=8 = %ld\n", total);

    return 0;
}
