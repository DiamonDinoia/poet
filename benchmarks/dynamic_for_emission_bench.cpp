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

// ── Hand-written emission strategies ─────────────────────────────────────────

template<std::size_t Unroll, typename WorkFn>
double carried_index_multi_acc(std::size_t count, std::size_t start, WorkFn work) {
    std::array<double, Unroll> accs{};
    const std::size_t full = count - (count % Unroll);

    std::size_t i = start;
    std::size_t done = 0;
    for (; done < full; done += Unroll) {
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

template<std::size_t Unroll, typename WorkFn>
double computed_index_multi_acc(std::size_t count, std::size_t start, WorkFn work) {
    std::array<double, Unroll> accs{};
    const std::size_t full = count - (count % Unroll);

    std::size_t base = start;
    std::size_t done = 0;
    for (; done < full; done += Unroll) {
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

int main(int argc, char **argv) {
    {
        std::cerr << "\n=== dynamic_for Emission Strategy Benchmark ===\n";
        std::cerr << "ISA:              " << static_cast<unsigned>(regs.isa) << "\n";
        std::cerr << "Vector width:     " << regs.vector_width_bits << " bits\n";
        std::cerr << "Lanes (64-bit):   " << lanes_64 << "\n";
        std::cerr << "Optimal accums:   " << optimal_accs << "  (lanes_64 * 2)\n\n";
    }

    const auto salt = next_salt();

    // ════════════════════════════════════════════════════════════════════════
    // Section 1: Heavy body (accumulation)
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        reg("Heavy_body/carried-index", N, [salt] {
            return carried_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) { return heavy_work(i, salt); });
        });

        reg("Heavy_body/computed-index", N, [salt] {
            return computed_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) { return heavy_work(i, salt); });
        });

        reg("Heavy_body/dynamic_for_lane_form", N, [salt] {
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
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;

        reg("Light_body/carried-index", N, [salt] {
            return carried_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) {
                return static_cast<double>(xorshift32(static_cast<std::uint32_t>(i) ^ salt));
            });
        });

        reg("Light_body/computed-index", N, [salt] {
            return computed_index_multi_acc<optimal_accs>(N, 0, [salt](std::size_t i) {
                return static_cast<double>(xorshift32(static_cast<std::uint32_t>(i) ^ salt));
            });
        });

        reg("Light_body/dynamic_for_lane_form", N, [salt] {
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
    // ════════════════════════════════════════════════════════════════════════
    {
        constexpr std::size_t N = 10000;
        constexpr std::size_t effective_iters = 5000;

        reg("Stride/dynamic_for_CT_stride_2", effective_iters, [salt] {
            std::array<double, optimal_accs> accs{};
            poet::dynamic_for<optimal_accs, 2>(
              std::size_t{ 0 }, std::size_t{ N }, [&accs, salt](auto lane_c, std::size_t i) {
                  constexpr auto lane = decltype(lane_c)::value;
                  accs[lane] += heavy_work(i, salt);
              });
            return reduce(accs);
        });

        reg("Stride/dynamic_for_RT_stride_2", effective_iters, [salt] {
            std::array<double, optimal_accs> accs{};
            poet::dynamic_for<optimal_accs>(
              std::size_t{ 0 }, std::size_t{ N }, std::size_t{ 2 }, [&accs, salt](auto lane_c, std::size_t i) {
                  constexpr auto lane = decltype(lane_c)::value;
                  accs[lane] += heavy_work(i, salt);
              });
            return reduce(accs);
        });
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
