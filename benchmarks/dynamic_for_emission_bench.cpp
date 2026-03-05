/// \file dynamic_for_emission_bench.cpp
/// \brief Validates carried-index vs computed-index emission strategy.
///
/// Three sections:
///   1. Heavy body (accumulation) — body dominates, both strategies similar
///   2. Light body (index-visible overhead) — where SLP behavior matters
///   3. CT stride vs RT stride — compile-time stride template parameter benefit

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <nanobench.h>

#include <poet/poet.hpp>

using namespace std::chrono_literals;

namespace {

// ── Register-aware tuning ────────────────────────────────────────────────────

constexpr auto regs = poet::available_registers();
constexpr std::size_t lanes_64 = regs.lanes_64bit;
constexpr std::size_t optimal_accs = lanes_64 * 2;

// ── Workload ─────────────────────────────────────────────────────────────────

static inline std::uint32_t xorshift32(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static inline double heavy_work(std::size_t i, std::uint32_t salt) noexcept {
    double x = static_cast<double>(static_cast<std::int32_t>(xorshift32(static_cast<std::uint32_t>(i) ^ salt)));
    x = x * 1.0000001192092896 + 0.3333333333333333;
    x = x * 0.9999998807907104 + 0.14285714285714285;
    x = x * 1.0000000596046448 + -0.0625;
    x = x * 1.0000001192092896 + 0.25;
    x = x * 0.9999998807907104 + -0.125;
    return x;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

volatile std::uint32_t g_salt = 1;

std::uint32_t next_salt() noexcept {
    auto s = xorshift32(g_salt);
    g_salt = s;
    return s;
}

template<typename Fn> void run(ankerl::nanobench::Bench &b, std::uint64_t batch, const char *name, Fn &&fn) {
    b.batch(batch).run(name, [fn = std::forward<Fn>(fn)]() mutable { ankerl::nanobench::doNotOptimizeAway(fn()); });
}

template<std::size_t N> double reduce(const std::array<double, N> &a) {
    double t = 0.0;
    for (double v : a) t += v;
    return t;
}

// ── Hand-written emission strategies ─────────────────────────────────────────
//
// These inline both strategies directly — no library changes needed.

/// Carried-index: idx starts at base, incremented by stride each lane.
/// This is what dynamic_for uses internally.
template<std::size_t Unroll, typename WorkFn>
double carried_index_multi_acc(std::size_t count, std::size_t start, WorkFn work) {
    std::array<double, Unroll> accs{};
    const std::size_t full = count - (count % Unroll);

    std::size_t i = start;
    std::size_t done = 0;
    for (; done < full; done += Unroll) {
        // Carried: each lane depends on the previous lane's index
        std::size_t idx = i;
        for (std::size_t lane = 0; lane < Unroll; ++lane) {
            accs[lane] += work(idx);
            idx += 1;
        }
        i = idx;
    }
    for (; done < count; ++done) {
        accs[0] += work(i);
        i += 1;
    }
    return reduce(accs);
}

/// Computed-index: idx = base + lane, computed independently per lane.
/// GCC's SLP vectorizer may try to pack the lane indices into a vector,
/// potentially causing register spills on light bodies.
template<std::size_t Unroll, typename WorkFn>
double computed_index_multi_acc(std::size_t count, std::size_t start, WorkFn work) {
    std::array<double, Unroll> accs{};
    const std::size_t full = count - (count % Unroll);

    std::size_t base = start;
    std::size_t done = 0;
    for (; done < full; done += Unroll) {
        // Computed: each lane's index is independently calculated
        for (std::size_t lane = 0; lane < Unroll; ++lane) { accs[lane] += work(base + lane); }
        base += Unroll;
    }
    for (; done < count; ++done) {
        accs[0] += work(base);
        base += 1;
    }
    return reduce(accs);
}

}// namespace

int main() {
    {
        std::cout << "\n=== dynamic_for Emission Strategy Benchmark ===\n";
        std::cout << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cout << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cout << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cout << "Optimal accums:   " << optimal_accs << "  (lanes_64 * 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Section 1: Heavy body (accumulation)
    //
    // Body dominates index computation cost. All three variants should
    // perform similarly.
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        ankerl::nanobench::Bench b;
        b.minEpochTime(100ms).relative(true);
        b.title("Heavy body emission: carried vs computed vs dynamic_for (N=10000)");

        run(b, N, "carried-index (hand-written)", [salt] {
            return carried_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) { return heavy_work(i, salt); });
        });

        run(b, N, "computed-index (hand-written)", [salt] {
            return computed_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) { return heavy_work(i, salt); });
        });

        run(b, N, "dynamic_for lane form", [salt] {
            std::array<double, optimal_accs> accs{};
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, [&accs, salt](auto lane_c, std::size_t i) {
                  constexpr auto lane = decltype(lane_c)::value;
                  accs[lane] += heavy_work(i, salt);
              });
            return reduce(accs);
        });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Section 2: Light body (index-visible overhead)
    //
    // xorshift32 body is cheap enough that index computation strategy
    // matters. Computed-index may cause GCC SLP spills.
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        ankerl::nanobench::Bench b;
        b.minEpochTime(100ms).relative(true);
        b.title("Light body emission: carried vs computed vs dynamic_for (N=10000)");

        run(b, N, "carried-index (hand-written)", [salt] {
            return carried_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) {
                return static_cast<double>(xorshift32(static_cast<std::uint32_t>(i) ^ salt));
            });
        });

        run(b, N, "computed-index (hand-written)", [salt] {
            return computed_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) {
                return static_cast<double>(xorshift32(static_cast<std::uint32_t>(i) ^ salt));
            });
        });

        run(b, N, "dynamic_for lane form", [salt] {
            std::array<double, optimal_accs> accs{};
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, [&accs, salt](auto lane_c, std::size_t i) {
                  constexpr auto lane = decltype(lane_c)::value;
                  accs[lane] += static_cast<double>(xorshift32(static_cast<std::uint32_t>(i) ^ salt));
              });
            return reduce(accs);
        });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Section 3: CT stride vs RT stride
    //
    // dynamic_for<Unroll, 2> (CT stride) vs dynamic_for<Unroll>(b, e, 2, f)
    // (RT stride). Both use lane form with accumulation.
    // N = 5000 effective iterations (range [0, 10000) stride 2).
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;
        constexpr std::size_t effective_iters = 5000;

        ankerl::nanobench::Bench b;
        b.minEpochTime(100ms).relative(true);
        b.title("CT stride vs RT stride: stride=2 lane form (effective N=5000)");

        run(b, effective_iters, "dynamic_for CT stride=2", [salt] {
            std::array<double, optimal_accs> accs{};
            poet::dynamic_for<optimal_accs, 2>(
              std::size_t{ 0 }, std::size_t{ N }, [&accs, salt](auto lane_c, std::size_t i) {
                  constexpr auto lane = decltype(lane_c)::value;
                  accs[lane] += heavy_work(i, salt);
              });
            return reduce(accs);
        });

        run(b, effective_iters, "dynamic_for RT stride=2", [salt] {
            std::array<double, optimal_accs> accs{};
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, std::size_t{ 2 }, [&accs, salt](auto lane_c, std::size_t i) {
                  constexpr auto lane = decltype(lane_c)::value;
                  accs[lane] += heavy_work(i, salt);
              });
            return reduce(accs);
        });
    }
}
