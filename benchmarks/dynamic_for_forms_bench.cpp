/// \file dynamic_for_forms_bench.cpp
/// \brief Compares dynamic_for callable forms (lane vs index-only) across workloads.
///
/// Three sections:
///   1. Accumulation (serial dependency) — lane form breaks dep chains, should win
///   2. Element-wise independent work — body dominates, all roughly equal
///   3. Small N tail overhead — dispatch cost for tiny ranges

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

}// namespace

int main() {
    {
        std::cout << "\n=== dynamic_for Forms Benchmark ===\n";
        std::cout << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cout << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cout << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cout << "Optimal accums:   " << optimal_accs << "  (lanes_64 * 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Section 1: Accumulation (serial dependency)
    //
    // Lane form with per-lane accumulators breaks the serial dependency
    // chain, enabling ILP. Index-only and plain for are serial.
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        ankerl::nanobench::Bench b;
        b.minEpochTime(100ms).relative(true);
        b.title("Accumulation: plain for vs index-only vs lane form (N=10000)");

        run(b, N, "plain for (1 acc)", [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        run(b, N, "dynamic_for index-only (1 acc)", [salt] {
            double acc = 0.0;
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, [&acc, salt](std::size_t i) { acc += heavy_work(i, salt); });
            return acc;
        });

        run(b, N, "dynamic_for lane form (optimal accs)", [salt] {
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
    // Section 2: Element-wise independent work
    //
    // Each iteration is independent (out[i] = heavy_work(i)). The body
    // dominates, so all three forms should be roughly equal.
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;
        static std::array<double, N> out{};

        ankerl::nanobench::Bench b;
        b.minEpochTime(100ms).relative(true);
        b.title("Element-wise: plain for vs index-only vs lane form (N=10000)");

        run(b, N, "plain for", [salt] {
            for (std::size_t i = 0; i < N; ++i) out[i] = heavy_work(i, salt);
            return out[N / 2];
        });

        run(b, N, "dynamic_for index-only", [salt] {
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, [salt](std::size_t i) { out[i] = heavy_work(i, salt); });
            return out[N / 2];
        });

        run(b, N, "dynamic_for lane form (unused lane)", [salt] {
            poet::dynamic_for<optimal_accs>(std::size_t{ 0 }, std::size_t{ N }, [salt](auto /*lane_c*/, std::size_t i) {
                out[i] = heavy_work(i, salt);
            });
            return out[N / 2];
        });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Section 3: Small N tail overhead
    //
    // N below Unroll=8. Light body (xorshift32) to expose dispatch overhead.
    // ════════════════════════════════════════════════════════════════════════
    {
        auto run_small_n = [](std::size_t n, std::uint32_t s) {
            const std::string suffix = " (N=" + std::to_string(n) + ")";

            ankerl::nanobench::Bench b;
            b.minEpochTime(100ms).relative(true);
            b.title("Small N overhead" + suffix);

            b.batch(n).run(("plain for" + suffix).c_str(), [n, s] {
                std::uint32_t acc = s;
                for (std::size_t i = 0; i < n; ++i) acc = xorshift32(acc + static_cast<std::uint32_t>(i));
                ankerl::nanobench::doNotOptimizeAway(acc);
            });

            b.batch(n).run(("dynamic_for index-only" + suffix).c_str(), [n, s] {
                std::uint32_t acc = s;
                poet::dynamic_for<8>(std::size_t{ 0 }, n, [&acc](std::size_t i) {
                    acc = xorshift32(acc + static_cast<std::uint32_t>(i));
                });
                ankerl::nanobench::doNotOptimizeAway(acc);
            });

            b.batch(n).run(("dynamic_for lane form" + suffix).c_str(), [n, s] {
                std::uint32_t acc = s;
                poet::dynamic_for<8>(std::size_t{ 0 }, n, [&acc](auto /*lane_c*/, std::size_t i) {
                    acc = xorshift32(acc + static_cast<std::uint32_t>(i));
                });
                ankerl::nanobench::doNotOptimizeAway(acc);
            });
        };

        run_small_n(3, salt);
        run_small_n(7, salt);
    }
}
