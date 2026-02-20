#include <array>
#include <chrono>
#include <tuple>
#include <utility>

#include <nanobench.h>

#include <poet/core/static_dispatch.hpp>

using namespace std::chrono_literals;
namespace {

// Simple kernel for 1D/2D/nD dispatch
struct simple_kernel {
    template<int... Vs> int operator()(int scale) const {
        int s = 0;
        ((s += Vs), ...);
        return scale * s;
    }
};

// Dispatch counts per benchmark run
constexpr int dispatches_1d = 4;
constexpr int dispatches_2d = 4;
constexpr int dispatches_5d = 100;

volatile int runtime_noise = 0;

// 1D ranges
using range_1d_contig = poet::make_range<1, 8>;// 1..8
using range_1d_noncontig = std::integer_sequence<int, 1, 10, 20, 30, 40, 50, 60, 70>;// same size, sparse

// 2D ranges
using range_2d_contig = poet::make_range<1, 8>;
using range_2d_noncontig = std::integer_sequence<int, 1, 10, 20, 30, 40, 50, 60, 70>;

// 5D ranges (4 options each = table size 1024)
using range_5d_contig = poet::make_range<0, 3>;// 0..3
using range_5d_noncontig = std::integer_sequence<int, 0, 10, 20, 30>;

template<typename Fn> void run_case(ankerl::nanobench::Bench &bench, std::uint64_t batch, const char *label, Fn &&fn) {
    auto fn_local = std::forward<Fn>(fn);
    bench.batch(batch).run(label, [fn_local = std::move(fn_local)]() mutable {
        const auto total = fn_local();
        ankerl::nanobench::doNotOptimizeAway(total);
    });
}

template<typename Range, std::size_t N> int dispatch_1d_batch(const std::array<int, N> &values, int scale) {
    int total = 0;
    for (int value : values) {
        auto v = value;
        ankerl::nanobench::doNotOptimizeAway(v);
        auto param = std::make_tuple(poet::DispatchParam<Range>{ v });
        total += poet::dispatch(simple_kernel{}, param, scale);
    }
    return total;
}

template<typename Range, std::size_t N>
int dispatch_2d_batch(const std::array<std::pair<int, int>, N> &values, int scale) {
    int total = 0;
    for (auto [w, h] : values) {
        auto vw = w;
        auto vh = h;
        ankerl::nanobench::doNotOptimizeAway(vw);
        ankerl::nanobench::doNotOptimizeAway(vh);
        auto params = std::make_tuple(poet::DispatchParam<Range>{ vw }, poet::DispatchParam<Range>{ vh });
        total += poet::dispatch(simple_kernel{}, params, scale);
    }
    return total;
}

template<typename Range> int dispatch_5d_repeated(const std::array<int, 5> &values, int repetitions, int scale) {
    int total = 0;
    for (int rep = 0; rep < repetitions; ++rep) {
        auto params = std::make_tuple(poet::DispatchParam<Range>{ values[0] },
          poet::DispatchParam<Range>{ values[1] },
          poet::DispatchParam<Range>{ values[2] },
          poet::DispatchParam<Range>{ values[3] },
          poet::DispatchParam<Range>{ values[4] });
        total += poet::dispatch(simple_kernel{}, params, scale);
    }
    return total;
}

int next_noise() {
    const int v = runtime_noise;
    runtime_noise = v + 1;
    return v;
}

auto make_contig_hit_values(int noise) -> std::array<int, 5> {
    return { (noise + 0) & 3, (noise + 1) & 3, (noise + 2) & 3, (noise + 3) & 3, (noise + 4) & 3 };
}

auto make_noncontig_hit_values(int noise) -> std::array<int, 5> {
    constexpr std::array<int, 4> sparse{ 0, 10, 20, 30 };
    return { sparse[(noise + 0) & 3],
        sparse[(noise + 1) & 3],
        sparse[(noise + 2) & 3],
        sparse[(noise + 3) & 3],
        sparse[(noise + 4) & 3] };
}

}// namespace

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("dispatch: dimensionality, hit/miss, contiguous/sparse");
    bench.minEpochTime(10ms);

    constexpr std::array<int, dispatches_1d> one_d_contig_hits{ 1, 3, 5, 7 };
    constexpr std::array<int, dispatches_1d> one_d_contig_misses{ 0, 9, 15, 100 };
    constexpr std::array<int, dispatches_1d> one_d_sparse_hits{ 1, 20, 50, 70 };
    constexpr std::array<int, dispatches_1d> one_d_sparse_misses{ 0, 5, 15, 100 };

    constexpr std::array<std::pair<int, int>, dispatches_2d> two_d_contig_hits{
        { { 2, 3 }, { 4, 5 }, { 6, 7 }, { 8, 8 } }
    };
    constexpr std::array<std::pair<int, int>, dispatches_2d> two_d_contig_misses{
        { { 0, 0 }, { 9, 1 }, { 1, 9 }, { 12, 12 } }
    };
    constexpr std::array<std::pair<int, int>, dispatches_2d> two_d_sparse_hits{
        { { 10, 20 }, { 30, 40 }, { 50, 60 }, { 70, 70 } }
    };
    constexpr std::array<std::pair<int, int>, dispatches_2d> two_d_sparse_misses{
        { { 0, 0 }, { 5, 15 }, { 25, 35 }, { 100, 100 } }
    };

    // ========== 1D Dispatch ==========
    run_case(bench, dispatches_1d, "1D contiguous hit", [&] {
        return dispatch_1d_batch<range_1d_contig>(one_d_contig_hits, 2);
    });
    run_case(bench, dispatches_1d, "1D contiguous miss", [&] {
        return dispatch_1d_batch<range_1d_contig>(one_d_contig_misses, 2);
    });
    run_case(bench, dispatches_1d, "1D non-contiguous hit", [&] {
        return dispatch_1d_batch<range_1d_noncontig>(one_d_sparse_hits, 2);
    });
    run_case(bench, dispatches_1d, "1D non-contiguous miss", [&] {
        return dispatch_1d_batch<range_1d_noncontig>(one_d_sparse_misses, 2);
    });

    // ========== 2D Dispatch ==========
    run_case(bench, dispatches_2d, "2D contiguous hit", [&] {
        return dispatch_2d_batch<range_2d_contig>(two_d_contig_hits, 2);
    });
    run_case(bench, dispatches_2d, "2D contiguous miss", [&] {
        return dispatch_2d_batch<range_2d_contig>(two_d_contig_misses, 2);
    });
    run_case(bench, dispatches_2d, "2D non-contiguous hit", [&] {
        return dispatch_2d_batch<range_2d_noncontig>(two_d_sparse_hits, 2);
    });
    run_case(bench, dispatches_2d, "2D non-contiguous miss", [&] {
        return dispatch_2d_batch<range_2d_noncontig>(two_d_sparse_misses, 2);
    });

    // ========== 5D Dispatch (table size = 4^5 = 1024) ==========
    run_case(bench, dispatches_5d, "5D contiguous hit", [] {
        const int noise = next_noise();
        auto values = make_contig_hit_values(noise);
        ankerl::nanobench::doNotOptimizeAway(values);
        return dispatch_5d_repeated<range_5d_contig>(values, dispatches_5d, 3);
    });
    run_case(bench, dispatches_5d, "5D contiguous miss", [] {
        const int noise = next_noise();
        auto values = make_contig_hit_values(noise);
        values[0] = 5;
        ankerl::nanobench::doNotOptimizeAway(values);
        return dispatch_5d_repeated<range_5d_contig>(values, dispatches_5d, 3);
    });
    run_case(bench, dispatches_5d, "5D non-contiguous hit", [] {
        const int noise = next_noise();
        auto values = make_noncontig_hit_values(noise);
        ankerl::nanobench::doNotOptimizeAway(values);
        return dispatch_5d_repeated<range_5d_noncontig>(values, dispatches_5d, 3);
    });
    run_case(bench, dispatches_5d, "5D non-contiguous miss", [] {
        const int noise = next_noise();
        auto values = make_noncontig_hit_values(noise);
        values[0] = 5;
        ankerl::nanobench::doNotOptimizeAway(values);
        return dispatch_5d_repeated<range_5d_noncontig>(values, dispatches_5d, 3);
    });

    return 0;
}
