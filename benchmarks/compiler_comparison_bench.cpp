/// \file compiler_comparison_bench.cpp
/// \brief Cross-compiler performance comparison benchmark.
///
/// Four sections designed to expose compiler quality differences:
///   1. Dispatch baselines — raw if-else / switch / fn-ptr vs POET dispatch
///   2. Vectorization probe — float saxpy + reduce with alignment hints
///   3. N sweep for dynamic_for — cache boundary & scaling behavior
///   4. Template inlining depth — static_for at small N vs plain loop

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include <benchmark/benchmark.h>

#include <poet/poet.hpp>

namespace {

// ── Shared utilities ─────────────────────────────────────────────────────────

static inline std::uint32_t xorshift32(std::uint32_t x) noexcept {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

volatile std::uint32_t g_salt = 1;

std::uint32_t next_salt() noexcept {
    auto s = xorshift32(g_salt);
    g_salt = s;
    return s;
}

volatile int g_noise = 1;

int next_noise() noexcept {
    const int v = g_noise;
    g_noise = v + 1;
    return v;
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

template<std::size_t N> double reduce(const std::array<double, N> &a) {
    double t = 0.0;
    for (double v : a) t += v;
    return t;
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 1: Dispatch Baselines
// ═════════════════════════════════════════════════════════════════════════════

inline int dispatch_work(int val, int scale) noexcept { return val * val + scale; }

int dispatch_if_else(int val, int scale) noexcept {
    if (val == 1) return dispatch_work(1, scale);
    if (val == 2) return dispatch_work(2, scale);
    if (val == 3) return dispatch_work(3, scale);
    if (val == 4) return dispatch_work(4, scale);
    if (val == 5) return dispatch_work(5, scale);
    if (val == 6) return dispatch_work(6, scale);
    if (val == 7) return dispatch_work(7, scale);
    if (val == 8) return dispatch_work(8, scale);
    return -1;
}

int dispatch_switch(int val, int scale) noexcept {
    switch (val) {
    case 1:
        return dispatch_work(1, scale);
    case 2:
        return dispatch_work(2, scale);
    case 3:
        return dispatch_work(3, scale);
    case 4:
        return dispatch_work(4, scale);
    case 5:
        return dispatch_work(5, scale);
    case 6:
        return dispatch_work(6, scale);
    case 7:
        return dispatch_work(7, scale);
    case 8:
        return dispatch_work(8, scale);
    default:
        return -1;
    }
}

using dispatch_fn_t = int (*)(int);

template<int V> int dispatch_stub(int scale) noexcept { return dispatch_work(V, scale); }

static constexpr std::array<dispatch_fn_t, 8> dispatch_table = {
    dispatch_stub<1>,
    dispatch_stub<2>,
    dispatch_stub<3>,
    dispatch_stub<4>,
    dispatch_stub<5>,
    dispatch_stub<6>,
    dispatch_stub<7>,
    dispatch_stub<8>,
};

int dispatch_fnptr(int val, int scale) noexcept {
    const auto idx = static_cast<std::size_t>(val - 1);
    if (idx < dispatch_table.size()) return dispatch_table[idx](scale);
    return -1;
}

struct dispatch_kernel {
    template<int V> int operator()(int scale) const { return dispatch_work(V, scale); }
};

using dispatch_range = poet::make_range<1, 8>;

// ═════════════════════════════════════════════════════════════════════════════
// Section 2: Vectorization Probe
// ═════════════════════════════════════════════════════════════════════════════

constexpr std::size_t kSaxpyN = 4096;

alignas(64) float saxpy_x[kSaxpyN];
alignas(64) float saxpy_y[kSaxpyN];

void saxpy_init() {
    for (std::size_t i = 0; i < kSaxpyN; ++i) {
        saxpy_x[i] = static_cast<float>(i) * 0.001f;
        saxpy_y[i] = 0.0f;
    }
}

float saxpy_plain(float a, float b) noexcept {
    for (std::size_t i = 0; i < kSaxpyN; ++i) saxpy_y[i] = a * saxpy_x[i] + b;
    float sum = 0.0f;
    for (std::size_t i = 0; i < kSaxpyN; ++i) sum += saxpy_y[i];
    return sum;
}

#if defined(_MSC_VER)
#define BENCH_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define BENCH_RESTRICT __restrict__
#else
#define BENCH_RESTRICT
#endif

template<std::size_t Align, typename T> inline T *bench_assume_aligned(T *ptr) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return static_cast<T *>(__builtin_assume_aligned(ptr, Align));
#elif defined(_MSC_VER)
    __assume((reinterpret_cast<std::uintptr_t>(ptr) & (Align - 1)) == 0);
    return ptr;
#else
    return ptr;
#endif
}

float saxpy_aligned(float a, float b) noexcept {
    const float *BENCH_RESTRICT xp = bench_assume_aligned<64>(static_cast<const float *>(saxpy_x));
    float *BENCH_RESTRICT yp = bench_assume_aligned<64>(saxpy_y);
    for (std::size_t i = 0; i < kSaxpyN; ++i) yp[i] = a * xp[i] + b;
    float sum = 0.0f;
    for (std::size_t i = 0; i < kSaxpyN; ++i) sum += yp[i];
    return sum;
}

float saxpy_restrict(float a, float b) noexcept {
    float *BENCH_RESTRICT yp = saxpy_y;
    const float *BENCH_RESTRICT xp = saxpy_x;
    for (std::size_t i = 0; i < kSaxpyN; ++i) yp[i] = a * xp[i] + b;
    float sum = 0.0f;
    for (std::size_t i = 0; i < kSaxpyN; ++i) sum += yp[i];
    return sum;
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 3: N Sweep for dynamic_for
// ═════════════════════════════════════════════════════════════════════════════

constexpr auto bench_regs = poet::available_registers();
constexpr std::size_t tuned_accs = bench_regs.lanes_64bit * 2;

template<std::size_t NumAccs> double hand_unrolled_multi_acc(std::size_t count, std::uint32_t salt) {
    std::array<double, NumAccs> accs{};
    const std::size_t full = count - (count % NumAccs);

    std::size_t i = 0;
    for (; i < full; i += NumAccs)
        for (std::size_t lane = 0; lane < NumAccs; ++lane) accs[lane] += heavy_work(i + lane, salt);
    for (; i < count; ++i) accs[0] += heavy_work(i, salt);

    return reduce(accs);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 4: Template Inlining Depth
// ═════════════════════════════════════════════════════════════════════════════

template<std::size_t N> struct InlineAccFunctor {
    std::uint64_t &acc;
    template<auto I> void operator()() { acc += static_cast<std::uint64_t>(I) * 3 + 1; }
};

}// namespace

// ── Section 1: Dispatch Baselines ────────────────────────────────────────

static void BM_dispatch_if_else(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 1 + (next_noise() & 7);
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(dispatch_if_else(v, 2));
    }
}
BENCHMARK(BM_dispatch_if_else);

static void BM_dispatch_switch(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 1 + (next_noise() & 7);
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(dispatch_switch(v, 2));
    }
}
BENCHMARK(BM_dispatch_switch);

static void BM_dispatch_fnptr(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 1 + (next_noise() & 7);
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(dispatch_fnptr(v, 2));
    }
}
BENCHMARK(BM_dispatch_fnptr);

static void BM_dispatch_poet(benchmark::State &state) {
    for (auto _ : state) {
        auto v = 1 + (next_noise() & 7);
        benchmark::DoNotOptimize(v);
        benchmark::DoNotOptimize(
          poet::dispatch(dispatch_kernel{}, poet::DispatchParam<dispatch_range>{ v }, 2));
    }
}
BENCHMARK(BM_dispatch_poet);

// ── Section 2: Vectorization Probe ───────────────────────────────────────

static void BM_saxpy_plain(benchmark::State &state) {
    saxpy_init();
    const float a = 2.5f;
    const float b_val = 1.0f;
    for (auto _ : state) {
        benchmark::DoNotOptimize(saxpy_plain(a, b_val));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSaxpyN));
}
BENCHMARK(BM_saxpy_plain);

