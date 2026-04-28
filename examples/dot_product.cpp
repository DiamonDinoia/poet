// Example: lane-aware dot product breaks the serial accumulator dependency.
//
// A scalar `for` loop has one accumulator: `acc += a[i]*b[i]`. The next
// iteration can't issue its FMA until the previous `acc` retires, so the
// loop is bound by the FMA latency (~4 cycles on Zen4/Skylake).
//
// `poet::dynamic_for<L>` with a lane-aware lambda gives the compiler L
// independent accumulators, indexed by a compile-time lane id. The chains
// run in parallel and throughput becomes FMA-throughput-limited (one per
// cycle), not latency-limited.
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
