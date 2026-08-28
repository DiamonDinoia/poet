// Example: lane-aware dot product breaks the serial accumulator dependency.
//
// A scalar `for` loop accumulates into one register, so every FMA waits on
// the previous one. `poet::dynamic_for<L>` with a lane-aware lambda gives the
// compiler L independent accumulators on parallel dependency chains.
//
// Build:
//   cmake -S . -B build -DPOET_BUILD_EXAMPLES=ON
//   cmake --build build --target example_dot_product
//   ./build/examples/example_dot_product

#include <array>
#include <cstddef>
#include <cstdio>
#include <random>
#include <vector>

#include <poet/poet.hpp>

namespace {

// Fixed-seed RNG so output stays reproducible.
std::vector<double> make_data(std::size_t n, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> v(n);
    for (auto &x : v) x = dist(rng);
    return v;
}

double dot_scalar(const double *a, const double *b, std::size_t n) {
    double acc = 0.0;
    for (std::size_t i = 0; i < n; ++i) acc += a[i] * b[i];
    return acc;
}

template<std::size_t L> double dot_lane_aware(const double *a, const double *b, std::size_t n) {
    std::array<double, L> lanes{};
    poet::dynamic_for<L>(std::size_t{ 0 }, n, [&](auto lane, std::size_t i) { lanes[lane] += a[i] * b[i]; });
    double acc = 0.0;
    for (double v : lanes) acc += v;
    return acc;
}

}// namespace

int main() {
    // `volatile n` keeps the loop bound runtime so the compiler emits an
    // actual loop, not a constant.
    volatile std::size_t v_n = 1024;
    const std::size_t n = v_n;

    auto a = make_data(n, 1);
    auto b = make_data(n, 2);

    double s = dot_scalar(a.data(), b.data(), n);
    double l4 = dot_lane_aware<4>(a.data(), b.data(), n);
    double l8 = dot_lane_aware<8>(a.data(), b.data(), n);

    std::printf("n=%zu\n", n);
    std::printf("  scalar         = %.10f\n", s);
    std::printf("  lane-aware<4>  = %.10f  (delta %.2e)\n", l4, l4 - s);
    std::printf("  lane-aware<8>  = %.10f  (delta %.2e)\n", l8, l8 - s);

    return 0;
}