static void BM_saxpy_aligned(benchmark::State &state) {
    saxpy_init();
    const float a = 2.5f;
    const float b_val = 1.0f;
    for (auto _ : state) {
        benchmark::DoNotOptimize(saxpy_aligned(a, b_val));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSaxpyN));
}
BENCHMARK(BM_saxpy_aligned);

static void BM_saxpy_restrict(benchmark::State &state) {
    saxpy_init();
    const float a = 2.5f;
    const float b_val = 1.0f;
    for (auto _ : state) {
        benchmark::DoNotOptimize(saxpy_restrict(a, b_val));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSaxpyN));
}
BENCHMARK(BM_saxpy_restrict);

// ── Section 3: N Sweep ──────────────────────────────────────────────────

template<std::size_t SweepN> static void BM_sweep_1acc(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        double acc = 0.0;
        for (std::size_t i = 0; i < SweepN; ++i) acc += heavy_work(i, salt);
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(SweepN));
}

template<std::size_t SweepN> static void BM_sweep_tuned_acc(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        benchmark::DoNotOptimize(hand_unrolled_multi_acc<tuned_accs>(SweepN, salt));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(SweepN));
}

template<std::size_t SweepN> static void BM_sweep_dynamic_for(benchmark::State &state) {
    const auto salt = next_salt();
    for (auto _ : state) {
        std::array<double, tuned_accs> accs{};
        poet::dynamic_for<tuned_accs>(
          std::size_t{ 0 }, std::size_t{ SweepN }, [&accs, salt](auto lane_c, std::size_t i) {
              constexpr auto lane = decltype(lane_c)::value;
              accs[lane] += heavy_work(i, salt);
          });
        benchmark::DoNotOptimize(reduce(accs));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(SweepN));
}

