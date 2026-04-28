// Example: poet::dispatch_set — sparse allowed combinations + throw_on_no_match.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_dispatch_set
//   ./build/examples/example_dispatch_set

#include <cstdio>
#include <stdexcept>

#include <poet/poet.hpp>

struct MatMul {
    template<int R, int C> int operator()(int a, int b) const { return R * a + C * b; }
};

int main() {
    using Shapes = poet::dispatch_set<int, poet::tuple_<2, 2>, poet::tuple_<4, 4>, poet::tuple_<2, 4>>;

    // Valid combination: hits the (2, 4) specialization.
    // `volatile` keeps the runtime path live in the asm pane.
    volatile int v_rows = 2;
    volatile int v_cols = 4;
    int rows = v_rows, cols = v_cols;
    int v = poet::dispatch(MatMul{}, Shapes{ rows, cols }, 3, 5);
    std::printf("MatMul<%d,%d>(3,5) = %d\n", rows, cols, v);

    // Missing combination with throw_on_no_match: surfaces the error.
    try {
        volatile int v_bad_rows = 3;
        volatile int v_bad_cols = 3;
        int bad_rows = v_bad_rows;
        int bad_cols = v_bad_cols;
        (void)poet::dispatch(poet::throw_on_no_match, MatMul{}, Shapes{ bad_rows, bad_cols }, 1, 2);
        std::printf("unexpected: dispatch did not throw\n");
        return 1;
    } catch (const std::exception &e) { std::printf("caught expected miss: %s\n", e.what()); }

    return 0;
}
