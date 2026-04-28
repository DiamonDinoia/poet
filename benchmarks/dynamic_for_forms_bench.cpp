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
#include <string>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>

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

template<typename Fn> void reg(const char *name, std::uint64_t batch, Fn &&fn) {
    benchmark::RegisterBenchmark(name, [fn = std::forward<Fn>(fn), batch](benchmark::State &state) mutable {
        for (auto _ : state) benchmark::DoNotOptimize(fn());
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(batch));
    })->MinTime(0.1);
}

template<std::size_t N> double reduce(const std::array<double, N> &a) {
    double t = 0.0;
    for (double v : a) t += v;
    return t;
}

}// namespace

int main(int argc, char **argv) {
    {
        std::cerr << "\n=== dynamic_for Forms Benchmark ===\n";
        std::cerr << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cerr << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cerr << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cerr << "Optimal accums:   " << optimal_accs << "  (lanes_64 * 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Section 1: Accumulation (serial dependency)
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        reg("Accumulation/plain_for_1_acc", N, [salt] {
            double acc = 0.0;
            for (std::size_t i = 0; i < N; ++i) acc += heavy_work(i, salt);
            return acc;
        });

        reg("Accumulation/dynamic_for_index_only_1_acc", N, [salt] {
            double acc = 0.0;
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, [&acc, salt](std::size_t i) { acc += heavy_work(i, salt); });
            return acc;
        });

        reg("Accumulation/dynamic_for_lane_form_optimal_accs", N, [salt] {
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
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;
        static std::array<double, N> out{};

        reg("Elementwise/plain_for", N, [salt] {
            for (std::size_t i = 0; i < N; ++i) out[i] = heavy_work(i, salt);
            return out[N / 2];
        });

        reg("Elementwise/dynamic_for_index_only", N, [salt] {
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, [salt](std::size_t i) { out[i] = heavy_work(i, salt); });
            return out[N / 2];
        });

        reg("Elementwise/dynamic_for_lane_form_unused_lane", N, [salt] {
            poet::dynamic_for<optimal_accs>(std::size_t{ 0 }, std::size_t{ N }, [salt](auto /*lane_c*/, std::size_t i) {
                out[i] = heavy_work(i, salt);
            });
            return out[N / 2];
        });
    }

    // ════════════════════════════════════════════════════════════════════════
    // Section 3: Small N tail overhead
    // ════════════════════════════════════════════════════════════════════════
    {
        auto run_small_n = [](std::size_t n, std::uint32_t s) {
            const std::string suffix = "_N=" + std::to_string(n);

            benchmark::RegisterBenchmark(("SmallN/plain_for" + suffix).c_str(), [n, s](benchmark::State &state) {
                for (auto _ : state) {
                    std::uint32_t acc = s;
                    for (std::size_t i = 0; i < n; ++i) acc = xorshift32(acc + static_cast<std::uint32_t>(i));
                    benchmark::DoNotOptimize(acc);
                }
                state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
            })->MinTime(0.1);

            benchmark::RegisterBenchmark(("SmallN/dynamic_for_index_only" + suffix).c_str(),
              [n, s](benchmark::State &state) {
                  for (auto _ : state) {
                      std::uint32_t acc = s;
                      poet::dynamic_for<8>(std::size_t{ 0 }, n, [&acc](std::size_t i) {
                          acc = xorshift32(acc + static_cast<std::uint32_t>(i));
                      });
                      benchmark::DoNotOptimize(acc);
                  }
                  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
              })
              ->MinTime(0.1);

            benchmark::RegisterBenchmark(("SmallN/dynamic_for_lane_form" + suffix).c_str(),
              [n, s](benchmark::State &state) {
                  for (auto _ : state) {
                      std::uint32_t acc = s;
                      poet::dynamic_for<8>(std::size_t{ 0 }, n, [&acc](auto /*lane_c*/, std::size_t i) {
                          acc = xorshift32(acc + static_cast<std::uint32_t>(i));
                      });
                      benchmark::DoNotOptimize(acc);
                  }
                  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
              })
              ->MinTime(0.1);
        };

        run_small_n(3, salt);
        run_small_n(7, salt);
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
