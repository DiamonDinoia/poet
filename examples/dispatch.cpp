// Example: poet::dispatch — runtime-to-compile-time specialization.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_dispatch
//   ./build/examples/example_dispatch

#include <cstdio>

#include <poet/poet.hpp>

struct AddN {
    template<int N> int operator()(int x) const { return N + x; }
};

struct Kernel2D {
    template<int R, int C> int operator()(int seed) const { return R * 100 + C * 10 + seed; }
};

int main() {
    // `volatile` runtime inputs keep dispatch from being folded at compile
    // time so the asm pane shows the actual jump table.
    volatile int v_choice = 2;
    int choice = v_choice;
    int y = poet::dispatch(AddN{}, poet::dispatch_param<poet::inclusive_range<0, 4>>{ choice }, 10);
    std::printf("AddN(choice=%d, x=10) = %d\n", choice, y);

    volatile int v_rows = 3;
    volatile int v_cols = 4;
    int rows = v_rows, cols = v_cols;
    int z = poet::dispatch(Kernel2D{},
      poet::dispatch_param<poet::inclusive_range<1, 4>>{ rows },
      poet::dispatch_param<poet::inclusive_range<1, 4>>{ cols },
      7);
    std::printf("Kernel2D(rows=%d, cols=%d, seed=7) = %d\n", rows, cols, z);

    return 0;
}
