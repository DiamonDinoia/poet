#include <cstddef>
#include <array>
#include <utility>
#include <nanobench.h>

#include <poet/core/static_dispatch.hpp>
#include <poet/core/macros.hpp>

namespace {

// ============================================================================
// Complete switch implementation for sparse sequences
// ============================================================================

template<typename Functor, int... Values>
struct sparse_switch_impl;

// Base case
template<typename Functor>
struct sparse_switch_impl<Functor> {
    static POET_FORCEINLINE void dispatch(int /*val*/, Functor& /*func*/) {
        // No match
    }
};

// Recursive case - generates proper switch
template<typename Functor, int First, int... Rest>
struct sparse_switch_impl<Functor, First, Rest...> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        if (val == First) {
            func.template operator()<First>();
        } else if constexpr (sizeof...(Rest) > 0) {
            sparse_switch_impl<Functor, Rest...>::dispatch(val, func);
        }
    }
};

// Wrapper that unpacks integer_sequence
template<typename Seq, typename Functor>
struct sparse_switch_dispatcher;

template<int... Vals, typename Functor>
struct sparse_switch_dispatcher<std::integer_sequence<int, Vals...>, Functor> {
    static POET_FORCEINLINE void dispatch(int val, Functor& func) {
        // Modern compilers will optimize this if-else chain into a jump table
        // or binary search for sparse values
        sparse_switch_impl<Functor, Vals...>::dispatch(val, func);
    }
};

// ============================================================================
// Complete switch implementation for N-D dispatch (flattened)
// ============================================================================

// Helper: compute flat index from N-D indices
template<std::size_t N>
constexpr std::size_t flatten_nd_index(
    const std::array<int, N>& indices,
    const std::array<std::size_t, N>& strides) {

    std::size_t flat = 0;
    for (std::size_t i = 0; i < N; ++i) {
        flat += static_cast<std::size_t>(indices[i]) * strides[i];
    }
    return flat;
}

// Helper: compute strides for N-D array
template<std::size_t N>
constexpr std::array<std::size_t, N> compute_strides(const std::array<std::size_t, N>& dims) {
    std::array<std::size_t, N> strides{};
    if constexpr (N > 0) {
        strides[N - 1] = 1;
        for (std::size_t i = N - 1; i > 0; --i) {
            strides[i - 1] = strides[i] * dims[i];
        }
    }
    return strides;
}

// Generate all N-D combinations and flatten them
template<typename Functor, std::size_t FlatIdx, std::size_t TotalSize>
struct nd_switch_impl {
    template<typename Seq1, typename Seq2>
    static POET_FORCEINLINE void dispatch(std::size_t flat_idx, Functor& func, Seq1, Seq2) {
        if constexpr (FlatIdx < TotalSize) {
            if (flat_idx == FlatIdx) {
                // Unflatten the index to get original 2D coordinates
                // For 2D: i0 = flat / dim1, i1 = flat % dim1
                constexpr std::size_t dim1 = Seq2::size();
                constexpr int i0 = static_cast<int>(FlatIdx / dim1);
                constexpr int i1 = static_cast<int>(FlatIdx % dim1);

                // Get actual values from sequences
                constexpr int v0 = poet::detail::sequence_nth_value<Seq1, i0>::value;
                constexpr int v1 = poet::detail::sequence_nth_value<Seq2, i1>::value;

                func.template operator()<v0, v1>();
            } else {
                nd_switch_impl<Functor, FlatIdx + 1, TotalSize>::dispatch(flat_idx, func, Seq1{}, Seq2{});
            }
        }
    }
};

// 2D switch dispatcher
template<typename Seq1, typename Seq2, typename Functor>
struct nd_switch_dispatcher_2d {
    static POET_FORCEINLINE void dispatch(int v1, int v2, Functor& func) {
        constexpr std::size_t dim0 = Seq1::size();
        constexpr std::size_t dim1 = Seq2::size();
        constexpr std::size_t total = dim0 * dim1;

        // Find indices in sequences
        const auto idx0 = poet::detail::sequence_runtime_index<Seq1>::find(v1);
        const auto idx1 = poet::detail::sequence_runtime_index<Seq2>::find(v2);

        if (idx0 && idx1) {
            const std::size_t flat_idx = (*idx0) * dim1 + (*idx1);
            nd_switch_impl<Functor, 0, total>::dispatch(flat_idx, func, Seq1{}, Seq2{});
        }
    }
};

// ============================================================================
// Test functors
// ============================================================================

struct simple_functor {
    int* result;

    template<int V>
    void operator()() const {
        *result += V;
    }
};

struct functor_2d {
    int* result;

    template<int V1, int V2>
    void operator()() const {
        *result += V1 * 100 + V2;
    }
};

// ============================================================================
// Benchmarks
// ============================================================================

void bench_sparse_complete(ankerl::nanobench::Bench& bench) {
    int result = 0;
    using SparseSeq = std::integer_sequence<int, 1, 5, 10, 50, 100, 200, 500, 1000>;
    constexpr int test_val = 100;

    bench.run("Sparse (8 values): poet::dispatch", [&] {
        simple_functor func{&result};
        poet::dispatch(func, poet::DispatchParam<SparseSeq>{test_val});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("Sparse (8 values): complete switch", [&] {
        simple_functor func{&result};
        sparse_switch_dispatcher<SparseSeq, simple_functor>::dispatch(test_val, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

void bench_2d_flattened_complete(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int v1 = 2;
    constexpr int v2 = 3;

    bench.run("2D 4x4 flattened: poet::dispatch", [&] {
        functor_2d func{&result};
        poet::dispatch(func,
            poet::DispatchParam<poet::make_range<0, 3>>{v1},
            poet::DispatchParam<poet::make_range<0, 3>>{v2});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("2D 4x4 flattened: complete switch", [&] {
        functor_2d func{&result};
        nd_switch_dispatcher_2d<
            poet::make_range<0, 3>,
            poet::make_range<0, 3>,
            functor_2d>::dispatch(v1, v2, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

void bench_2d_8x8_flattened(ankerl::nanobench::Bench& bench) {
    int result = 0;
    constexpr int v1 = 4;
    constexpr int v2 = 5;

    bench.run("2D 8x8 (64 cases): poet::dispatch", [&] {
        functor_2d func{&result};
        poet::dispatch(func,
            poet::DispatchParam<poet::make_range<0, 7>>{v1},
            poet::DispatchParam<poet::make_range<0, 7>>{v2});
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("2D 8x8 (64 cases): flattened switch", [&] {
        functor_2d func{&result};
        nd_switch_dispatcher_2d<
            poet::make_range<0, 7>,
            poet::make_range<0, 7>,
            functor_2d>::dispatch(v1, v2, func);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

} // namespace

void run_switch_complete_benchmarks() {
    ankerl::nanobench::Bench bench;
    bench.title("Complete Switch Implementation: Sparse + N-D");
    bench.minEpochTime(10ms);
    bench.relative(true);

    bench.run("====== Sparse Sequences ======", [] {});
    bench_sparse_complete(bench);

    bench.run("====== 2D Flattened (4x4) ======", [] {});
    bench_2d_flattened_complete(bench);

    bench.run("====== 2D Flattened (8x8 = 64 cases) ======", [] {});
    bench_2d_8x8_flattened(bench);
}
