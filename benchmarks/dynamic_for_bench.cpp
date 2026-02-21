#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <nanobench.h>

#include <poet/core/dynamic_for.hpp>
#include <poet/core/static_for.hpp>

using namespace std::chrono_literals;

namespace {

constexpr std::size_t small_count = 1000;
constexpr std::size_t large_count = 5000;
constexpr std::size_t irregular_begin = 3;
constexpr std::size_t irregular_end = 515;
constexpr std::size_t default_unroll = 8;
constexpr std::uint64_t salt_increment = 0x9e3779b97f4a7c15ULL;
volatile std::uint64_t benchmark_salt = 1;

// xorshift32: simple PRNG that both GCC and Clang vectorize well.
// Used in game engines, simulation, and production systems.
// Chosen for fair comparison: both compilers vectorize equally,
// showing POET's value without compiler-specific optimization differences.
static inline std::uint32_t xorshift32(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// Lightweight workload: just the hash, no FMA chain.
// Used for loop-overhead benchmarks where the body is trivial.
static inline double fast_work(std::size_t i) noexcept {
    const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(i));
    return static_cast<double>(static_cast<std::int32_t>(r)) * (1.0 / (1U << 31));
}

// Heavy workload: 5-deep multiply-add chain.
// Latency ~20 cycles per call; useful for ILP / accumulator benchmarks.
// Written as x*a+b (not std::fma) so the compiler emits hardware FMA when
// available (-mfma/-march=native) without falling back to a software libm call.
static inline double compute_work(std::size_t i) noexcept {
    const std::uint32_t r = xorshift32(static_cast<std::uint32_t>(i));
    double x = static_cast<double>(static_cast<std::int32_t>(r)) * (1.0 / (1U << 31));
    x = x * 1.0000001192092896 + 0.3333333333333333;
    x = x * 0.9999998807907104 + 0.14285714285714285;
    x = x * 1.0000000596046448 + -0.0625;
    x = x * 1.0000001192092896 + 0.25;
    x = x * 0.9999998807907104 + -0.125;
    return x;
}

// ── Trivial-body variants (for loop-overhead section) ─────────────────────

// Baseline: plain loop with trivial per-element work.
double serial_loop_fast(std::size_t begin, std::size_t end, std::size_t salt) {
    double sum = 0.0;
    for (std::size_t i = begin; i < end; ++i) sum += fast_work(i + salt);
    return sum;
}

// dynamic_for with trivial body: register pressure is low enough that
// the by-reference `sum` stays in a register, so unrolling reduces loop overhead.
template<std::size_t Unroll> double dynamic_for_loop_fast(std::size_t begin, std::size_t end, std::size_t salt) {
    double sum = 0.0;
    poet::dynamic_for<Unroll>(begin, end, [&sum, salt](std::size_t i) { sum += fast_work(i + salt); });
    return sum;
}

// ── Heavy-body variants (for accumulator-ILP section) ─────────────────────

// Naive reduction: captures `sum` by reference.
// With compute_work (5 FMA constants + 8 lane accumulators), register
// pressure forces `sum` to be spilled to the stack and reloaded every
// Unroll iterations via store-to-load forwarding.  The unrolling adds
// overhead instead of helping.
double serial_loop(std::size_t begin, std::size_t end, std::size_t salt) {
    double sum = 0.0;
    for (std::size_t i = begin; i < end; ++i) sum += compute_work(i + salt);
    return sum;
}

template<std::size_t Unroll> double dynamic_for_loop(std::size_t begin, std::size_t end, std::size_t salt) {
    double sum = 0.0;
    poet::dynamic_for<Unroll>(begin, end, [&sum, salt](std::size_t i) { sum += compute_work(i + salt); });
    return sum;
}

// Serial multi-accumulator: separate sums per lane index.
// Each `sums[i % NumAccs]` slot is independent, giving the compiler
// room to pipeline the FMA chains.
template<std::size_t NumAccs> double serial_loop_multi_acc(std::size_t begin, std::size_t end, std::size_t salt) {
    std::array<double, NumAccs> sums{};
    for (std::size_t i = begin; i < end; ++i) sums[i % NumAccs] += compute_work(i + salt);
    double total = 0.0;
    for (double v : sums) total += v;
    return total;
}

// dynamic_for lane callback with independent per-lane accumulators.
// The `auto lane_c` callback provides a compile-time lane index so each
// `sums[lane]` maps to a distinct register, eliminating the spill that
// plagues the single-accumulator version.
template<std::size_t Unroll, std::size_t NumAccs>
double dynamic_for_loop_multi_acc(std::size_t begin, std::size_t end, std::size_t salt) {
    std::array<double, NumAccs> sums{};
    poet::dynamic_for<Unroll>(begin, end, [&sums, salt](auto lane_c, std::size_t i) {
        constexpr std::size_t lane = decltype(lane_c)::value;
        constexpr std::size_t acc_idx = lane % NumAccs;
        sums[acc_idx] += compute_work(i + salt);
    });
    double total = 0.0;
    for (double v : sums) total += v;
    return total;
}

// Combined dynamic_for + static_for:
// - dynamic_for<Unroll> with per-lane accumulation handles the runtime loop,
//   keeping Unroll independent FMA chains alive simultaneously.
// - static_for<0, NumAccs> handles the final reduction at compile time —
//   fully unrolled, zero branch overhead, guaranteed register-to-register adds.
//
// NumAccs controls the parallelism / register pressure trade-off.
// With compute_work (5 FMA constants), the XMM budget is:
//   NumAccs accumulators + ~7 FMA/scale constants + scratch = NumAccs + ~9
// On x86-64 (16 XMM regs): NumAccs <= 7.  NumAccs=4 is the empirical sweet spot
// that keeps all accumulators and constants in registers (zero spills).
template<std::size_t NumAccs> double dynamic_for_static_reduce(std::size_t begin, std::size_t end, std::size_t salt) {
    std::array<double, NumAccs> accs{};
    poet::dynamic_for<NumAccs>(begin, end, [&accs, salt](auto lane_c, std::size_t i) {
        accs[decltype(lane_c)::value] += compute_work(i + salt);
    });
    double sum = 0.0;
    poet::static_for<0, static_cast<std::intmax_t>(NumAccs)>([&](auto ic) { sum += accs[ic.value]; });
    return sum;
}

inline std::uint64_t next_salt() noexcept {
    const auto salt = benchmark_salt;
    benchmark_salt += salt_increment;
    return salt;
}

template<typename ComputeFn>
void run_benchmark_case(ankerl::nanobench::Bench &bench,
  std::size_t begin,
  std::size_t end,
  const std::string &label,
  ComputeFn &&compute) {
    auto compute_fn = std::forward<ComputeFn>(compute);
    bench.batch(end - begin).run(label, [begin, end, compute_fn = std::move(compute_fn)]() mutable {
        auto b = begin;
        auto e = end;
        const auto salt = next_salt();
        ankerl::nanobench::doNotOptimizeAway(b);
        ankerl::nanobench::doNotOptimizeAway(e);
        ankerl::nanobench::doNotOptimizeAway(salt);
        const auto result = compute_fn(b, e, salt);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

// ── Loop overhead amortization ────────────────────────────────────────────
// Body = fast_work (splitmix64 only).  Per-element compute is cheap so
// branch/compare/increment overhead is a meaningful fraction of total cost.
// dynamic_for<Unroll> reduces that overhead to 1/(Unroll) per element.
void benchmark_loop_overhead(ankerl::nanobench::Bench &bench,
  std::size_t begin,
  std::size_t end,
  const std::string &label) {
    run_benchmark_case(
      bench, begin, end, "for loop " + label, [](auto b, auto e, auto salt) { return serial_loop_fast(b, e, salt); });
    run_benchmark_case(bench, begin, end, "dynamic_for<8> " + label, [](auto b, auto e, auto salt) {
        return dynamic_for_loop_fast<8>(b, e, salt);
    });
    run_benchmark_case(bench, begin, end, "dynamic_for<4> " + label, [](auto b, auto e, auto salt) {
        return dynamic_for_loop_fast<4>(b, e, salt);
    });
}

// ── Accumulator / ILP comparison ─────────────────────────────────────────
// Body = compute_work (5-deep FMA chain, ~20 cy latency).
// Shows three patterns in order of ascending performance:
//   1. Naive single-acc   — &sum forced to stack (memory spill)
//   2. Serial multi-acc   — N independent slots, compiler may pipeline
//   3. dynamic_for+static_for — Unroll live FMA chains + compile-time reduction
void benchmark_range(ankerl::nanobench::Bench &bench, std::size_t begin, std::size_t end, const std::string &label) {
    // ── 1. Naive single accumulator (reference-capture causes memory spill) ─
    run_benchmark_case(
      bench, begin, end, "for loop " + label, [](auto b, auto e, auto salt) { return serial_loop(b, e, salt); });
    run_benchmark_case(bench, begin, end, "dynamic_for naive " + label, [](auto b, auto e, auto salt) {
        return dynamic_for_loop<default_unroll>(b, e, salt);
    });

    // ── 2. Serial multi-accumulator (compiler-visible independence, no unroll) ─
    run_benchmark_case(bench, begin, end, "for loop multi-acc4 " + label, [](auto b, auto e, auto salt) {
        return serial_loop_multi_acc<4>(b, e, salt);
    });

    // ── 3. dynamic_for lane acc + static_for reduce (full ILP, guaranteed unroll) ─
    // NumAccs=4: fits all accumulators + FMA constants in 16 XMM registers (zero spills).
    // NumAccs=8: exceeds register budget with compute_work, causes spills — shown for contrast.
    run_benchmark_case(bench, begin, end, "dynamic_for+static_reduce<4> " + label, [](auto b, auto e, auto salt) {
        return dynamic_for_static_reduce<4>(b, e, salt);
    });
    run_benchmark_case(bench, begin, end, "dynamic_for+static_reduce<8> " + label, [](auto b, auto e, auto salt) {
        return dynamic_for_static_reduce<8>(b, e, salt);
    });
    run_benchmark_case(bench, begin, end, "dynamic_for lane multi-acc4 " + label, [](auto b, auto e, auto salt) {
        return dynamic_for_loop_multi_acc<default_unroll, 4>(b, e, salt);
    });
}

}// namespace

// Runtime stride: step passed as argument
template<std::size_t Unroll, typename T> double dynamic_for_runtime_stride(T begin, T end, T step, std::size_t salt) {
    double sum = 0.0;
    poet::dynamic_for<Unroll>(
      begin, end, step, [&sum, salt](auto i) { sum += compute_work(static_cast<std::size_t>(i) + salt); });
    return sum;
}

// Compile-time stride: step as template parameter
template<std::size_t Unroll, std::intmax_t Step, typename T>
double dynamic_for_ct_stride(T begin, T end, std::size_t salt) {
    double sum = 0.0;
    poet::dynamic_for<Unroll, Step>(
      begin, end, [&sum, salt](auto i) { sum += compute_work(static_cast<std::size_t>(i) + salt); });
    return sum;
}

void benchmark_stride_comparison(ankerl::nanobench::Bench &bench) {
    constexpr std::size_t N = 2000;

    // Step=1: runtime vs compile-time
    {
        std::size_t iters = N;
        bench.batch(iters).run("stride=1 runtime", [&]() {
            auto b = std::size_t{ 0 };
            auto e = N;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_runtime_stride<8>(b, e, std::size_t{ 1 }, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
        bench.batch(iters).run("stride=1 compile-time", [&]() {
            auto b = std::size_t{ 0 };
            auto e = N;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_ct_stride<8, 1>(b, e, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Step=2: runtime vs compile-time
    {
        std::size_t iters = N / 2;
        bench.batch(iters).run("stride=2 runtime", [&]() {
            auto b = std::size_t{ 0 };
            auto e = N;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_runtime_stride<8>(b, e, std::size_t{ 2 }, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
        bench.batch(iters).run("stride=2 compile-time", [&]() {
            auto b = std::size_t{ 0 };
            auto e = N;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_ct_stride<8, 2>(b, e, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Step=3: runtime vs compile-time
    {
        std::size_t iters = (N + 2) / 3;
        bench.batch(iters).run("stride=3 runtime", [&]() {
            auto b = std::size_t{ 0 };
            auto e = N;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_runtime_stride<8>(b, e, std::size_t{ 3 }, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
        bench.batch(iters).run("stride=3 compile-time", [&]() {
            auto b = std::size_t{ 0 };
            auto e = N;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_ct_stride<8, 3>(b, e, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // Small range (tail-dominated): Step=2
    {
        constexpr std::size_t small_n = 7;// < Unroll=8, all tail
        std::size_t iters = (small_n + 1) / 2;
        bench.batch(iters).run("stride=2 small(7) runtime", [&]() {
            auto b = std::size_t{ 0 };
            auto e = small_n;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_runtime_stride<8>(b, e, std::size_t{ 2 }, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
        bench.batch(iters).run("stride=2 small(7) compile-time", [&]() {
            auto b = std::size_t{ 0 };
            auto e = small_n;
            const auto salt = next_salt();
            ankerl::nanobench::doNotOptimizeAway(b);
            ankerl::nanobench::doNotOptimizeAway(e);
            const auto result = dynamic_for_ct_stride<8, 2>(b, e, salt);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }
}

int main() {
    ankerl::nanobench::Bench bench;
    bench.minEpochTime(10ms);

    bench.title("dynamic_for: loop overhead amortization (trivial body)");
    benchmark_loop_overhead(bench, 0, small_count, std::string("small ") + std::to_string(small_count));
    benchmark_loop_overhead(bench, 0, large_count, std::string("large ") + std::to_string(large_count));

    bench.title("dynamic_for: accumulator patterns (heavy FMA-chain body)");
    benchmark_range(bench, 0, small_count, std::string("small ") + std::to_string(small_count));
    benchmark_range(bench, 0, large_count, std::string("large ") + std::to_string(large_count));
    benchmark_range(bench,
      irregular_begin,
      irregular_end,
      std::string("irregular (") + std::to_string(irregular_begin) + "-" + std::to_string(irregular_end) + ")");

    bench.title("dynamic_for: runtime stride vs compile-time stride");
    benchmark_stride_comparison(bench);

    return 0;
}
