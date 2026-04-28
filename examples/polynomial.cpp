// Example: Horner's polynomial evaluation specialized at runtime.
//
// `poet::dispatch` picks a compile-time degree N in [1, 8] from a runtime
// integer; once N is fixed, `poet::static_for` unrolls Horner's recurrence
// completely and the compiler constant-folds the coefficient lookups.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_polynomial
//   ./build/examples/example_polynomial

#include <array>
#include <cstdio>

#include <poet/poet.hpp>

// Horner's method specialized for compile-time degree N.
// Evaluates p(x) = c[0] + c[1]*x + c[2]*x^2 + ... + c[N]*x^N.
struct Horner {
    template<int N> double operator()(const double *coeffs, double x) const {
        double acc = coeffs[N];
        // Unrolled inner loop: every coefficient access is a constant offset.
        poet::static_for<0, N>([&](auto I) { acc = acc * x + coeffs[N - 1 - I]; });
        return acc;
    }
};

int main() {
    // p(x) = 1 + 2x + 3x^2 + 4x^3 + 5x^4 + 6x^5 + 7x^6 + 8x^7 + 9x^8
    constexpr std::array<double, 9> coeffs{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    // `volatile` keeps the dispatch path live so the asm pane shows the
    // jump table over the compile-time specializations.
    volatile int v_degree = 5;
    volatile double v_x = 1.5;
    int degree = v_degree;
    double x = v_x;

    double y = poet::dispatch(Horner{}, poet::dispatch_param<poet::inclusive_range<1, 8>>{ degree }, coeffs.data(), x);

    std::printf("p(%.1f) at degree %d = %.6f\n", x, degree, y);

    // Sanity: degree-3 evaluation of 1 + 2x + 3x^2 + 4x^3 at x=2.
    v_degree = 3;
    v_x = 2.0;
    double y3 = poet::dispatch(Horner{},
      poet::dispatch_param<poet::inclusive_range<1, 8>>{ static_cast<int>(v_degree) },
      coeffs.data(),
      static_cast<double>(v_x));
    std::printf("p(2.0) at degree 3 = %.6f (expected 49.0)\n", y3);

    return 0;
}