BENCHMARK(BM_sweep_1acc<64>)->Name("BM_sweep_1acc_N64");
BENCHMARK(BM_sweep_tuned_acc<64>)->Name("BM_sweep_tuned_acc_N64");
BENCHMARK(BM_sweep_dynamic_for<64>)->Name("BM_sweep_dynamic_for_N64");
BENCHMARK(BM_sweep_1acc<512>)->Name("BM_sweep_1acc_N512");
BENCHMARK(BM_sweep_tuned_acc<512>)->Name("BM_sweep_tuned_acc_N512");
BENCHMARK(BM_sweep_dynamic_for<512>)->Name("BM_sweep_dynamic_for_N512");
BENCHMARK(BM_sweep_1acc<4096>)->Name("BM_sweep_1acc_N4096");
BENCHMARK(BM_sweep_tuned_acc<4096>)->Name("BM_sweep_tuned_acc_N4096");
BENCHMARK(BM_sweep_dynamic_for<4096>)->Name("BM_sweep_dynamic_for_N4096");
BENCHMARK(BM_sweep_1acc<32768>)->Name("BM_sweep_1acc_N32768");
BENCHMARK(BM_sweep_tuned_acc<32768>)->Name("BM_sweep_tuned_acc_N32768");
BENCHMARK(BM_sweep_dynamic_for<32768>)->Name("BM_sweep_dynamic_for_N32768");

// ── Section 4: Template Inlining Depth ───────────────────────────────────

template<std::size_t InlineN> static void BM_inline_plain_loop(benchmark::State &state) {
    for (auto _ : state) {
        std::uint64_t acc = 0;
        for (std::size_t i = 0; i < InlineN; ++i) acc += i * 3 + 1;
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(InlineN));
}

template<std::size_t InlineN> static void BM_inline_static_for(benchmark::State &state) {
    for (auto _ : state) {
        std::uint64_t acc = 0;
        poet::static_for<0, static_cast<std::intmax_t>(InlineN)>(InlineAccFunctor<InlineN>{ .acc = acc });
        benchmark::DoNotOptimize(acc);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(InlineN));
}

BENCHMARK(BM_inline_plain_loop<4>)->Name("BM_inline_plain_loop_N4");
BENCHMARK(BM_inline_static_for<4>)->Name("BM_inline_static_for_N4");
BENCHMARK(BM_inline_plain_loop<8>)->Name("BM_inline_plain_loop_N8");
BENCHMARK(BM_inline_static_for<8>)->Name("BM_inline_static_for_N8");
BENCHMARK(BM_inline_plain_loop<16>)->Name("BM_inline_plain_loop_N16");
BENCHMARK(BM_inline_static_for<16>)->Name("BM_inline_static_for_N16");

BENCHMARK_MAIN();
