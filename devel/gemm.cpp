#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <poet/poet.hpp>
#include <random>
#include <type_traits>
#include <vector>

// Restrict macro for better vectorization
#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict__
#elif defined(_MSC_VER)
#define RESTRICT __restrict
#else
#define RESTRICT
#endif
// xsimd for SIMD vectorization of the unrolled microkernel
// xsimd headers trigger some warnings that are turned into errors by our -Werror build flags.
// Silence those warnings locally when including xsimd.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
// xsimd support removed from this file
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace blis_like {

// ------------------------
// Architecture descriptors
// ------------------------
struct ArchGeneric {
    static constexpr int MR = 6;
    static constexpr int NR = 8;
    static constexpr int KC = 256;
    static constexpr int MC = 512;
    static constexpr int NC = 2048;
};

// Intel processors with AVX-512 (Skylake-X, Ice Lake, Sapphire Rapids)
struct ArchIntelAVX512 {
    static constexpr int MR = 8;
    static constexpr int NR = 16;// AVX-512: 8 doubles per vector
    static constexpr int KC = 512;
    static constexpr int MC = 1536;
    static constexpr int NC = 8192;
};

// AMD Zen 3/4 (Ryzen 5000/7000 series)
struct ArchAMDZen3 {
    static constexpr int MR = 8;// More GP registers available
    static constexpr int NR = 8;
    static constexpr int KC = 384;// 32 KB L1 data cache
    static constexpr int MC = 768;// 512 KB L2 cache
    static constexpr int NC = 4096;// Large shared L3 cache
};

// Conservative fallback for unknown architectures
struct ArchFallback {
    static constexpr int MR = 4;
    static constexpr int NR = 4;
    static constexpr int KC = 192;
    static constexpr int MC = 384;
    static constexpr int NC = 2048;
};

// ------------------------
// Utilities
// ------------------------

template<class T> [[gnu::always_inline]] inline void scale_C(int m, int n, T beta, T *RESTRICT C, int ldc) {
    if (beta == T(1)) return;
    if (beta == T(0)) {
        for (int i = 0; i < m; ++i) {
            T *cptr = C + i * ldc;
            std::fill(cptr, cptr + n, T(0));
        }
        return;
    }
    for (int i = 0; i < m; ++i) {
        T *cptr = C + i * ldc;
        for (int j = 0; j < n; ++j) cptr[j] *= beta;
    }
}

// (Previously had lightweight runtime profiling accumulators here; removed
// to keep the binary focused on the single fastest configuration.)

// ------------------------
// Packing helpers (unused in the driver but kept for completeness)
// ------------------------

template<class T>
[[gnu::always_inline]] inline void pack_A_block(int mc, int kc, const T *RESTRICT A, int lda, T *RESTRICT Atilde) {
    for (int i = 0; i < mc; ++i) {
        const T *arow = A + i * lda;
        T *out = Atilde + i * kc;
        for (int p = 0; p < kc; ++p) out[p] = arow[p];
    }
}

template<class T>
[[gnu::always_inline]] inline void pack_B_block(int kc, int nc, const T *RESTRICT B, int ldb, T *RESTRICT Btilde) {
    for (int p = 0; p < kc; ++p) {
        const T *brow = B + p * ldb;
        T *out = Btilde + p * nc;
        for (int j = 0; j < nc; ++j) out[j] = brow[j];
    }
}

// ------------------------
// Microkernels
// ------------------------

template<class T>
void microkernel_runtime(int kc,
  const T *RESTRICT A_sub,
  int lda_t,
  const T *RESTRICT B_sub,
  int ldb_t,
  T *RESTRICT C_sub,
  int ldc,
  int mr,
  int nr,
  int MR_max,
  int NR_max,
  T alpha) {
    // Avoid per-call heap allocations for the accumulator by using a
    // small stack buffer for the common maximum MR/NR sizes. If the
    // requested MR_max/NR_max exceeds our static limits, fall back to
    // a heap allocation as before.
    constexpr std::size_t STATIC_MAX_MR = 8;
    constexpr std::size_t STATIC_MAX_NR = 16;

    const std::size_t MRs = static_cast<std::size_t>(MR_max);
    const std::size_t NRs = static_cast<std::size_t>(NR_max);
    const std::size_t acc_size = MRs * NRs;

    alignas(64) T acc_static[STATIC_MAX_MR * STATIC_MAX_NR];
    T *acc_ptr = nullptr;
    std::vector<T> acc_heap;

    if (MRs <= STATIC_MAX_MR && NRs <= STATIC_MAX_NR) {
        // zero only the needed portion
        for (std::size_t i = 0; i < acc_size; ++i) acc_static[i] = T(0);
        acc_ptr = acc_static;
    } else {
        // requested accumulator bigger than static static buffer: fall
        // back to heap allocation (rare path)
        acc_heap.assign(acc_size, T(0));
        acc_ptr = acc_heap.data();
    }

    for (int p = 0; p < kc; ++p) {
        for (int i = 0; i < mr; ++i) {
            const std::size_t is = static_cast<std::size_t>(i);
            T a_ip = A_sub[i * lda_t + p];
            const T *bptr = B_sub + p * ldb_t;
            for (int j = 0; j < nr; ++j) {
                const std::size_t js = static_cast<std::size_t>(j);
                acc_ptr[is * NRs + js] += a_ip * bptr[j];
            }
        }
    }

    for (int i = 0; i < mr; ++i) {
        const std::size_t is = static_cast<std::size_t>(i);
        T *cptr = C_sub + i * ldc;
        for (int j = 0; j < nr; ++j) {
            const std::size_t js = static_cast<std::size_t>(j);
            cptr[j] += alpha * acc_ptr[is * NRs + js];
        }
    }
}

template<int MR_max, int NR_max, class T>
[[gnu::always_inline]] inline void microkernel_template(int kc,
  const T *RESTRICT A_sub,
  int lda_t,
  const T *RESTRICT B_sub,
  int ldb_t,
  T *RESTRICT C_sub,
  int ldc,
  int mr,
  int nr,
  T alpha) {
    static_assert(MR_max > 0 && NR_max > 0, "MR_max and NR_max must be positive");
    constexpr std::size_t MRs = static_cast<std::size_t>(MR_max);
    constexpr std::size_t NRs = static_cast<std::size_t>(NR_max);
    std::array<T, MRs * NRs> acc{};

    for (int p = 0; p < kc; ++p) {
        for (int i = 0; i < mr; ++i) {
            const std::size_t is = static_cast<std::size_t>(i);
            T a_ip = A_sub[i * lda_t + p];
            const T *bptr = B_sub + p * ldb_t;
            for (int j = 0; j < nr; ++j) {
                const std::size_t js = static_cast<std::size_t>(j);
                acc[is * NRs + js] += a_ip * bptr[j];
            }
        }
    }

    for (int i = 0; i < mr; ++i) {
        const std::size_t is = static_cast<std::size_t>(i);
        T *cptr = C_sub + i * ldc;
        for (int j = 0; j < nr; ++j) {
            const std::size_t js = static_cast<std::size_t>(j);
            cptr[j] += alpha * acc[is * NRs + js];
        }
    }
}

template<int tMR, int tNR, class T>
[[gnu::always_inline]] inline void microkernel_fulltile(int kc,
  const T *RESTRICT A_sub,
  int lda_t,
  const T *RESTRICT B_sub,
  int ldb_t,
  T *RESTRICT C_sub,
  int ldc,
  T alpha) {
    static_assert(tMR > 0 && tNR > 0, "tile sizes must be positive");
    constexpr std::size_t MRs = static_cast<std::size_t>(tMR);
    constexpr std::size_t NRs = static_cast<std::size_t>(tNR);
    std::array<T, MRs * NRs> acc{};

    for (int p = 0; p < kc; ++p) {
        for (int i = 0; i < tMR; ++i) {
            const auto is = static_cast<std::size_t>(i);
            T a_ip = A_sub[i * lda_t + p];
            const T *bptr = B_sub + p * ldb_t;
            for (int j = 0; j < tNR; ++j) {
                const auto js = static_cast<std::size_t>(j);
                acc[is * NRs + js] += a_ip * bptr[j];
            }
        }
    }

    for (int i = 0; i < tMR; ++i) {
        const auto is = static_cast<std::size_t>(i);
        T *cptr = C_sub + i * ldc;
        for (int j = 0; j < tNR; ++j) {
            const auto js = static_cast<std::size_t>(j);
            cptr[j] += alpha * acc[is * NRs + js];
        }
    }
}

// ------------------------
// poet::static_for helpers (unrolled path)
// ------------------------

// FULL TILE helpers - NO runtime conditionals!
template<int MR_max, int NR_max, class T> struct microkernel_unroll_inner_helper_full {
    T a_ip;
    const T *bptr;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t i;

    template<auto J> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t j = static_cast<std::size_t>(J);
        // NO conditional - operates on full NR_max
        (*acc)[i * static_cast<std::size_t>(NR_max) + j] += a_ip * bptr[j];
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_outer_helper_fulltile {
    const T *A_sub;
    const T *B_sub;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t lda_t;
    std::size_t ldb_t;
    std::size_t p;

    template<auto I> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t i = static_cast<std::size_t>(I);
        // NO conditional - operates on full MR_max x NR_max
        const std::size_t baseA = i * lda_t + p;
        T a_ip = A_sub[baseA];
        const T *bptr = B_sub + p * ldb_t;
        microkernel_unroll_inner_helper_full<MR_max, NR_max, T> inner{ a_ip, bptr, acc, i };
        poet::static_for<0, NR_max>(inner);
    }
};

// PARTIAL TILE helpers - runtime conditionals for tail handling
template<int MR_max, int NR_max, class T> struct microkernel_unroll_inner_helper {
    T a_ip;
    const T *bptr;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t nr;
    std::size_t i;

    template<auto J> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t j = static_cast<std::size_t>(J);
        (*acc)[i * static_cast<std::size_t>(NR_max) + j] += a_ip * bptr[j];
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_outer_helper_full {
    const T *A_sub;
    const T *B_sub;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t lda_t;
    std::size_t ldb_t;
    std::size_t mr;
    std::size_t p;

    template<auto I> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t i = static_cast<std::size_t>(I);
        if (i < static_cast<std::size_t>(mr)) {
            const std::size_t baseA = i * lda_t + p;
            T a_ip = A_sub[baseA];
            const T *bptr = B_sub + p * ldb_t;
            microkernel_unroll_inner_helper<MR_max, NR_max, T> inner{
                a_ip, bptr, acc, static_cast<std::size_t>(NR_max), i
            };
            poet::static_for<0, NR_max>(inner);
        }
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_outer_helper_tail {
    const T *A_sub;
    const T *B_sub;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t lda_t;
    std::size_t ldb_t;
    std::size_t mr;
    std::size_t nr;
    std::size_t p;

    template<auto I> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t i = static_cast<std::size_t>(I);
        if (i < static_cast<std::size_t>(mr)) {
            const std::size_t baseA = i * lda_t + p;
            T a_ip = A_sub[baseA];
            const T *bptr = B_sub + p * ldb_t;
            for (std::size_t j = 0; j < nr; ++j) { (*acc)[i * static_cast<std::size_t>(NR_max) + j] += a_ip * bptr[j]; }
        }
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_store_inner_helper_full {
    T *cptr;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t i;
    T alpha;

    template<auto J> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t j = static_cast<std::size_t>(J);
        // NO conditional - operates on full NR_max
        cptr[j] += alpha * (*acc)[i * static_cast<std::size_t>(NR_max) + j];
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_store_outer_helper_fulltile {
    T *C_sub;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t ldc;
    T alpha;

    template<auto I> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t i = static_cast<std::size_t>(I);
        // NO conditional - operates on full MR_max x NR_max
        T *cptr = C_sub + i * ldc;
        microkernel_unroll_store_inner_helper_full<MR_max, NR_max, T> storej{ cptr, acc, i, alpha };
        poet::static_for<0, NR_max>(storej);
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_store_inner_helper {
    T *cptr;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t nr;
    std::size_t i;
    T alpha;

    template<auto J> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t j = static_cast<std::size_t>(J);
        cptr[j] += alpha * (*acc)[i * static_cast<std::size_t>(NR_max) + j];
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_store_outer_helper_full {
    T *C_sub;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t ldc;
    std::size_t mr;
    T alpha;

    template<auto I> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t i = static_cast<std::size_t>(I);
        if (i < static_cast<std::size_t>(mr)) {
            T *cptr = C_sub + i * ldc;
            microkernel_unroll_store_inner_helper<MR_max, NR_max, T> storej{
                cptr, acc, static_cast<std::size_t>(NR_max), i, alpha
            };
            poet::static_for<0, NR_max>(storej);
        }
    }
};

template<int MR_max, int NR_max, class T> struct microkernel_unroll_store_outer_helper_tail {
    T *C_sub;
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> *acc;
    std::size_t ldc;
    std::size_t mr;
    std::size_t nr;
    T alpha;

    template<auto I> [[gnu::always_inline]] inline void operator()() const {
        constexpr std::size_t i = static_cast<std::size_t>(I);
        if (i < static_cast<std::size_t>(mr)) {
            T *cptr = C_sub + i * ldc;
            for (std::size_t j = 0; j < nr; ++j) {
                cptr[j] += alpha * (*acc)[i * static_cast<std::size_t>(NR_max) + j];
            }
        }
    }
};

// ========== NON-SIMD FULL TILE MICROKERNEL: Zero runtime branches ==========
template<int MR, int NR, class T>
[[gnu::always_inline]] inline void microkernel_unroll_full(int kc,
  const T *RESTRICT A_sub,
  int lda_t,
  const T *RESTRICT B_sub,
  int ldb_t,
  T *RESTRICT C_sub,
  int ldc,
  T alpha) {
    static_assert(MR > 0 && NR > 0, "tile sizes must be positive");
    constexpr std::size_t MRs = static_cast<std::size_t>(MR);
    constexpr std::size_t NRs = static_cast<std::size_t>(NR);
    std::array<T, MRs * NRs> acc{};

    // ========== FULL TILE PATH: NO runtime conditionals! ==========
    for (int p = 0; p < kc; ++p) {
        microkernel_unroll_outer_helper_fulltile<MR, NR, T> outer{ A_sub,
            B_sub,
            &acc,
            static_cast<std::size_t>(lda_t),
            static_cast<std::size_t>(ldb_t),
            static_cast<std::size_t>(p) };
        poet::static_for<0, MR>(outer);
    }

    microkernel_unroll_store_outer_helper_fulltile<MR, NR, T> store{
        C_sub, &acc, static_cast<std::size_t>(ldc), alpha
    };
    poet::static_for<0, MR>(store);
}

// ========== NON-SIMD PARTIAL TILE MICROKERNEL: Simple fallback for tails ==========
template<int MR, int NR, class T>
[[gnu::always_inline]] inline void microkernel_unroll_partial(int kc,
  const T *RESTRICT A_sub,
  int lda_t,
  const T *RESTRICT B_sub,
  int ldb_t,
  T *RESTRICT C_sub,
  int ldc,
  int mr,
  int nr,
  T alpha) {
    static_assert(MR > 0 && NR > 0, "tile sizes must be positive");
    constexpr std::size_t MRs = static_cast<std::size_t>(MR);
    constexpr std::size_t NRs = static_cast<std::size_t>(NR);
    std::array<T, MRs * NRs> acc{};

    // ========== PARTIAL TILE PATH: Runtime loops for tails ==========
    for (int p = 0; p < kc; ++p) {
        if (nr == NR) {
            microkernel_unroll_outer_helper_full<MR, NR, T> outer{ A_sub,
                B_sub,
                &acc,
                static_cast<std::size_t>(lda_t),
                static_cast<std::size_t>(ldb_t),
                static_cast<std::size_t>(mr),
                static_cast<std::size_t>(p) };
            poet::static_for<0, MR>(outer);
        } else {
            microkernel_unroll_outer_helper_tail<MR, NR, T> outer{ A_sub,
                B_sub,
                &acc,
                static_cast<std::size_t>(lda_t),
                static_cast<std::size_t>(ldb_t),
                static_cast<std::size_t>(mr),
                static_cast<std::size_t>(nr),
                static_cast<std::size_t>(p) };
            poet::static_for<0, MR>(outer);
        }
    }

    if (nr == NR) {
        microkernel_unroll_store_outer_helper_full<MR, NR, T> store{
            C_sub, &acc, static_cast<std::size_t>(ldc), static_cast<std::size_t>(mr), alpha
        };
        poet::static_for<0, MR>(store);
    } else {
        microkernel_unroll_store_outer_helper_tail<MR, NR, T> store{ C_sub,
            &acc,
            static_cast<std::size_t>(ldc),
            static_cast<std::size_t>(mr),
            static_cast<std::size_t>(nr),
            alpha };
        poet::static_for<0, MR>(store);
    }
}

// xsimd microkernels removed: the xsimd-based full/partial/streaming
// microkernel implementations have been deleted to remove the dependency
// on xsimd in this file. Other scalar/SIMD paths remain intact.

// ---------- New: branch-free wrappers for unrolled path ----------

template<int MR_max, int NR_max, class T>
[[gnu::always_inline]] inline void microkernel_unroll_fullN(int kc,
  const T *A_sub,
  int lda_t,
  const T *B_sub,
  int ldb_t,
  T *C_sub,
  int ldc,
  int mr,
  T alpha) {
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> acc{};
    for (int p = 0; p < kc; ++p) {
        microkernel_unroll_outer_helper_full<MR_max, NR_max, T> outer{ A_sub,
            B_sub,
            &acc,
            static_cast<std::size_t>(lda_t),
            static_cast<std::size_t>(ldb_t),
            static_cast<std::size_t>(mr),
            static_cast<std::size_t>(p) };
        poet::static_for<0, MR_max>(outer);
    }
    microkernel_unroll_store_outer_helper_full<MR_max, NR_max, T> store{
        C_sub, &acc, static_cast<std::size_t>(ldc), static_cast<std::size_t>(mr), alpha
    };
    poet::static_for<0, MR_max>(store);
}

template<int MR_max, int NR_max, class T>
[[gnu::always_inline]] inline void microkernel_unroll_tailN(int kc,
  const T *A_sub,
  int lda_t,
  const T *B_sub,
  int ldb_t,
  T *C_sub,
  int ldc,
  int mr,
  int nr,
  T alpha) {
    std::array<T, static_cast<std::size_t>(MR_max) * static_cast<std::size_t>(NR_max)> acc{};
    for (int p = 0; p < kc; ++p) {
        microkernel_unroll_outer_helper_tail<MR_max, NR_max, T> outer{ A_sub,
            B_sub,
            &acc,
            static_cast<std::size_t>(lda_t),
            static_cast<std::size_t>(ldb_t),
            static_cast<std::size_t>(mr),
            static_cast<std::size_t>(nr),
            static_cast<std::size_t>(p) };
        poet::static_for<0, MR_max>(outer);
    }
    microkernel_unroll_store_outer_helper_tail<MR_max, NR_max, T> store{
        C_sub, &acc, static_cast<std::size_t>(ldc), static_cast<std::size_t>(mr), static_cast<std::size_t>(nr), alpha
    };
    poet::static_for<0, MR_max>(store);
}

// ---------- New: partial/full microkernels for non-unroll path ----------

template<int tMR, int tNR, class T>
[[gnu::always_inline]] inline void microkernel_mtail_nfull(int kc,
  const T *A_sub,
  int lda_t,
  const T *B_sub,
  int ldb_t,
  T *C_sub,
  int ldc,
  int mr,
  T alpha) {
    static_assert(tMR > 0 && tNR > 0, "tile sizes must be positive");
    constexpr std::size_t MRs = static_cast<std::size_t>(tMR);
    constexpr std::size_t NRs = static_cast<std::size_t>(tNR);
    std::array<T, MRs * NRs> acc{};

    // Accumulate over mr rows, full tNR cols using static_for
    for (int p = 0; p < kc; ++p) {
        microkernel_unroll_outer_helper_full<tMR, tNR, T> outer{ A_sub,
            B_sub,
            &acc,
            static_cast<std::size_t>(lda_t),
            static_cast<std::size_t>(ldb_t),
            static_cast<std::size_t>(mr),
            static_cast<std::size_t>(p) };
        poet::static_for<0, tMR>(outer);
    }

    // Store over mr rows, full tNR cols using static_for
    microkernel_unroll_store_outer_helper_full<tMR, tNR, T> store{
        C_sub, &acc, static_cast<std::size_t>(ldc), static_cast<std::size_t>(mr), alpha
    };
    poet::static_for<0, tMR>(store);
}

template<int tMR, int tNR, class T>
[[gnu::always_inline]] inline void microkernel_mfull_ntail(int kc,
  const T *A_sub,
  int lda_t,
  const T *B_sub,
  int ldb_t,
  T *C_sub,
  int ldc,
  int nr,
  T alpha) {
    static_assert(tMR > 0 && tNR > 0, "tile sizes must be positive");
    constexpr std::size_t MRs = static_cast<std::size_t>(tMR);
    constexpr std::size_t NRs = static_cast<std::size_t>(tNR);
    std::array<T, MRs * NRs> acc{};

    // Accumulate over full tMR rows, nr tail cols using static_for for i and scalar for j tail
    for (int p = 0; p < kc; ++p) {
        microkernel_unroll_outer_helper_tail<tMR, tNR, T> outer{ A_sub,
            B_sub,
            &acc,
            static_cast<std::size_t>(lda_t),
            static_cast<std::size_t>(ldb_t),
            static_cast<std::size_t>(tMR),
            static_cast<std::size_t>(nr),
            static_cast<std::size_t>(p) };
        poet::static_for<0, tMR>(outer);
    }

    // Store over full tMR rows, nr tail cols using static_for for i and scalar for j tail
    microkernel_unroll_store_outer_helper_tail<tMR, tNR, T> store{
        C_sub, &acc, static_cast<std::size_t>(ldc), static_cast<std::size_t>(tMR), static_cast<std::size_t>(nr), alpha
    };
    poet::static_for<0, tMR>(store);
}

// ------------------------
// Optimized packing helpers
// ------------------------

// Optimized B-panel packing with better cache behavior
template<typename T>
[[gnu::always_inline]] inline void
  pack_B_panel_optimized(T *dest, const T *B, int ldb, int kc, int nc, int tKC, int tNC, int pc, int jc) {

    // Pack full rows with minimal overhead
    for (int p = 0; p < kc; ++p) {
        const T *brow = B + (pc + p) * ldb + jc;
        T *out = dest + static_cast<std::size_t>(p) * static_cast<std::size_t>(tNC);

        // Prefetch next row
        if (p + 1 < kc) { __builtin_prefetch(B + (pc + p + 1) * ldb + jc, 0, 1); }

        // Copy actual data
        std::memcpy(out, brow, static_cast<std::size_t>(nc) * sizeof(T));

        // Zero-fill padding
        if (nc < tNC) { std::memset(out + nc, 0, static_cast<std::size_t>(tNC - nc) * sizeof(T)); }
    }

    // Zero-fill remaining rows if needed
    if (kc < tKC) {
        std::size_t remain_size = static_cast<std::size_t>(tKC - kc) * static_cast<std::size_t>(tNC);
        std::memset(dest + static_cast<std::size_t>(kc) * static_cast<std::size_t>(tNC), 0, remain_size * sizeof(T));
    }
}

// Optimized A-panel packing with better cache behavior
template<typename T>
[[gnu::always_inline]] inline void
  pack_A_panel_optimized(T *dest, const T *A, int lda, int mc, int kc, int tMC, int tKC, int ic, int pc) {

    // Pack full rows with minimal overhead
    for (int i = 0; i < mc; ++i) {
        const T *arow = A + (ic + i) * lda + pc;
        T *out = dest + static_cast<std::size_t>(i) * static_cast<std::size_t>(tKC);

        // Prefetch next row
        if (i + 1 < mc) { __builtin_prefetch(A + (ic + i + 1) * lda + pc, 0, 1); }

        // Copy actual data
        std::memcpy(out, arow, static_cast<std::size_t>(kc) * sizeof(T));

        // Zero-fill padding
        if (kc < tKC) { std::memset(out + kc, 0, static_cast<std::size_t>(tKC - kc) * sizeof(T)); }
    }

    // Zero-fill remaining rows if needed
    if (mc < tMC) {
        std::size_t remain_size = static_cast<std::size_t>(tMC - mc) * static_cast<std::size_t>(tKC);
        std::memset(dest + static_cast<std::size_t>(mc) * static_cast<std::size_t>(tKC), 0, remain_size * sizeof(T));
    }
}

// ------------------------
// Dispatch functors
// ------------------------

template<class T> struct gemm_static_functor {
    int M, N, K;
    const T *RESTRICT A;
    int lda;
    const T *RESTRICT B;
    int ldb;
    T *RESTRICT C;
    int ldc;
    T alpha;
    T beta;

    template<int tMR, int tNR, int tKC, int tMC, int tNC> [[gnu::always_inline]] inline void operator()() const {
        static_assert(tMR > 0 && tNR > 0 && tKC > 0 && tMC > 0 && tNC > 0, "tile sizes must be positive");

        scale_C(M, N, beta, C, ldc);

        // Reserve buffers once (reuse with resize).
        // Note: xsimd aligned allocator removed; use plain vectors here.
        std::vector<T> Bpanel;
        std::vector<T> Apanel;
        Bpanel.reserve(static_cast<size_t>(tKC) * static_cast<size_t>(tNC));
        Apanel.reserve(static_cast<size_t>(tMC) * static_cast<size_t>(tKC));

        // Packed-only execution path: always pack B and A once per slab. Removed
        // runtime profiling hooks so the binary focuses on the single fastest configuration.

        for (int jc = 0; jc < N; jc += tNC) {
            const int nc = std::min(tNC, N - jc);

            for (int pc = 0; pc < K; pc += tKC) {
                const int kc = std::min(tKC, K - pc);

                // Resize and pack B using optimized packing (no timing)
                Bpanel.resize(static_cast<size_t>(tKC) * static_cast<size_t>(tNC));
                pack_B_panel_optimized(Bpanel.data(), B, ldb, kc, nc, tKC, tNC, pc, jc);

                for (int ic = 0; ic < M; ic += tMC) {
                    const int mc = std::min(tMC, M - ic);

                    // Resize and pack A using optimized packing (no timing)
                    Apanel.resize(static_cast<size_t>(tMC) * static_cast<size_t>(tKC));
                    pack_A_panel_optimized(Apanel.data(), A, lda, mc, kc, tMC, tKC, ic, pc);

                    const int nc_full = (nc / tNR) * tNR;
                    const int mc_full = (mc / tMR) * tMR;

                    // A: Full jr × full ir (HOT PATH - zero runtime branches, perfect unrolling)
                    for (int jr = 0; jr < nc_full; jr += tNR) {
                        const T *Bsub = Bpanel.data() + jr;
                        for (int ir = 0; ir < mc_full; ir += tMR) {
                            const T *Asub = Apanel.data() + static_cast<size_t>(ir) * static_cast<size_t>(tKC);
                            T *Cblk = C + (ic + ir) * ldc + (jc + jr);
                            // Full tile microkernel - NO runtime mr/nr parameters!
                            microkernel_unroll_full<tMR, tNR, T>(kc, Asub, tKC, Bsub, tNC, Cblk, ldc, alpha);
                        }
                    }

                    // B: Full jr × tail ir (m-tail handling - partial tile)
                    if (mc_full != mc) {
                        const int mr_tail = mc - mc_full;
                        for (int jr = 0; jr < nc_full; jr += tNR) {
                            const T *Bsub = Bpanel.data() + jr;
                            const T *Asub = Apanel.data() + static_cast<size_t>(mc_full) * static_cast<size_t>(tKC);
                            T *Cblk = C + (ic + mc_full) * ldc + (jc + jr);
                            microkernel_unroll_partial<tMR, tNR, T>(
                              kc, Asub, tKC, Bsub, tNC, Cblk, ldc, mr_tail, tNR, alpha);
                        }
                    }

                    // C: Tail jr × full ir (n-tail handling - partial tile)
                    if (nc_full != nc) {
                        const int nr_tail = nc - nc_full;
                        const T *Bsub;
                        int ldb_t;
                        if constexpr (std::is_same<T, float>::value) {
                            Bsub = Bpanel.data() + static_cast<size_t>(nc_full) * static_cast<size_t>(tKC);
                            ldb_t = tKC;
                        } else {
                            Bsub = Bpanel.data() + nc_full;
                            ldb_t = tNC;
                        }
                        for (int ir = 0; ir < mc_full; ir += tMR) {
                            const T *Asub = Apanel.data() + static_cast<size_t>(ir) * static_cast<size_t>(tKC);
                            T *Cblk = C + (ic + ir) * ldc + (jc + nc_full);
                            microkernel_unroll_partial<tMR, tNR, T>(
                              kc, Asub, tKC, Bsub, ldb_t, Cblk, ldc, tMR, nr_tail, alpha);
                        }
                    }

                    // D: Tail jr × tail ir (corner case - rare, partial tile)
                    if (mc_full != mc && nc_full != nc) {
                        const int mr_tail = mc - mc_full;
                        const int nr_tail = nc - nc_full;
                        const T *Bsub;
                        int ldb_t;
                        if constexpr (std::is_same<T, float>::value) {
                            Bsub = Bpanel.data() + static_cast<size_t>(nc_full) * static_cast<size_t>(tKC);
                            ldb_t = tKC;
                        } else {
                            Bsub = Bpanel.data() + nc_full;
                            ldb_t = tNC;
                        }
                        const T *Asub = Apanel.data() + static_cast<size_t>(mc_full) * static_cast<size_t>(tKC);
                        T *Cblk = C + (ic + mc_full) * ldc + (jc + nc_full);
                        microkernel_unroll_partial<tMR, tNR, T>(
                          kc, Asub, tKC, Bsub, ldb_t, Cblk, ldc, mr_tail, nr_tail, alpha);
                    }
                }
            }
        }
    }
};

template<class T> struct gemm_static_unroll_functor {
    int M, N, K;
    const T *RESTRICT A;
    int lda;
    const T *RESTRICT B;
    int ldb;
    T *RESTRICT C;
    int ldc;
    T alpha;
    T beta;

    template<int tMR, int tNR, int tKC, int tMC, int tNC> [[gnu::always_inline]] inline void operator()() const {
        static_assert(tMR > 0 && tNR > 0 && tKC > 0 && tMC > 0 && tNC > 0, "tile sizes must be positive");

        scale_C(M, N, beta, C, ldc);

        // Reserve buffers once (reuse with resize)
        std::vector<T> Bpanel;
        std::vector<T> Apanel;
        Bpanel.reserve(static_cast<size_t>(tKC) * static_cast<size_t>(tNC));
        Apanel.reserve(static_cast<size_t>(tMC) * static_cast<size_t>(tKC));

        // Unrolled static functor removed: keeping only core packed/static functor
        // and runtime/dispatch entry points. The unrolled dispatch variant is
        // omitted per request to keep only naive, packed, and dispatch paths.

        for (int jc = 0; jc < N; jc += tNC) {
            const int nc = std::min(tNC, N - jc);

            for (int pc = 0; pc < K; pc += tKC) {
                const int kc = std::min(tKC, K - pc);

                // Resize and pack B using optimized packing
                Bpanel.resize(static_cast<size_t>(tKC) * static_cast<size_t>(tNC));
                pack_B_panel_optimized(Bpanel.data(), B, ldb, kc, nc, tKC, tNC, pc, jc);

                for (int ic = 0; ic < M; ic += tMC) {
                    const int mc = std::min(tMC, M - ic);

                    // Resize and pack A using optimized packing
                    Apanel.resize(static_cast<size_t>(tMC) * static_cast<size_t>(tKC));
                    pack_A_panel_optimized(Apanel.data(), A, lda, mc, kc, tMC, tKC, ic, pc);

                    const int nc_full = (nc / tNR) * tNR;
                    const int mc_full = (mc / tMR) * tMR;

                    // A: Full jr × full ir (HOT PATH - zero runtime branches, perfect unrolling)
                    for (int jr = 0; jr < nc_full; jr += tNR) {
                        const T *Bsub = Bpanel.data() + jr;
                        for (int ir = 0; ir < mc_full; ir += tMR) {
                            const T *Asub = Apanel.data() + static_cast<size_t>(ir) * static_cast<size_t>(tKC);
                            T *Cblk = C + (ic + ir) * ldc + (jc + jr);
                            // Full tile microkernel - NO runtime mr/nr parameters!
                            microkernel_unroll_full<tMR, tNR, T>(kc, Asub, tKC, Bsub, tNC, Cblk, ldc, alpha);
                        }
                    }

                    // B: Full jr × tail ir (m-tail handling - partial tile)
                    if (mc_full != mc) {
                        const int mr_tail = mc - mc_full;
                        for (int jr = 0; jr < nc_full; jr += tNR) {
                            const T *Bsub = Bpanel.data() + jr;
                            const T *Asub = Apanel.data() + static_cast<size_t>(mc_full) * static_cast<size_t>(tKC);
                            T *Cblk = C + (ic + mc_full) * ldc + (jc + jr);
                            microkernel_unroll_partial<tMR, tNR, T>(
                              kc, Asub, tKC, Bsub, tNC, Cblk, ldc, mr_tail, tNR, alpha);
                        }
                    }

                    // C: Tail jr × full ir (n-tail handling - partial tile)
                    if (nc_full != nc) {
                        const int nr_tail = nc - nc_full;
                        const T *Bsub = Bpanel.data() + nc_full;
                        for (int ir = 0; ir < mc_full; ir += tMR) {
                            const T *Asub = Apanel.data() + static_cast<size_t>(ir) * static_cast<size_t>(tKC);
                            T *Cblk = C + (ic + ir) * ldc + (jc + nc_full);
                            microkernel_unroll_partial<tMR, tNR, T>(
                              kc, Asub, tKC, Bsub, tNC, Cblk, ldc, tMR, nr_tail, alpha);
                        }
                    }

                    // D: Tail jr × tail ir (corner case - rare, partial tile)
                    if (mc_full != mc && nc_full != nc) {
                        const int mr_tail = mc - mc_full;
                        const int nr_tail = nc - nc_full;
                        const T *Bsub = Bpanel.data() + nc_full;
                        const T *Asub = Apanel.data() + static_cast<size_t>(mc_full) * static_cast<size_t>(tKC);
                        T *Cblk = C + (ic + mc_full) * ldc + (jc + nc_full);
                        microkernel_unroll_partial<tMR, tNR, T>(
                          kc, Asub, tKC, Bsub, tNC, Cblk, ldc, mr_tail, nr_tail, alpha);
                    }
                }
            }
        }
    }
};

// xsimd-backed unroll functor and dispatch wrapper removed.
// The xsimd-specific `gemm_static_unroll_xsimd_functor` and
// `gemm_dispatch_unroll_xsimd` were deleted to remove xsimd usage.
// Use `gemm_dispatch_unroll` or other available paths instead.

// ------------------------
// Public wrappers
// ------------------------

template<class T>
[[gnu::flatten]] [[gnu::noinline]] inline void gemm_dispatch(int M,
  int N,
  int K,
  const T *RESTRICT A,
  int lda,
  const T *RESTRICT B,
  int ldb,
  T *RESTRICT C,
  int ldc,
  T alpha = T(1),
  T beta = T(1),
  int MR = 6,
  int NR = 16,
  int KC = 256,
  int MC = 480,
  int NC = 2048) {
    // MINIMAL dispatch for fast compilation - single optimal configuration
    using mr_choices = std::integer_sequence<int, 6>;
    using nr_choices = std::integer_sequence<int, 8>;
    using kc_choices = std::integer_sequence<int, 256>;
    using mc_choices = std::integer_sequence<int, 480>;
    using nc_choices = std::integer_sequence<int, 2048>;

    auto params = std::make_tuple(poet::DispatchParam<mr_choices>{ MR },
      poet::DispatchParam<nr_choices>{ NR },
      poet::DispatchParam<kc_choices>{ KC },
      poet::DispatchParam<mc_choices>{ MC },
      poet::DispatchParam<nc_choices>{ NC });

    gemm_static_functor<T> functor{ M, N, K, A, lda, B, ldb, C, ldc, alpha, beta };
    poet::dispatch(functor, params);
}

template<class T>
[[gnu::flatten]] [[gnu::noinline]] inline void gemm_dispatch_unroll(int M,
  int N,
  int K,
  const T *RESTRICT A,
  int lda,
  const T *RESTRICT B,
  int ldb,
  T *RESTRICT C,
  int ldc,
  T alpha = T(1),
  T beta = T(1),
  int MR = 6,
  int NR = 16,
  int KC = 256,
  int MC = 480,
  int NC = 2048) {
    // MINIMAL dispatch for fast compilation - single optimal configuration
    using mr_choices = std::integer_sequence<int, 6>;
    using nr_choices = std::integer_sequence<int, 8>;
    using kc_choices = std::integer_sequence<int, 256>;
    using mc_choices = std::integer_sequence<int, 480>;
    using nc_choices = std::integer_sequence<int, 2048>;

    auto params = std::make_tuple(poet::DispatchParam<mr_choices>{ MR },
      poet::DispatchParam<nr_choices>{ NR },
      poet::DispatchParam<kc_choices>{ KC },
      poet::DispatchParam<mc_choices>{ MC },
      poet::DispatchParam<nc_choices>{ NC });

    gemm_static_unroll_functor<T> functor{ M, N, K, A, lda, B, ldb, C, ldc, alpha, beta };
    poet::dispatch(functor, params);
}

template<class T>
void gemm(int M,
  int N,
  int K,
  const T *RESTRICT A,
  int lda,
  const T *RESTRICT B,
  int ldb,
  T *RESTRICT C,
  int ldc,
  T alpha = T(1),
  T beta = T(1),
  int MR = 6,
  int NR = 8,
  int KC = 256,
  int MC = 512,
  int NC = 2048) {
    static_assert(std::is_floating_point<T>::value, "T must be float or double");
    assert(M >= 0 && N >= 0 && K >= 0);
    assert(lda >= std::max(1, K));
    assert(ldb >= std::max(1, N));
    assert(ldc >= std::max(1, N));
    /*  */
    // Optional: scale C by beta once
    scale_C(M, N, beta, C, ldc);

    // Outer 3 loops: L3/L2 aware blocking with packing
    for (int jc = 0; jc < N; jc += NC) {
        int nc = std::min(NC, N - jc);

        // allocate B panel once per (pc) slab into this jc-range
        std::vector<T> Bpanel;
        Bpanel.reserve(static_cast<size_t>(KC) * static_cast<size_t>(nc));

        for (int pc = 0; pc < K; pc += KC) {
            int kc = std::min(KC, K - pc);

            // Pack B(pc : pc+kc, jc : jc+nc) -> kc x nc
            Bpanel.assign(static_cast<size_t>(kc) * static_cast<size_t>(nc), T(0));
            pack_B_block(kc, nc, B + pc * ldb + jc, ldb, Bpanel.data());

            for (int ic = 0; ic < M; ic += MC) {
                int mc = std::min(MC, M - ic);

                // Pack A(ic : ic+mc, pc : pc+kc) -> mc x kc
                std::vector<T> Apanel(static_cast<size_t>(mc) * static_cast<size_t>(kc), T(0));
                pack_A_block(mc, kc, A + ic * lda + pc, lda, Apanel.data());

                // Inner 2 loops over micro-panels (register level)
                for (int jr = 0; jr < nc; jr += NR) {
                    int nr = std::min(NR, nc - jr);
                    const T *Bsub = Bpanel.data() + jr;// column offset inside the kc x nc packed panel

                    for (int ir = 0; ir < mc; ir += MR) {
                        int mr = std::min(MR, mc - ir);
                        const T *Asub = Apanel.data() + ir * kc;// row off set inside the mc x kc packed panel
                        T *Cblk = C + (ic + ir) * ldc + (jc + jr);

                        microkernel_runtime<T>(
                          kc, Asub, /*lda_t=*/kc, Bsub, /*ldb_t=*/nc, Cblk, ldc, mr, nr, MR, NR, alpha);
                    }
                }
            }
        }
    }
}


template<class T>
[[gnu::flatten]] [[gnu::noinline]] inline void gemm_runtime(int M,
  int N,
  int K,
  const T *RESTRICT A,
  int lda,
  const T *RESTRICT B,
  int ldb,
  T *RESTRICT C,
  int ldc,
  T alpha = T(1),
  T beta = T(1),
  int MR = 6,
  int NR = 8,
  int KC = 256,
  int MC = 512,
  int NC = 2048) {
    volatile int vMR = MR;
    volatile int vNR = NR;
    volatile int vKC = KC;
    volatile int vMC = MC;
    volatile int vNC = NC;
    gemm<T>(M, N, K, A, lda, B, ldb, C, ldc, alpha, beta, vMR, vNR, vKC, vMC, vNC);
}

// ------------------------
// Naive GEMM
// ------------------------

template<class T>
[[gnu::flatten]] [[gnu::noinline]] inline void gemm_naive(int M,
  int N,
  int K,
  const T *RESTRICT A,
  int lda,
  const T *RESTRICT B,
  int ldb,
  T *RESTRICT C,
  int ldc,
  T alpha = T(1),
  T beta = T(1)) {
    static_assert(std::is_floating_point<T>::value, "T must be float or double");
    assert(M >= 0 && N >= 0 && K >= 0);
    assert(lda >= std::max(1, K));
    assert(ldb >= std::max(1, N));
    assert(ldc >= std::max(1, N));

    scale_C(M, N, beta, C, ldc);

    for (int i = 0; i < M; ++i) {
        const T *Ai = A + i * lda;
        T *Ci = C + i * ldc;
        for (int k = 0; k < K; ++k) {
            T aik = Ai[k];
            const T *Bk = B + k * ldb;
            for (int j = 0; j < N; ++j) { Ci[j] += alpha * aik * Bk[j]; }
        }
    }
}

}// namespace blis_like

struct ArchIntelUltra7155H {
    static constexpr int MR = 6;
    static constexpr int NR = 8;
    static constexpr int KC = 256;
    static constexpr int MC = 480;
    static constexpr int NC = 2048;
};

using T = double;

struct BenchmarkResult {
    std::string label;
    double total_seconds = 0.0;
    double avg_seconds = 0.0;
    long double checksum = 0.0L;
};

template<typename RunF, typename CheckF>
static BenchmarkResult run_gemm_benchmark(const std::string &label, int repeats, RunF run_once, CheckF checksum_fn) {
    BenchmarkResult res;
    res.label = label;

    run_once();

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < repeats; ++r) { run_once(); }
    auto t1 = std::chrono::high_resolution_clock::now();

    long double last_checksum = checksum_fn();

    std::chrono::duration<double> dur = t1 - t0;
    res.total_seconds = dur.count();
    res.avg_seconds = res.total_seconds / static_cast<double>(repeats);
    res.checksum = last_checksum;
    return res;
}

template<typename U> [[gnu::always_inline]] inline static long double compute_checksum(const std::vector<U> &v) {
    long double s = 0.0L;
    for (size_t i = 0; i < v.size(); ++i) s += static_cast<long double>(v[i]);
    return s;
}

int main() {
    const int M = 1024 / 2;
    const int N = 1024 / 2;
    const int K = 1024 / 2;
    const int repeats = 5;

    // Allocate inputs and result buffers for all variants so we can compare
    std::vector<T> A(static_cast<size_t>(M) * static_cast<size_t>(K), 0);
    std::vector<T> B(static_cast<size_t>(K) * static_cast<size_t>(N), 0);
    std::vector<T> C_naive(static_cast<size_t>(M) * static_cast<size_t>(N), 0);
    std::vector<T> C_packed(static_cast<size_t>(M) * static_cast<size_t>(N), 0);
    std::vector<T> C_dispatch(static_cast<size_t>(M) * static_cast<size_t>(N), 0);

    // Initialize A and B deterministically so checksums are repeatable
    std::mt19937 rng(42);
    std::uniform_real_distribution<T> dist(T(-1.0), T(1.0));
    for (size_t i = 0; i < A.size(); ++i) A[i] = dist(rng);
    for (size_t i = 0; i < B.size(); ++i) B[i] = dist(rng);

    T alpha = T(1.0);
    T beta = T(0.0);

    // Define run_once lambdas for each variant
    auto naive_run_once = [&]() {
        blis_like::gemm_naive<T>(M, N, K, A.data(), K, B.data(), N, C_naive.data(), N, alpha, beta);
    };
    auto naive_checksum = [&]() -> long double { return compute_checksum(C_naive); };

    auto packed_run_once = [&]() {
        blis_like::gemm_runtime<T>(M,
          N,
          K,
          A.data(),
          K,
          B.data(),
          N,
          C_packed.data(),
          N,
          alpha,
          beta,
          ArchIntelUltra7155H::MR,
          ArchIntelUltra7155H::NR,
          ArchIntelUltra7155H::KC,
          ArchIntelUltra7155H::MC,
          ArchIntelUltra7155H::NC);
    };
    auto packed_checksum = [&]() -> long double { return compute_checksum(C_packed); };

    auto dispatch_run_once = [&]() {
        blis_like::gemm_dispatch<T>(M,
          N,
          K,
          A.data(),
          K,
          B.data(),
          N,
          C_dispatch.data(),
          N,
          alpha,
          beta,
          ArchIntelUltra7155H::MR,
          ArchIntelUltra7155H::NR,
          ArchIntelUltra7155H::KC,
          ArchIntelUltra7155H::MC,
          ArchIntelUltra7155H::NC);
    };
    auto dispatch_checksum = [&]() -> long double { return compute_checksum(C_dispatch); };

    // dispatch_unroll benchmark removed per request


    // Run benchmarks
    BenchmarkResult naive_res = run_gemm_benchmark("naive", repeats, naive_run_once, naive_checksum);
    BenchmarkResult packed_res = run_gemm_benchmark("packed", repeats, packed_run_once, packed_checksum);
    BenchmarkResult dispatch_res = run_gemm_benchmark("dispatch", repeats, dispatch_run_once, dispatch_checksum);
    // dispatch_unroll benchmark removed per request
    // xsimd variant removed: dispatch_unroll_xsimd benchmark omitted

    // Compute GFLOPS
    const double flops = 2.0 * double(M) * double(N) * double(K);
    const double gflops_naive = naive_res.avg_seconds > 0.0 ? (flops / naive_res.avg_seconds / 1e9) : 0.0;
    const double gflops_packed = packed_res.avg_seconds > 0.0 ? (flops / packed_res.avg_seconds / 1e9) : 0.0;
    const double gflops_dispatch = dispatch_res.avg_seconds > 0.0 ? (flops / dispatch_res.avg_seconds / 1e9) : 0.0;
    // unroll GFLOPS omitted

    // Print concise results
    std::cout << "M=" << M << " N=" << N << " K=" << K << " repeats=" << repeats << '\n';

    auto print_result = [&](const BenchmarkResult &r, const std::string &label, double gflops) {
        std::cout << '[' << label << "]  avg time (s): " << std::fixed << std::setprecision(6) << r.avg_seconds
                  << "  total time (s): " << r.total_seconds << "  GFLOPS: " << gflops
                  << "  checksum: " << std::setprecision(10) << r.checksum << '\n';
    };

    print_result(naive_res, "naive", gflops_naive);
    print_result(packed_res, "packed", gflops_packed);
    print_result(dispatch_res, "dispatch", gflops_dispatch);

    // Print some simple speedup ratios
    const double speedup_packed_over_naive =
      (packed_res.avg_seconds > 0.0) ? (naive_res.avg_seconds / packed_res.avg_seconds) : 0.0;
    std::cout << "speedup packed/naive: " << std::fixed << std::setprecision(3) << speedup_packed_over_naive << "x\n";

    return 0;
}
